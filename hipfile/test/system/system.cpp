/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile.h"
#include "hipfile-warnings.h"
#include "test-common.h"
#include "test-shared-fixtures.h"

#include <gtest/gtest.h>
#include <hip/hip_runtime.h>

#include <cstddef>
#include <thread>
#include <vector>

HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

// hipFile APIs that trigger driver init/deinit.
// These call into the HIP driver.
TEST_F(DriverInit, hipFileBufRegisterNullptrInitsDriver)
{
    ASSERT_EQ(hipFileBufRegister(nullptr, 4096, 0), HipFileOpError(hipFileInvalidValue));
    ASSERT_EQ(hipFileUseCount(), 1);
}

TEST_F(DriverInit, hipFileBufRegisterInitsDriver)
{
    size_t bufsz{4096};
    void  *buf{nullptr};

    ASSERT_EQ(hipMalloc(&buf, bufsz), hipSuccess);
    ASSERT_EQ(hipFileBufRegister(buf, bufsz, 0), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileUseCount(), 1);
    ASSERT_EQ(hipFileBufDeregister(buf), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFree(buf), hipSuccess);
}

TEST_F(DriverInit, hipFileDriverOpenAfterBufRegisterIncrementsUseCount)
{
    size_t bufsz{4096};
    void  *buf{nullptr};

    ASSERT_EQ(hipMalloc(&buf, bufsz), hipSuccess);
    EXPECT_EQ(hipFileBufRegister(buf, bufsz, 0), HIPFILE_SUCCESS);
    EXPECT_EQ(hipFileUseCount(), 1);

    ASSERT_EQ(hipFileDriverOpen(), HIPFILE_SUCCESS);
    EXPECT_EQ(hipFileUseCount(), 2);

    ASSERT_EQ(hipFileBufDeregister(buf), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFree(buf), hipSuccess);
}

TEST_F(DriverInit, hipFileBufRegisterThreadedInitsDriverOnce)
{
    constexpr size_t         count{64};
    constexpr size_t         bufsz{4096};
    std::vector<std::thread> threads;
    std::vector<void *>      buffers(count);

    for (size_t i{0}; i < count; i++) {
        threads.emplace_back([i, &buffers] {
            // Need to allocate in the same thread that we register in
            // otherwise hipFileBufRegister returns CUDA_ERROR_INVALID_CONTEXT
            // on NVIDIA
            ASSERT_EQ(hipMalloc(&buffers[i], bufsz), hipSuccess);
            EXPECT_EQ(hipFileBufRegister(buffers[i], bufsz, 0), HIPFILE_SUCCESS);
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    ASSERT_EQ(hipFileUseCount(), 1);

    for (size_t i{0}; i < count; i++) {
        EXPECT_EQ(hipFileBufDeregister(buffers[i]), HIPFILE_SUCCESS);
        ASSERT_EQ(hipFree(buffers[i]), hipSuccess);
    }
}

TEST_F(DriverInit, hipFileDriverCloseDeregistersBuffer)
{
    size_t bufsz{4096};
    void  *buf{nullptr};

    ASSERT_EQ(hipMalloc(&buf, bufsz), hipSuccess);
    ASSERT_EQ(hipFileBufRegister(buf, bufsz, 0), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileBufRegister(buf, bufsz, 0), HipFileOpError(hipFileMemoryAlreadyRegistered));
    ASSERT_EQ(hipFileDriverClose(), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileBufRegister(buf, bufsz, 0), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFree(buf), hipSuccess);
}

TEST_F(DriverInit, hipFileStream_register_correct_flags_returns_success)
{
    hipStream_t hip_stream;
    ASSERT_EQ(hipStreamCreate(&hip_stream), hipSuccess);
    ASSERT_EQ(hipFileStreamRegister(hip_stream, HIPFILE_STREAM_FLAGS_MASK), HIPFILE_SUCCESS);
}

TEST_F(DriverInit, hipFileStream_register_then_deregister_returns_success)
{
    hipStream_t hip_stream;
    ASSERT_EQ(hipStreamCreate(&hip_stream), hipSuccess);
    ASSERT_EQ(hipFileStreamRegister(hip_stream, HIPFILE_STREAM_FLAGS_MASK), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileStreamDeregister(hip_stream), HIPFILE_SUCCESS);
}

TEST_F(DriverInit, hipFileStream_register_incorrect_flags_returns_error)
{
    hipStream_t hip_stream;

    ASSERT_EQ(hipStreamCreate(&hip_stream), hipSuccess);
#ifdef __HIP_PLATFORM_AMD__
    ASSERT_EQ(hipFileStreamRegister(hip_stream, HIPFILE_STREAM_FLAGS_MASK + 1),
              HipFileOpError(hipFileInvalidValue));
#else
    ASSERT_EQ(hipFileStreamRegister(hip_stream, HIPFILE_STREAM_FLAGS_MASK + 1), HIPFILE_SUCCESS);
#endif
}

TEST_F(DriverInit, hipFileStream_register_twice_returns_error)
{
    hipStream_t hip_stream;
    ASSERT_EQ(hipStreamCreate(&hip_stream), hipSuccess);
    ASSERT_EQ(hipFileStreamRegister(hip_stream, HIPFILE_STREAM_FLAGS_MASK), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileStreamRegister(hip_stream, HIPFILE_STREAM_FLAGS_MASK),
              HipFileOpError(hipFileInvalidValue));
}

TEST_F(DriverInit, hipFileStream_deregister_twice_returns_error)
{
    hipStream_t hip_stream;
    ASSERT_EQ(hipStreamCreate(&hip_stream), hipSuccess);
    ASSERT_EQ(hipFileStreamRegister(hip_stream, HIPFILE_STREAM_FLAGS_MASK), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileStreamDeregister(hip_stream), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileStreamDeregister(hip_stream), HipFileOpError(hipFileInvalidValue));
}

TEST_F(DriverInit, hipFileStream_deregister_invalid_stream_returns_error)
{
    hipStream_t hip_stream = reinterpret_cast<hipStream_t>(1);
    ASSERT_EQ(hipFileStreamDeregister(hip_stream), HipFileOpError(hipFileInvalidValue));
}

// hipFile APIs that do not trigger driver init/deinit
// These call into the HIP driver.
TEST_F(DriverNoInit, hipFileStreamRegister)
{
    hipStream_t stream;

    ASSERT_EQ(hipStreamCreate(&stream), hipSuccess);
    ASSERT_EQ(hipFileStreamRegister(stream, 0), HIPFILE_SUCCESS);

    ASSERT_EQ(hipFileUseCount(), 0);

    // Cleanup
    ASSERT_EQ(hipFileStreamDeregister(stream), HIPFILE_SUCCESS);
    ASSERT_EQ(hipStreamDestroy(stream), hipSuccess);
}

TEST_F(DriverNoInit, hipFileStreamRegister_register_and_deregister_default_stream_works)
{
    ASSERT_EQ(hipFileStreamRegister(nullptr, 0), HIPFILE_SUCCESS);
    ASSERT_EQ(hipFileStreamDeregister(nullptr), HIPFILE_SUCCESS);
}

TEST_F(DriverNoInit, hipFileStreamDeregisterStream)
{
    hipStream_t stream;

    ASSERT_EQ(hipStreamCreate(&stream), hipSuccess);
    ASSERT_EQ(hipFileStreamRegister(stream, 0), HIPFILE_SUCCESS);

    ASSERT_EQ(hipFileUseCount(), 0);

    ASSERT_EQ(hipFileStreamDeregister(stream), HIPFILE_SUCCESS);

    ASSERT_EQ(hipFileUseCount(), 0);

    ASSERT_EQ(hipStreamDestroy(stream), hipSuccess);
}

HIPFILE_WARN_NO_GLOBAL_CTOR_OFF
