/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "hipfile-warnings.h"
#include "mhip.h"
#include "mmountinfo.h"
#include "msys.h"
#include "rocfile.h"
#include "rocfile-test.h"
#include "sys.h"

#include <cerrno>
#include <cstdint>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>
#include <memory>

using namespace hipFile;
using namespace testing;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

struct RocFileDriverAdmin : public RocFileUnopened {};

// Ensure that rocFileOpen() and rocFileClose() increment and
// decrement the reference count
TEST_F(RocFileDriverAdmin, OpenClose)
{
    const int64_t N = 10;

    ASSERT_EQ(rocFileUseCount(), 0);

    for (int64_t i = 1; i <= N; i++) {
        ASSERT_EQ(rocFileDriverOpen(), ROCFILE_SUCCESS);
        ASSERT_EQ(rocFileUseCount(), i);
    }

    for (int64_t i = N; i >= 1; i--) {
        ASSERT_EQ(rocFileDriverClose(), ROCFILE_SUCCESS);
        ASSERT_EQ(rocFileUseCount(), i - 1);
    }

    ASSERT_EQ(rocFileUseCount(), 0);
}

// Ensure rocFileHandleRegister() initializes the driver
// and DOES bump the reference count
TEST_F(RocFileDriverAdmin, HandleRegisterInitsDriver)
{
    StrictMock<MSys>            msys{};
    StrictMock<MLibMountHelper> mlibmounthelper{};

    rocFileHandle_t handle{};
    rocFileDescr_t  descr{};

    descr.handle.fd = 1234;
    descr.type      = rocFileHandleTypeOpaqueFD;
    descr.fs_ops    = nullptr;

    ASSERT_EQ(rocFileUseCount(), 0);
    expect_file_registration(msys, mlibmounthelper);
    ASSERT_EQ(rocFileHandleRegister(&handle, &descr), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 1);
}

// Ensure rocFileHandleRegister() handles a file descriptor
// of zero (technically a legal POSIX value) and DOES bump
// the reference count
TEST_F(RocFileDriverAdmin, HandleRegisterGoodFD)
{
    StrictMock<MSys>            msys{};
    StrictMock<MLibMountHelper> mlibmounthelper{};

    rocFileHandle_t handle{};
    rocFileDescr_t  descr{};

    descr.handle.fd = 0;
    descr.type      = rocFileHandleTypeOpaqueFD;
    descr.fs_ops    = nullptr;

    expect_file_registration(msys, mlibmounthelper);

    ASSERT_EQ(rocFileUseCount(), 0);
    ASSERT_EQ(rocFileHandleRegister(&handle, &descr), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 1);
    ASSERT_EQ(rocFileHandleDeregister(handle), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 1);
}

// Ensure rocFileHandleRegister() fails when passed a negative file
// descriptor and does NOT bump the reference count
TEST_F(RocFileDriverAdmin, HandleRegisterBadFD)
{
    StrictMock<MSys>            msys{};
    StrictMock<MLibMountHelper> mlibmounthelper{};

    rocFileHandle_t handle{};
    rocFileDescr_t  descr{};

    descr.handle.fd = -1;
    descr.type      = rocFileHandleTypeOpaqueFD;

    EXPECT_CALL(msys, statx).WillOnce(Throw(Sys::RuntimeError(EBADF)));

    ASSERT_EQ(rocFileUseCount(), 0);
    ASSERT_NE(rocFileHandleRegister(&handle, &descr), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 0);
}

// Ensure rocFileHandleDeregister() fails when passed a NULL or
// unregistered pointer and does NOT bump the driver reference count
TEST_F(RocFileDriverAdmin, HandleDeregisterDoesNotInitDriver)
{
    StrictMock<MSys>            msys{};
    StrictMock<MLibMountHelper> mlibmounthelper{};

    // Check NULL
    ASSERT_EQ(rocFileUseCount(), 0);
    ASSERT_NE(rocFileHandleDeregister(nullptr), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 0);

    // Check unregistered handle
    rocFileHandle_t handle{};

    ASSERT_EQ(rocFileUseCount(), 0);
    ASSERT_NE(rocFileHandleDeregister(handle), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 0);
}

// Ensure that closing the driver will also close any open handles. This
// is checked by trying to re-register a handle, which should fail if
// the handle were still open.
TEST_F(RocFileDriverAdmin, CloseDeregistersFile)
{
    StrictMock<MSys>            msys{};
    StrictMock<MLibMountHelper> mlibmounthelper{};

    rocFileHandle_t handle{};
    rocFileDescr_t  descr{};

    descr.handle.fd = 1234;
    descr.type      = rocFileHandleTypeOpaqueFD;
    descr.fs_ops    = nullptr;

    ASSERT_EQ(rocFileUseCount(), 0);
    expect_file_registration(msys, mlibmounthelper);
    ASSERT_EQ(rocFileHandleRegister(&handle, &descr), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 1);
    ASSERT_EQ(rocFileDriverClose(), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 0);
    expect_file_registration(msys, mlibmounthelper);
    ASSERT_EQ(rocFileHandleRegister(&handle, &descr), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 1);
}

// Ensure that registering a buffer increments the driver reference
// count
TEST_F(RocFileDriverAdmin, BufRegisterInitsDriver)
{
    StrictMock<MHip> mhip;

    expect_buffer_registration(mhip, hipMemoryTypeDevice);

    ASSERT_EQ(rocFileUseCount(), 0);
    ASSERT_EQ(rocFileBufRegister(reinterpret_cast<void *>(0x1), 0, 0), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 1);
}

// Ensure that buffer deregistration does not increment the driver
// reference count
TEST_F(RocFileDriverAdmin, BufDeregisterDoesNotInitDriver)
{
    ASSERT_EQ(rocFileUseCount(), 0);
    ASSERT_EQ(rocFileBufDeregister(nullptr), RocFileOpError(rocFileDriverNotInitialized));
    ASSERT_EQ(rocFileUseCount(), 0);
}

// Ensure that closing the driver also closes open buffers. This is
// checked by attempting to re-register a buffer, which should fail
// if it's already registered.
TEST_F(RocFileDriverAdmin, CloseDeregistersBuffer)
{
    StrictMock<MHip> mhip;

    ASSERT_EQ(rocFileUseCount(), 0);
    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    ASSERT_EQ(rocFileBufRegister(reinterpret_cast<void *>(0x1), 0, 0), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 1);

    ASSERT_EQ(rocFileDriverClose(), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 0);

    expect_buffer_registration(mhip, hipMemoryTypeDevice);
    ASSERT_EQ(rocFileBufRegister(reinterpret_cast<void *>(0x1), 0, 0), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileUseCount(), 1);
}

// Ensure rocFileReadAsync():
// * Returns rocFileDriverNotInitialized when called w/o a driver init
// * Does NOT initialize the driver and returns a reference count of 0
TEST_F(RocFileDriverAdmin, ReadAsyncDoesNotInitDriver)
{
    ASSERT_EQ(rocFileReadAsync(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr),
              RocFileOpError(rocFileDriverNotInitialized));
    ASSERT_EQ(rocFileUseCount(), 0);
}

// Ensure rocFileWriteAsync():
// * Returns rocFileDriverNotInitialized when called w/o a driver init
// * Does NOT initialize the driver and returns a reference count of 0
TEST_F(RocFileDriverAdmin, WriteAsyncDoesNotInitDriverDriver)
{
    ASSERT_EQ(rocFileWriteAsync(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr),
              RocFileOpError(rocFileDriverNotInitialized));
    ASSERT_EQ(rocFileUseCount(), 0);
}

// Ensure rocFileRead():
// * Returns rocFileDriverNotInitialized when called w/o a driver init
//   (the weird negative sign is a quirk of returning a ssize_t)
// * Does NOT initialize the driver and returns a reference count of 0
TEST_F(RocFileDriverAdmin, ReadDoesNotInitDriver)
{
    ASSERT_EQ(rocFileRead(nullptr, nullptr, 0, 0, 0), -rocFileDriverNotInitialized);
    ASSERT_EQ(rocFileUseCount(), 0);
}

// Ensure rocFileWrite():
// * Returns rocFileDriverNotInitialized when called w/o a driver init
//   (the weird negative sign is a quirk of returning a ssize_t)
// * Does NOT initialize the driver and returns a reference count of 0
TEST_F(RocFileDriverAdmin, WriteDoesNotInitDriver)
{
    ASSERT_EQ(rocFileWrite(nullptr, nullptr, 0, 0, 0), -rocFileDriverNotInitialized);
    ASSERT_EQ(rocFileUseCount(), 0);
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
