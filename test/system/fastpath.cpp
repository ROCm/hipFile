/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hip.h"
#include "hipfile.h"
#include "mhip.h"

#include "test-common.h"
#include "test-options.h"

#include <cstdlib>
#include <unistd.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>

//using namespace hipFile;
using namespace testing;
using namespace std;

extern SystemTestOptions test_env;

HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

enum IoType {
    Read = 0,
    Write = 1
}

struct FastpathWithFallbackIO : public TestWithParam<std::tuple<IoType, std::exception_ptr>> {
    
    Tmpfile         tmpfile;
    size_t          tmpfile_size;
    hipFileHandle_t tmpfile_handle;
    void           *device_buffer;
    size_t          device_buffer_size;

    NiceMock<hipFile::MHip> mhip;

    FastpathWithFallbackIO()
        : tmpfile{test_env.ais_capable_dir}, tmpfile_size{1024 * 1024}, tmpfile_handle{nullptr},
          device_buffer{nullptr}, device_buffer_size{1024 * 1024}
    {
    }

    void SetUp() override
    {
        if (unsetenv("HIPFILE_FORCE_COMPAT_MODE")) {
            FAIL() << "Could not clear HIPFILE_FORCE_COMPAT_MODE";
        }
        if (setenv("HIPFILE_ALLOW_COMPAT_MODE", "true", 1)) {
            FAIL() << "Could not set HIPFILE_ALLOW_COMPAT_MODE=true";
        }
        // Must be called prior to any expectations on Hip set.
        mhip.enable_passthrough();

        switch (_get_param_io_type()) {
            case IoType::Read:
                EXPECT_CALL(mhip, hipAmdFileRead);
                break;
            case IoType::Write:
                EXPECT_CALL(mhip, hipAmdFileWrite);
                break;
            default:
                FAIL() << "Unsupported IoTestBackend";
        }

        ASSERT_EQ(0, ftruncate(tmpfile.fd, static_cast<off_t>(tmpfile_size)));

        hipFileDescr_t descr{};
        descr.type      = hipFileHandleTypeOpaqueFD;
        descr.handle.fd = tmpfile.fd;

        ASSERT_EQ(HIPFILE_SUCCESS, hipFileHandleRegister(&tmpfile_handle, &descr));
        ASSERT_EQ(hipSuccess, hipMalloc(&device_buffer, device_buffer_size));
        ASSERT_EQ(HIPFILE_SUCCESS, hipFileBufRegister(device_buffer, device_buffer_size, 0));
    }

    void TearDown() override
    {
        ASSERT_EQ(HIP_SUCCESS, hipFileBufDeregister(device_buffer););
        ASSERT_EQ(hipSuccess, hipFree(device_buffer));
        hipFileHandleDeregister(tmpfile_handle);
    }

    inline IoType _get_param_io_type() const
    {
        return std::get<0>(GetParam());
    }

    inline const std::exception_ptr _get_param_exc_ptr() const
    {
        return std::get<1>(GetParam());
    }
};

INSTANTIATE_TEST_SUITE_P(
    , FastpathWithFallbackIO,
    Combine(Values(IoType::Read, IoType::Write),
            Values(std::make_exception_ptr(hipFile::Hip::RuntimeError(hipErrorNoDevice)),
                   std::make_exception_ptr(std::system_error(make_error_code(errc::no_such_device))))));

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
