/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile-warnings.h"
#include "hipfile.h"

#include "test-common.h"
#include "test-options.h"

#include <cstdlib>
#include <unistd.h>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>

extern SystemTestOptions test_env;

HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

enum class IoTestBackend {
    Fastpath,
    Fallback,
};

struct HipFileIo : public testing::TestWithParam<IoTestBackend> {

    Tmpfile         tmpfile;
    size_t          tmpfile_size;
    hipFileHandle_t tmpfile_handle;
    void           *unregistered_device_buffer;
    size_t          unregistered_device_buffer_size;

    HipFileIo()
        : tmpfile{test_env.ais_capable_dir}, tmpfile_size{1024 * 1024}, tmpfile_handle{nullptr},
          unregistered_device_buffer{nullptr}, unregistered_device_buffer_size{1024 * 1024}
    {
    }

    // Must be called before hipfile is initialized. Relies on each test being
    // run in a separate process
    void enable_fastpath_only()
    {
        if (unsetenv("HIPFILE_FORCE_COMPAT_MODE")) {
            FAIL() << "Could not clear HIPFILE_FORCE_COMPAT_MODE";
        }
        if (setenv("HIPFILE_ALLOW_COMPAT_MODE", "false", 1)) {
            FAIL() << "Could not set HIPFILE_ALLOW_COMPAT_MODE=false";
        }
    }

    // Must be called before hipfile is initialized. Relies on each test being
    // run in a separate process
    void enable_fallback_only()
    {
        if (unsetenv("HIPFILE_ALLOW_COMPAT_MODE")) {
            FAIL() << "Could not clear HIPFILE_ALLOW_COMPAT_MODE";
        }
        if (setenv("HIPFILE_FORCE_COMPAT_MODE", "true", 1)) {
            FAIL() << "Could not set HIPFILE_FORCE_COMPAT_MODE=true";
        }
    }

    void SetUp() override
    {
        switch (GetParam()) {
            case IoTestBackend::Fastpath:
                enable_fastpath_only();
                break;

            case IoTestBackend::Fallback:
                enable_fallback_only();
                break;

            default:
                FAIL() << "Unsupported IoTestBackend";
        }

        ASSERT_EQ(0, ftruncate(tmpfile.fd, static_cast<off_t>(tmpfile_size)));

        hipFileDescr_t descr{};
        descr.type      = hipFileHandleTypeOpaqueFD;
        descr.handle.fd = tmpfile.fd;

        ASSERT_EQ(HIPFILE_SUCCESS, hipFileHandleRegister(&tmpfile_handle, &descr));

        ASSERT_EQ(hipSuccess, hipMalloc(&unregistered_device_buffer, unregistered_device_buffer_size));
    }

    void TearDown() override
    {
        ASSERT_EQ(hipSuccess, hipFree(unregistered_device_buffer));

        hipFileHandleDeregister(tmpfile_handle);
    }
};

TEST_P(HipFileIo, ReadToUnregisteredBufferAtOffset)
{
    hoff_t io_buffer_offset{4096};
    size_t io_size{unregistered_device_buffer_size - static_cast<size_t>(io_buffer_offset)};

    ASSERT_EQ(io_size, hipFileRead(tmpfile_handle, unregistered_device_buffer, io_size, 0, io_buffer_offset));
}

TEST_P(HipFileIo, ReadToUnregisteredBufferAtOffsetReturnsErrorIfOverflow)
{
    hoff_t io_buffer_offset{4096};
    size_t io_size{unregistered_device_buffer_size};

    ASSERT_EQ(-hipFileInvalidValue,
              hipFileRead(tmpfile_handle, unregistered_device_buffer, io_size, 0, io_buffer_offset));
}

INSTANTIATE_TEST_SUITE_P(, HipFileIo, testing::Values(IoTestBackend::Fastpath, IoTestBackend::Fallback));

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
