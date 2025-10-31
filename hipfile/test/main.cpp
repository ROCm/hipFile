/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile.h"
#include "test-common.h"
#include "test-shared-fixtures.h"

#include "hipfile-warnings.h"
#include "invalid-enum.h"

#include <gtest/gtest.h>
#include <hip/hip_runtime.h>

#include <ctime>
#include <limits>
#include <thread>
#include <vector>

using namespace std;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

TEST(Helpers, invalidEnum)
{
    enum foo {
        a = -7,
        b = 12,
    };
    ASSERT_EQ(invalidEnum<foo>(a - 1), -8);
    ASSERT_EQ(invalidEnum<foo>(a + 1), -6);
    ASSERT_EQ(invalidEnum<foo>(b - 1), 11);
    ASSERT_EQ(invalidEnum<foo>(b + 1), 13);
}

TEST(HipFileVersioning, Get)
{
    unsigned major = UINT_MAX;
    unsigned minor = UINT_MAX;
    unsigned patch = UINT_MAX;

    // Check for correct values
    ASSERT_EQ(hipFileGetVersion(&major, &minor, &patch), HIPFILE_SUCCESS);
    ASSERT_EQ(major, HIPFILE_VERSION_MAJOR);
    ASSERT_EQ(minor, HIPFILE_VERSION_MINOR);
    ASSERT_EQ(patch, HIPFILE_VERSION_PATCH);

    // NULL pointers should NOT produce errors
    ASSERT_EQ(hipFileGetVersion(nullptr, nullptr, nullptr), HIPFILE_SUCCESS);

    // hipFileGetBackendVersion() succeeds and returns a value >= 0
    //
    // We can't reliably predict what the version number will be for
    // an arbitrary library, but it probably won't be negative and
    // checking for >= 0 ensures the -1 initialization value is
    // overwritten.
    int backend_version = -1;
    ASSERT_EQ(hipFileGetBackendVersion(&backend_version), HIPFILE_SUCCESS);
    ASSERT_GE(backend_version, 0);

    // NULL pointer returns correct error
    ASSERT_EQ(hipFileGetBackendVersion(nullptr), HipFileOpError(hipFileInvalidValue));
}

// Test hipFile APIs that set/get configuration values
struct HipFileConfig : public ::testing::Test {

    void SetUp() override
    {
        ASSERT_EQ(hipFileUseCount(), 0);
    }

    // Ensure the driver is deinitialized/closed after the test
    void TearDown() override
    {
        // Ensure driver is opened so staged values are applied then cleared
        ASSERT_EQ(hipFileDriverOpen(), HIPFILE_SUCCESS);

        // Ensure driver is closed to clear applied values
        while (hipFileUseCount()) {
            ASSERT_EQ(hipFileDriverClose(), HIPFILE_SUCCESS);
        }
    }
};

TEST_F(HipFileConfig, SizeTParameterCantSetIfDriverOpen)
{
    ASSERT_EQ(hipFileDriverOpen(), HIPFILE_SUCCESS);
    auto set_after_open = hipFileSetParameterSizeT(hipFileParamExecutionMaxIOThreads, 10);

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(set_after_open, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(set_after_open, HipFileOpError(hipFileDriverAlreadyOpen));
#endif
}

TEST_F(HipFileConfig, SizeTParameterGetDefault)
{
    size_t default_;
    auto   get_default = hipFileGetParameterSizeT(hipFileParamExecutionMaxIOThreads, &default_);

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_EQ(default_, 0);
#endif
}

TEST_F(HipFileConfig, SizeTParameterSetStagesValue)
{
    size_t default_;
    auto   get_default = hipFileGetParameterSizeT(hipFileParamExecutionMaxIOThreads, &default_);

    size_t target{10};
    auto   stage_target = hipFileSetParameterSizeT(hipFileParamExecutionMaxIOThreads, target);

    size_t staged;
    auto   get_staged = hipFileGetParameterSizeT(hipFileParamExecutionMaxIOThreads, &staged);

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(stage_target, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(get_staged, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_NE(default_, target);
    ASSERT_EQ(stage_target, HIPFILE_SUCCESS);
    ASSERT_EQ(get_staged, HIPFILE_SUCCESS);
    ASSERT_EQ(target, staged);
#endif
}

TEST_F(HipFileConfig, SizeTParameterOpenAppliesValue)
{
    size_t default_;
    auto   get_default = hipFileGetParameterSizeT(hipFileParamExecutionMaxIOThreads, &default_);

    size_t target{10};
    auto   stage_target = hipFileSetParameterSizeT(hipFileParamExecutionMaxIOThreads, target);

    auto driver_open = hipFileDriverOpen();

    size_t applied;
    auto   get_applied = hipFileGetParameterSizeT(hipFileParamExecutionMaxIOThreads, &applied);

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(stage_target, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(get_applied, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_NE(default_, target);
    ASSERT_EQ(stage_target, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(get_applied, HIPFILE_SUCCESS);
    ASSERT_EQ(target, applied);
#endif
}

TEST_F(HipFileConfig, SizeTParameterCloseClearsAppliedValue)
{
    size_t default_;
    auto   get_default = hipFileGetParameterSizeT(hipFileParamExecutionMaxIOThreads, &default_);

    size_t target{10};
    auto   stage_target = hipFileSetParameterSizeT(hipFileParamExecutionMaxIOThreads, target);

    // Open applies staged value and clears staged
    auto driver_open = hipFileDriverOpen();

    // Close clears applied value
    auto driver_close = hipFileDriverClose();

    size_t cleared;
    auto   get_cleared = hipFileGetParameterSizeT(hipFileParamExecutionMaxIOThreads, &cleared);

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(stage_target, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_close, HIPFILE_SUCCESS);
    ASSERT_EQ(get_cleared, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_NE(default_, target);
    ASSERT_EQ(stage_target, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_close, HIPFILE_SUCCESS);
    ASSERT_EQ(get_cleared, HIPFILE_SUCCESS);
    ASSERT_EQ(default_, cleared);
#endif
}

TEST_F(HipFileConfig, BoolParameterCantSetIfDriverOpen)
{
    ASSERT_EQ(hipFileDriverOpen(), HIPFILE_SUCCESS);
    auto set_after_open = hipFileSetParameterBool(hipFileParamUsePcip2pdma, true);
#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(set_after_open, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(set_after_open, HipFileOpError(hipFileDriverAlreadyOpen));
#endif
}

TEST_F(HipFileConfig, BoolParameterGetDefault)
{
    bool default_;
    auto get_default = hipFileGetParameterBool(hipFileParamUsePcip2pdma, &default_);
#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_EQ(default_, false);
#endif
}

TEST_F(HipFileConfig, BoolParameterSetStagesValue)
{
    bool default_;
    auto get_default = hipFileGetParameterBool(hipFileParamUsePcip2pdma, &default_);

    constexpr bool target{true};
    auto           stage_target = hipFileSetParameterBool(hipFileParamUsePcip2pdma, target);

    bool staged;
    auto get_staged = hipFileGetParameterBool(hipFileParamUsePcip2pdma, &staged);

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(stage_target, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(get_staged, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_NE(default_, target);
    ASSERT_EQ(stage_target, HIPFILE_SUCCESS);
    ASSERT_EQ(get_staged, HIPFILE_SUCCESS);
    ASSERT_EQ(target, staged);
#endif
}

TEST_F(HipFileConfig, BoolParameterOpenAppliesValue)
{
    bool default_;
    auto get_default = hipFileGetParameterBool(hipFileParamUsePcip2pdma, &default_);

    constexpr bool target{true};
    auto           stage_target = hipFileSetParameterBool(hipFileParamUsePcip2pdma, target);

    auto driver_open = hipFileDriverOpen();

    bool applied;
    auto get_applied = hipFileGetParameterBool(hipFileParamUsePcip2pdma, &applied);

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(stage_target, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(get_applied, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_NE(default_, target);
    ASSERT_EQ(stage_target, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(get_applied, HIPFILE_SUCCESS);
    ASSERT_EQ(target, applied);
#endif
}

TEST_F(HipFileConfig, BoolParameterCloseClearsAppliedValue)
{
    bool default_;
    auto get_default = hipFileGetParameterBool(hipFileParamUsePcip2pdma, &default_);

    constexpr bool target{true};
    auto           stage_target = hipFileSetParameterBool(hipFileParamUsePcip2pdma, target);

    // Open applies staged value and clears staged
    auto driver_open = hipFileDriverOpen();

    // Close clears applied value
    auto driver_close = hipFileDriverClose();

    bool cleared;
    auto get_cleared = hipFileGetParameterBool(hipFileParamUsePcip2pdma, &cleared);

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(stage_target, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_close, HIPFILE_SUCCESS);
    ASSERT_EQ(get_cleared, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_NE(default_, target);
    ASSERT_EQ(stage_target, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_close, HIPFILE_SUCCESS);
    ASSERT_EQ(get_cleared, HIPFILE_SUCCESS);
    ASSERT_EQ(default_, cleared);
#endif
}

TEST_F(HipFileConfig, StringParameterCantSetIfDriverOpen)
{
    ASSERT_EQ(hipFileDriverOpen(), HIPFILE_SUCCESS);
    auto set_after_open = hipFileSetParameterString(hipFileParamLogDir, "/foo/bar");
#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(set_after_open, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(set_after_open, HipFileOpError(hipFileDriverAlreadyOpen));
#endif
}

TEST_F(HipFileConfig, StringParameterGetDefault)
{
    vector<char> buffer(64);

    auto get_default =
        hipFileGetParameterString(hipFileParamLogDir, buffer.data(), static_cast<int>(buffer.size()));
    string default_{buffer.data()};

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_EQ(default_, "");
#endif
}

TEST_F(HipFileConfig, StringParameterSetStagesValue)
{
    vector<char> buffer(64);

    auto get_default =
        hipFileGetParameterString(hipFileParamLogDir, buffer.data(), static_cast<int>(buffer.size()));
    string default_{buffer.data()};

    const string target{"/tmp"};
    auto         stage_target = hipFileSetParameterString(hipFileParamLogDir, target.c_str());

    auto get_staged =
        hipFileGetParameterString(hipFileParamLogDir, buffer.data(), static_cast<int>(buffer.size()));
    string staged{buffer.data()};

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(stage_target, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(get_staged, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_NE(default_, target);
    ASSERT_EQ(stage_target, HIPFILE_SUCCESS);
    ASSERT_EQ(get_staged, HIPFILE_SUCCESS);
    ASSERT_EQ(target, staged);
#endif
}

TEST_F(HipFileConfig, StringParameterOpenAppliesValue)
{
    vector<char> buffer(64);

    auto get_default =
        hipFileGetParameterString(hipFileParamLogDir, buffer.data(), static_cast<int>(buffer.size()));
    string default_{buffer.data()};

    // The target value must specify a directory that exists otherwise it will not be applied when the driver
    // is opened.
    const string target{"/tmp"};
    auto         stage_target = hipFileSetParameterString(hipFileParamLogDir, target.c_str());

    auto driver_open = hipFileDriverOpen();

    auto get_applied =
        hipFileGetParameterString(hipFileParamLogDir, buffer.data(), static_cast<int>(buffer.size()));
    string applied{buffer.data()};

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(stage_target, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(get_applied, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_NE(default_, target);
    ASSERT_EQ(stage_target, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(get_applied, HIPFILE_SUCCESS);
    ASSERT_EQ(target, applied);
#endif
}

TEST_F(HipFileConfig, StringParameterCloseClearsAppliedValue)
{
    vector<char> buffer(64);

    auto get_default =
        hipFileGetParameterString(hipFileParamLogDir, buffer.data(), static_cast<int>(buffer.size()));
    string default_{buffer.data()};

    // The target value must specify a directory that exists otherwise it will not be applied when the driver
    // is opened.
    const string target{"/tmp"};
    auto         stage_target = hipFileSetParameterString(hipFileParamLogDir, target.c_str());

    auto driver_open  = hipFileDriverOpen();
    auto driver_close = hipFileDriverClose();

    auto get_cleared =
        hipFileGetParameterString(hipFileParamLogDir, buffer.data(), static_cast<int>(buffer.size()));
    string cleared{buffer.data()};

#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(get_default, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(stage_target, HipFileOpError(hipFileInternalError));
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_close, HIPFILE_SUCCESS);
    ASSERT_EQ(get_cleared, HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(get_default, HIPFILE_SUCCESS);
    ASSERT_NE(default_, target);
    ASSERT_EQ(stage_target, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_open, HIPFILE_SUCCESS);
    ASSERT_EQ(driver_close, HIPFILE_SUCCESS);
    ASSERT_EQ(get_cleared, HIPFILE_SUCCESS);
    ASSERT_EQ(default_, cleared);
#endif
}

// hipFile APIs that trigger driver init/deinit.

TEST_F(DriverInit, driverOpenIncrementsUseCount)
{
    for (auto i{0}; i < 10; i++) {
        ASSERT_EQ(hipFileDriverOpen(), HIPFILE_SUCCESS);
        ASSERT_EQ(hipFileUseCount(), i + 1);
    }
}

TEST_F(DriverInit, driverCloseDecrementsUseCount)
{
    auto count{10};
    for (auto i{0}; i < count; i++) {
        ASSERT_EQ(hipFileDriverOpen(), HIPFILE_SUCCESS);
        ASSERT_EQ(hipFileUseCount(), i + 1);
    }
    for (auto i{count}; 0 < i; i--) {
        ASSERT_EQ(hipFileDriverClose(), HIPFILE_SUCCESS);
        ASSERT_EQ(hipFileUseCount(), i - 1);
    }
}

TEST_F(DriverInit, hipFileHandleRegisterNullDescr)
{
    hipFileHandle_t handle;
    ASSERT_EQ(hipFileHandleRegister(&handle, nullptr), HipFileOpError(hipFileInvalidValue));

    // NVIDIA inits the driver, even when the parameters are garbage. We do not.
    // We could mimic this, but it's probably a waste of time given that it's an
    // edge case.
#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(hipFileUseCount(), 0);
#else
    ASSERT_EQ(hipFileUseCount(), 1);
#endif
}

TEST_F(DriverInit, hipFileHandleRegisterNullHandle)
{
    Tmpfile tmpfile;

    hipFileDescr_t descr{};
    descr.type      = hipFileHandleTypeOpaqueFD;
    descr.handle.fd = tmpfile.fd;

    ASSERT_EQ(hipFileHandleRegister(nullptr, &descr), HipFileOpError(hipFileInvalidValue));

    // NVIDIA inits the driver, even when the parameters are garbage. We do not.
    // We could mimic this, but it's probably a waste of time given that it's an
    // edge case.
#if defined(__HIP_PLATFORM_AMD__)
    ASSERT_EQ(hipFileUseCount(), 0);
#else
    ASSERT_EQ(hipFileUseCount(), 1);
#endif
}

TEST_F(DriverInit, hipFileHandleRegisterInitsDriver)
{
    Tmpfile tmpfile;

    hipFileDescr_t descr{};
    descr.type      = hipFileHandleTypeOpaqueFD;
    descr.handle.fd = tmpfile.fd;

    hipFileHandle_t handle;
    ASSERT_EQ(hipFileHandleRegister(&handle, &descr), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileUseCount(), 1);
}

TEST_F(DriverInit, hipFileDriverOpenAfterHandleRegisterIncrementsUseCount)
{
    Tmpfile tmpfile;

    hipFileDescr_t descr{};
    descr.type      = hipFileHandleTypeOpaqueFD;
    descr.handle.fd = tmpfile.fd;

    hipFileHandle_t handle;
    ASSERT_EQ(hipFileHandleRegister(&handle, &descr), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileUseCount(), 1);

    ASSERT_EQ(hipFileDriverOpen(), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileUseCount(), 2);
}

TEST_F(DriverInit, hipFileHandleRegisterThreadedInitsDriverOnce)
{
    constexpr size_t             count{64};
    std::vector<Tmpfile>         tmpfiles(count);
    std::vector<std::thread>     threads;
    std::vector<hipFileDescr_t>  descrs(count);
    std::vector<hipFileHandle_t> handles(count);

    for (size_t i{0}; i < count; i++) {
        descrs[i].type      = hipFileHandleTypeOpaqueFD;
        descrs[i].handle.fd = tmpfiles[i].fd;
    }

    for (size_t i{0}; i < count; i++) {
        threads.emplace_back([i, &handles, &descrs] {
            EXPECT_EQ(hipFileHandleRegister(&handles[i], &descrs[i]), HIPFILE_SUCCESS);
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    ASSERT_EQ(hipFileUseCount(), 1);

    for (size_t i{0}; i < count; i++) {
        hipFileHandleDeregister(handles[i]);
    }
}

TEST_F(DriverInit, hipFileDriverCloseDeregisteresHandle)
{
    Tmpfile         tmpfile;
    hipFileHandle_t handle{};
    hipFileDescr_t  descr{};

    descr.type      = hipFileHandleTypeOpaqueFD;
    descr.handle.fd = tmpfile.fd;

    ASSERT_EQ(hipFileHandleRegister(&handle, &descr), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileHandleRegister(&handle, &descr), HipFileOpError(hipFileHandleAlreadyRegistered));
    ASSERT_EQ(hipFileDriverClose(), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileHandleRegister(&handle, &descr), HIPFILE_SUCCESS);
}

TEST_F(DriverInit, hipFileReadAsync)
{
    ASSERT_EQ(hipFileReadAsync(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr),
              HipFileOpError(hipFileInvalidValue));
    ASSERT_EQ(hipFileUseCount(), 1);
}

TEST_F(DriverInit, hipFileWriteAsync)
{
    ASSERT_EQ(hipFileWriteAsync(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr),
              HipFileOpError(hipFileInvalidValue));
    ASSERT_EQ(hipFileUseCount(), 1);
}

// hipFile APIs that do not trigger driver init/deinit

TEST_F(DriverNoInit, hipFileDriverCloseReturnsNotInitIfDriverNotOpened)
{
    ASSERT_EQ(hipFileDriverClose(), HipFileOpError(hipFileDriverNotInitialized));
}

TEST_F(DriverNoInit, hipFileBatchIOSetUp)
{
    hipFileBatchHandle_t handle;
#ifdef __HIP_PLATFORM_AMD__
    ASSERT_EQ(hipFileBatchIOSetUp(&handle, 1), HipFileOpError(hipFileSuccess));
#else
    // CU_FILE_INTERNAL_ERROR returned if this API is called prior to the driver being opened.
    ASSERT_EQ(hipFileBatchIOSetUp(&handle, 1), HipFileOpError(hipFileInternalError));
#endif
}

TEST_F(DriverNoInit, hipFileBatchIOSubmitNullArgs)
{
#ifdef __HIP_PLATFORM_AMD__
    ASSERT_EQ(hipFileBatchIOSubmit(nullptr, 0, nullptr, 0), HipFileOpError(hipFileInvalidValue));
#else
    ASSERT_EQ(hipFileBatchIOSubmit(nullptr, 0, nullptr, 0), HipFileOpError(hipFileInternalError));
#endif
}

TEST_F(DriverNoInit, hipFileBatchIOSubmit)
{
    hipFileBatchHandle_t handle{};
    hipFileIOParams_t    param{};

    ASSERT_EQ(hipFileBatchIOSubmit(handle, 1, &param, 0), HipFileOpError(hipFileInternalError));
}

TEST_F(DriverNoInit, hipFileBatchIOGetStatusNullArgs)
{
    ASSERT_EQ(hipFileBatchIOGetStatus(nullptr, 0, nullptr, nullptr, nullptr),
              HipFileOpError(hipFileInternalError));
}

TEST_F(DriverNoInit, hipFileBatchIOGetStatus)
{
    hipFileBatchHandle_t handle{};
    hipFileIOEvents_t    event{};
    unsigned             nr{1};
    struct timespec      ts {
        0, 0
    };

    ASSERT_EQ(hipFileBatchIOGetStatus(handle, 0, &nr, &event, &ts), HipFileOpError(hipFileInternalError));
}

TEST_F(DriverNoInit, hipFileBatchIOCancelNullArgs)
{
#ifdef __HIP_PLATFORM_AMD__ // Not implemented on AMD
    ASSERT_EQ(hipFileBatchIOCancel(nullptr), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileBatchIOCancel(nullptr), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNoInit, hipFileBatchIOCancel)
{
    hipFileBatchHandle_t handle{};

#ifdef __HIP_PLATFORM_AMD__ // Not implemented on AMD
    ASSERT_EQ(hipFileBatchIOCancel(handle), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileBatchIOCancel(handle), HIPFILE_SUCCESS); // Weird
#endif
}

TEST_F(DriverNoInit, hipFileBatchIODestroy)
{
    hipFileBatchHandle_t handle{};

    hipFileBatchIODestroy(handle);
}

TEST_F(DriverNoInit, hipFileGetVersion)
{
    unsigned major = UINT_MAX;
    unsigned minor = UINT_MAX;
    unsigned patch = UINT_MAX;
    ASSERT_EQ(hipFileGetVersion(&major, &minor, &patch), HIPFILE_SUCCESS);

    int version = -1;
    ASSERT_EQ(hipFileGetBackendVersion(&version), HIPFILE_SUCCESS);
}

TEST_F(DriverNoInit, hipFileGetParameterSizeT)
{
    size_t value;
#ifdef __HIP_PLATFORM_AMD__ // Not implemented on AMD
    ASSERT_EQ(hipFileGetParameterSizeT(hipFileParamProfileStats, &value),
              HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileGetParameterSizeT(hipFileParamProfileStats, &value), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNoInit, hipFileGetParameterBool)
{
    bool value;
#ifdef __HIP_PLATFORM_AMD__ // Not implemented on AMD
    ASSERT_EQ(hipFileGetParameterBool(hipFileParamUsePcip2pdma, &value),
              HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileGetParameterBool(hipFileParamUsePcip2pdma, &value), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNoInit, hipFileGetParameterString)
{
    vector<char> buffer(64);
#ifdef __HIP_PLATFORM_AMD__ // Not implemented on AMD
    ASSERT_EQ(hipFileGetParameterString(hipFileParamLogDir, buffer.data(), static_cast<int>(buffer.size())),
              HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileGetParameterString(hipFileParamLogDir, buffer.data(), static_cast<int>(buffer.size())),
              HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNoInit, hipFileSetParameterSizeT)
{
#ifdef __HIP_PLATFORM_AMD__ // Not implemented on AMD
    ASSERT_EQ(hipFileSetParameterSizeT(hipFileParamProfileStats, 1), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileSetParameterSizeT(hipFileParamProfileStats, 1), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNoInit, hipFileSetParameterBool)
{
#ifdef __HIP_PLATFORM_AMD__ // Not implemented on AMD
    ASSERT_EQ(hipFileSetParameterBool(hipFileParamUsePcip2pdma, false), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileSetParameterBool(hipFileParamUsePcip2pdma, false), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNoInit, hipFileSetParameterString)
{
#ifdef __HIP_PLATFORM_AMD__ // Not implemented on AMD
    ASSERT_EQ(hipFileSetParameterString(hipFileParamLogDir, "/tmp"), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileSetParameterString(hipFileParamLogDir, "/tmp"), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNoInit, hipFileBufDeregister)
{
    EXPECT_EQ(hipFileBufDeregister(reinterpret_cast<void *>(0x1)), HipFileOpError(hipFileDriverClosing));
}

TEST_F(DriverNoInit, hipFileHandleDeregister)
{
    hipFileHandleDeregister(reinterpret_cast<hipFileHandle_t>(0x1));
}

TEST_F(DriverNoInit, hipFileRead)
{
    vector<uint8_t> buffer(64);
    hipFileHandle_t handle{reinterpret_cast<void *>(0xCAFEBABE)};

    errno = 0;
    ASSERT_EQ(hipFileRead(handle, buffer.data(), buffer.size(), 0, 0), -1);
    ASSERT_EQ(errno, EINVAL);
}

TEST_F(DriverNoInit, hipFileWrite)
{
    vector<uint8_t> buffer(64);
    hipFileHandle_t handle{reinterpret_cast<void *>(0xCAFEBABE)};

    errno = 0;
    ASSERT_EQ(hipFileWrite(handle, buffer.data(), buffer.size(), 0, 0), -1);
    ASSERT_EQ(errno, EINVAL);
}

// DriverNotReallyNoInit tests API functions that are not supposed to
// initialize the driver but they have some side effect (in cuFile) that makes
// hipFileSetParameterSizeT/hipFileSetParameterBool/hipFileSetParameterString
// return hipFileDriverAlreadyOpen. They are split out from DriverNoInit tests
// because the driver needs to be cycled to clear out driver state. Cycling the
// driver is time consuming.
struct DriverNotReallyNoInit : public DriverNoInit {

    void SetUp() override
    {
        ASSERT_EQ(hipFileUseCount(), 0);
        ASSERT_EQ(hipFileDriverClose(), HipFileOpError(hipFileDriverNotInitialized));
    }

    void TearDown() override
    {
        ASSERT_EQ(hipFileUseCount(), 0);
        ASSERT_EQ(hipFileDriverClose(), HipFileOpError(hipFileDriverNotInitialized));

#ifdef __HIP_PLATFORM_NVIDIA__
        // Workaround: Open and close the driver to clear the state that these API functions set
        ASSERT_EQ(hipFileDriverOpen(), HIPFILE_SUCCESS);
        ASSERT_EQ(hipFileDriverClose(), HIPFILE_SUCCESS);
#endif

        ASSERT_EQ(hipFileUseCount(), 0);
        ASSERT_EQ(hipFileDriverClose(), HipFileOpError(hipFileDriverNotInitialized));
    }
};

TEST_F(DriverNotReallyNoInit, hipFileDriverGetPropertiesNullArgs)
{
#ifdef __HIP_PLATFORM_AMD__ // Not implemented
    ASSERT_EQ(hipFileDriverGetProperties(nullptr), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileDriverGetProperties(nullptr), HipFileOpError(hipFileInvalidValue));
#endif
}

TEST_F(DriverNotReallyNoInit, hipFileDriverGetProperties)
{
    hipFileDriverProps_t props;

#ifdef __HIP_PLATFORM_AMD__ // Not implemented
    ASSERT_EQ(hipFileDriverGetProperties(&props), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileDriverGetProperties(&props), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNotReallyNoInit, hipFileDriverSetPollMode)
{
#ifdef __HIP_PLATFORM_AMD__ // Not implemented
    ASSERT_EQ(hipFileDriverSetPollMode(true, 4096), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileDriverSetPollMode(true, 4096), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNotReallyNoInit, hipFileDriverSetMaxDirectIOSize)
{
#ifdef __HIP_PLATFORM_AMD__ // Not implemented
    ASSERT_EQ(hipFileDriverSetMaxDirectIOSize(16 * 1024), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileDriverSetMaxDirectIOSize(16 * 1024), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNotReallyNoInit, hipFileDriverSetMaxCacheSize)
{
#ifdef __HIP_PLATFORM_AMD__ // Not implemented
    ASSERT_EQ(hipFileDriverSetMaxCacheSize(16 * 1024), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileDriverSetMaxCacheSize(16 * 1024), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverNotReallyNoInit, hipFileDriverSetMaxPinnedMemSize)
{
#ifdef __HIP_PLATFORM_AMD__ // Not implemented
    ASSERT_EQ(hipFileDriverSetMaxPinnedMemSize(32 * 1024), HipFileOpError(hipFileInternalError));
#else
    ASSERT_EQ(hipFileDriverSetMaxPinnedMemSize(32 * 1024), HIPFILE_SUCCESS);
#endif
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
