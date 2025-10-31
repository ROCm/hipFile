/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "rocfile-test.h"

#include "msys.h"
#include "mmountinfo.h"

#include "context.h"
#include "file.h"
#include "rocfile.h"
#include "state.h"

#include "hipfile-warnings.h"

#include <gtest/gtest.h>

#include <fcntl.h>
#include <sys/sysmacros.h>

using namespace rocFile;

using ::testing::Return;
using ::testing::StrictMock;
using ::testing::Throw;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

struct RocFileHandle : public RocFileOpened {};

TEST_F(RocFileHandle, register_handle_internal_linux_fd)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    int fd{0xBADF00D};

    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    EXPECT_CALL(mlibmounthelper, getMountInfo);
    ASSERT_NE(Context<DriverState>::get()->registerFile(fd), nullptr);
}

TEST_F(RocFileHandle, file_initialization)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    int         fd{0x12345678};
    int         status_flags{0x789ABCDE};
    struct stat fstat;
    fstat.st_dev  = makedev(0x12345678, 0x87654321);
    fstat.st_mode = 0xFEDCBA98;
    MountInfo mountinfo;
    mountinfo.type                         = FilesystemType::ext4;
    mountinfo.options.ext4.journaling_mode = ExtJournalingMode::ordered;

    EXPECT_CALL(msys, fstat(fd)).WillOnce(Return(fstat));
    EXPECT_CALL(msys, fcntl(fd, F_GETFL, 0)).WillOnce(Return(status_flags));
    EXPECT_CALL(mlibmounthelper, getMountInfo(fstat.st_dev)).WillOnce(Return(std::make_optional(mountinfo)));
    auto fh{Context<DriverState>::get()->registerFile(fd)};
    auto file{Context<DriverState>::get()->getFile(fh)};

    EXPECT_EQ(fh, file->getHandle());
    EXPECT_EQ(fd, file->getFd());
    EXPECT_EQ(fstat.st_dev, file->getDevice());
    EXPECT_EQ(fstat.st_mode, file->getMode());
    EXPECT_EQ(status_flags, file->getStatusFlags());
    EXPECT_EQ(mountinfo.type, file->getMountInfo().value().type);
    EXPECT_EQ(mountinfo.options.ext4.journaling_mode,
              file->getMountInfo().value().options.ext4.journaling_mode);
}

TEST_F(RocFileHandle, register_handle_internal_linux_fd_already_registered)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    int fd{0xBADF00D};
    EXPECT_CALL(msys, fstat).Times(2);
    EXPECT_CALL(msys, fcntl).Times(2);
    EXPECT_CALL(mlibmounthelper, getMountInfo).Times(2);
    ASSERT_NE(Context<DriverState>::get()->registerFile(fd), nullptr);
    ASSERT_THROW(Context<DriverState>::get()->registerFile(fd), FileAlreadyRegistered);
}

TEST_F(RocFileHandle, register_handle_linux_fd)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    rocFileHandle_t fh{};
    rocFileDescr_t  rfd{};

    rfd.type      = rocFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    EXPECT_CALL(mlibmounthelper, getMountInfo);
    ASSERT_EQ(rocFileHandleRegister(&fh, &rfd), ROCFILE_SUCCESS);
    ASSERT_NE(fh, nullptr);
}

// If the fstat() fails during file registration return rocfileInternalError
TEST_F(RocFileHandle, RocfileHandleRegisterFstatError)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    rocFileHandle_t fh{};
    rocFileDescr_t  rfd{};
    rfd.type      = rocFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    EXPECT_CALL(msys, fstat).WillOnce(Throw(Sys::RuntimeError(EBADF)));

    ASSERT_EQ(rocFileHandleRegister(&fh, &rfd), RocFileOpError(rocFileInternalError));
}

// If the fcntl() fails during file registration return rocfileInternalError
TEST_F(RocFileHandle, RocfileHandleRegisterFcntlError)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    rocFileHandle_t fh{};
    rocFileDescr_t  rfd{};
    rfd.type      = rocFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl).WillOnce(Throw(Sys::RuntimeError(EBADF)));

    ASSERT_EQ(rocFileHandleRegister(&fh, &rfd), RocFileOpError(rocFileInternalError));
}

// If getting mount information fails during file registration return rocfileInternalError
TEST_F(RocFileHandle, RocfileHandleRegisterLibMountError)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    rocFileHandle_t fh{};
    rocFileDescr_t  rfd{};
    rfd.type      = rocFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    EXPECT_CALL(mlibmounthelper, getMountInfo).WillOnce(Throw(std::runtime_error("error from test")));

    ASSERT_EQ(rocFileHandleRegister(&fh, &rfd), RocFileOpError(rocFileInternalError));
}

TEST_F(RocFileHandle, register_handle_linux_fd_already_registered)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    rocFileHandle_t fh{};
    rocFileDescr_t  rfd{};

    rfd.type      = rocFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    EXPECT_CALL(msys, fstat).Times(2);
    EXPECT_CALL(msys, fcntl).Times(2);
    EXPECT_CALL(mlibmounthelper, getMountInfo).Times(2);
    ASSERT_EQ(rocFileHandleRegister(&fh, &rfd), ROCFILE_SUCCESS);
    ASSERT_NE(fh, nullptr);
    ASSERT_EQ(rocFileHandleRegister(&fh, &rfd), RocFileOpError(rocFileHandleAlreadyRegistered));
}

TEST_F(RocFileHandle, register_handle_windows_handle_not_supported)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    rocFileHandle_t fh{};
    rocFileDescr_t  rfd{};

    rfd.type      = rocFileHandleTypeOpaqueWin32;
    rfd.handle.fd = 0xBADF00D;

    ASSERT_EQ(rocFileHandleRegister(&fh, &rfd), RocFileOpError(rocFileIONotSupported));
}

TEST_F(RocFileHandle, register_handle_userspace_fs_not_supported)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    rocFileHandle_t fh{};
    rocFileDescr_t  rfd{};

    rfd.type      = rocFileHandleTypeUserspaceFS;
    rfd.handle.fd = 0xBADF00D;

    ASSERT_EQ(rocFileHandleRegister(&fh, &rfd), RocFileOpError(rocFileIONotSupported));
}

TEST_F(RocFileHandle, deregister_handle_internal_throws_if_not_registered)
{
    ASSERT_THROW(Context<DriverState>::get()->deregisterFile(reinterpret_cast<rocFileHandle_t>(0xdeadbeef)),
                 FileNotRegistered);
}

TEST_F(RocFileHandle, deregister_handle_returns_error_if_not_registered)
{
    ASSERT_EQ(rocFileHandleDeregister(reinterpret_cast<rocFileHandle_t>(0xdeadbeef)),
              RocFileOpError(rocFileHandleNotRegistered));
}

TEST_F(RocFileHandle, deregister_handle_internal)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;
    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    EXPECT_CALL(mlibmounthelper, getMountInfo);
    auto fh = Context<DriverState>::get()->registerFile(0xBADF00D);
    Context<DriverState>::get()->deregisterFile(fh);
    ASSERT_THROW(Context<DriverState>::get()->deregisterFile(fh), FileNotRegistered);
}

TEST_F(RocFileHandle, deregister_handle)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    rocFileHandle_t fh{};
    rocFileDescr_t  rfd{};

    rfd.type      = rocFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    EXPECT_CALL(mlibmounthelper, getMountInfo);
    ASSERT_EQ(rocFileHandleRegister(&fh, &rfd), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileHandleDeregister(fh), ROCFILE_SUCCESS);
    ASSERT_EQ(rocFileHandleDeregister(fh), RocFileOpError(rocFileHandleNotRegistered));
}

TEST_F(RocFileHandle, deregister_handle_internal_fails_when_operations_are_oustanding)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    EXPECT_CALL(mlibmounthelper, getMountInfo);
    auto fh = Context<DriverState>::get()->registerFile(0xBADF00D);
    {
        auto file = Context<DriverState>::get()->getFile(fh);
        ASSERT_THROW(Context<DriverState>::get()->deregisterFile(fh), FileOperationsOutstanding);
    }
    Context<DriverState>::get()->deregisterFile(fh);
}

TEST_F(RocFileHandle, deregister_handle_fails_when_operations_are_oustanding)
{
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;

    rocFileHandle_t fh{};
    rocFileDescr_t  rfd{};

    rfd.type      = rocFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    EXPECT_CALL(msys, fstat);
    EXPECT_CALL(msys, fcntl);
    EXPECT_CALL(mlibmounthelper, getMountInfo);
    ASSERT_EQ(rocFileHandleRegister(&fh, &rfd), ROCFILE_SUCCESS);
    {
        auto file = Context<DriverState>::get()->getFile(fh);
        ASSERT_EQ(rocFileHandleDeregister(fh), RocFileOpError(rocFileInternalError));
    }
    ASSERT_EQ(rocFileHandleDeregister(fh), ROCFILE_SUCCESS);
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
