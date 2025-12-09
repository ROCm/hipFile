/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "file.h"
#include "hipfile.h"
#include "hipfile-test.h"
#include "hipfile-warnings.h"
#include "msys.h"
#include "mmountinfo.h"
#include "mountinfo.h"
#include "state.h"
#include "sys.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <linux/stat.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <sys/eventfd.h>

using namespace hipFile;
using namespace testing;
using namespace std;

// Put tests inside the macros to suppress the global constructor
// warnings
HIPFILE_WARN_NO_GLOBAL_CTOR_OFF

struct ExpectUnregisteredFile;

struct ExpectUnregisteredFileBuilder {
    MSys                  &m_msys;
    MLibMountHelper       &m_mlibmounthelper;
    optional<struct statx> m_statx;
    optional<int>          m_fcntl_flags;
    optional<MountInfo>    m_mountinfo;
    optional<int>          m_fd_buffered;
    optional<int>          m_fd_unbuffered;

    ExpectUnregisteredFileBuilder(MSys &msys, MLibMountHelper &mlibmounthelper)
        : m_msys(msys), m_mlibmounthelper(mlibmounthelper)
    {
    }

    ExpectUnregisteredFileBuilder &statx(const struct statx &statxbuf)
    {
        m_statx = statxbuf;
        return *this;
    }

    ExpectUnregisteredFileBuilder &fcntl_flags(int flags)
    {
        m_fcntl_flags = flags;
        return *this;
    }

    ExpectUnregisteredFileBuilder &mountinfo(const MountInfo &mountinfo)
    {
        m_mountinfo = mountinfo;
        return *this;
    }

    ExpectUnregisteredFileBuilder &fd_buffered(int fd_buffered)
    {
        m_fd_buffered = fd_buffered;
        return *this;
    }

    ExpectUnregisteredFileBuilder &fd_unbuffered(int fd_unbuffered)
    {
        m_fd_unbuffered = fd_unbuffered;
        return *this;
    }

    ExpectUnregisteredFile build();
};

struct ExpectUnregisteredFile {
    ExpectUnregisteredFile(const ExpectUnregisteredFileBuilder &builder)
    {
        if (builder.m_statx) {
            EXPECT_CALL(builder.m_msys, statx).WillOnce(Return(builder.m_statx.value()));
        }
        else {
            EXPECT_CALL(builder.m_msys, statx);
        }

        if (builder.m_fcntl_flags) {
            EXPECT_CALL(builder.m_msys, fcntl(_, F_GETFL, 0)).WillOnce(Return(builder.m_fcntl_flags.value()));
        }
        else {
            EXPECT_CALL(builder.m_msys, fcntl(_, F_GETFL, 0));
        }

        if (builder.m_mountinfo) {
            EXPECT_CALL(builder.m_mlibmounthelper, getMountInfo)
                .WillOnce(Return(builder.m_mountinfo.value()));
        }
        else {
            EXPECT_CALL(builder.m_mlibmounthelper, getMountInfo);
        }

        EXPECT_CALL(builder.m_msys, fcntl(_, F_DUPFD_CLOEXEC, _))
            .Times(2)
            .WillOnce(Return(builder.m_fd_buffered ? builder.m_fd_buffered.value() : -1))
            .WillOnce(Return(builder.m_fd_unbuffered ? builder.m_fd_unbuffered.value() : -1));

        EXPECT_CALL(builder.m_msys, fcntl(_, F_SETFL, _)).Times(1);
    }
};

ExpectUnregisteredFile
ExpectUnregisteredFileBuilder::build()
{
    return ExpectUnregisteredFile(*this);
}

void
expect_file_registration(MSys &msys, MLibMountHelper &mlibmounthelper)
{
    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
}

struct HipFileHandle : public HipFileOpened {
    StrictMock<MSys>            msys;
    StrictMock<MLibMountHelper> mlibmounthelper;
};

TEST_F(HipFileHandle, register_handle_internal_linux_fd)
{
    int fd{0xBADF00D};

    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
    ASSERT_NE(Context<DriverState>::get()->registerFile(fd), nullptr);
}

TEST_F(HipFileHandle, file_initialization)
{
    int          fd{0x12345678};
    int          status_flags{0x789ABCDE};
    struct statx stxbuf;
    memset(&stxbuf, 0xA5, sizeof(stxbuf));

    // FileDescriptor will call close() on file.fd_buffered and file.fd_unbuffered
    // _after_ StrictMock<Sys> is destroyed. Return eventfds when the client
    // fd is duplicated so that Sys::close() has a valid file descriptor to
    // close.
    int fd_buffered{eventfd(0, 0)};
    ASSERT_NE(fd_buffered, -1);
    int fd_unbuffered{eventfd(0, 0)};
    ASSERT_NE(fd_unbuffered, -1);

    MountInfo mountinfo;
    mountinfo.type                         = FilesystemType::ext4;
    mountinfo.options.ext4.journaling_mode = ExtJournalingMode::ordered;

    ExpectUnregisteredFileBuilder(msys, mlibmounthelper)
        .statx(stxbuf)
        .fcntl_flags(status_flags)
        .mountinfo(mountinfo)
        .fd_buffered(fd_buffered)
        .fd_unbuffered(fd_unbuffered)
        .build();
    auto fh{Context<DriverState>::get()->registerFile(fd)};
    auto file{Context<DriverState>::get()->getFile(fh)};

    EXPECT_EQ(fh, file->getHandle());
    EXPECT_EQ(fd, file->getFd());
    auto file_stx{file->getStatx()};
    EXPECT_EQ(0, memcmp(&file_stx, &stxbuf, sizeof(stxbuf)));
    EXPECT_EQ(status_flags, file->getStatusFlags());
    EXPECT_EQ(mountinfo.type, file->getMountInfo().value().type);
    EXPECT_EQ(mountinfo.options.ext4.journaling_mode,
              file->getMountInfo().value().options.ext4.journaling_mode);
    EXPECT_EQ(fd_buffered, file->getFdBuffered());
    EXPECT_EQ(fd_unbuffered, file->getFdUnbuffered());
}

TEST_F(HipFileHandle, register_handle_internal_linux_fd_already_registered)
{
    int fd{0xBADF00D};
    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
    ASSERT_NE(Context<DriverState>::get()->registerFile(fd), nullptr);
    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
    ASSERT_THROW(Context<DriverState>::get()->registerFile(fd), FileAlreadyRegistered);
}

TEST_F(HipFileHandle, register_handle_linux_fd)
{
    hipFileHandle_t fh{};
    hipFileDescr_t  rfd{};

    rfd.type      = hipFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
    ASSERT_EQ(hipFileHandleRegister(&fh, &rfd), HIPFILE_SUCCESS);
    ASSERT_NE(fh, nullptr);
}

// If statx() fails during file registration return hipfileInternalError
TEST_F(HipFileHandle, RocfileHandleRegisterStatxError)
{
    hipFileHandle_t fh{};
    hipFileDescr_t  rfd{};
    rfd.type      = hipFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    EXPECT_CALL(msys, statx).WillOnce(Throw(Sys::RuntimeError(EBADF)));

    ASSERT_EQ(hipFileHandleRegister(&fh, &rfd), HipFileOpError(hipFileInternalError));
}

// If the fcntl() fails during file registration return hipfileInternalError
TEST_F(HipFileHandle, RocfileHandleRegisterFcntlError)
{
    hipFileHandle_t fh{};
    hipFileDescr_t  rfd{};
    rfd.type      = hipFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    EXPECT_CALL(msys, statx);
    EXPECT_CALL(msys, fcntl).WillOnce(Throw(Sys::RuntimeError(EBADF)));

    ASSERT_EQ(hipFileHandleRegister(&fh, &rfd), HipFileOpError(hipFileInternalError));
}

// If getting mount information fails during file registration return hipfileInternalError
TEST_F(HipFileHandle, RocfileHandleRegisterLibMountError)
{
    hipFileHandle_t fh{};
    hipFileDescr_t  rfd{};
    rfd.type      = hipFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    EXPECT_CALL(msys, statx);
    EXPECT_CALL(msys, fcntl);
    EXPECT_CALL(mlibmounthelper, getMountInfo).WillOnce(Throw(std::runtime_error("error from test")));

    ASSERT_EQ(hipFileHandleRegister(&fh, &rfd), HipFileOpError(hipFileInternalError));
}

TEST_F(HipFileHandle, register_handle_linux_fd_already_registered)
{
    hipFileHandle_t fh{};
    hipFileDescr_t  rfd{};

    rfd.type      = hipFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
    ASSERT_EQ(hipFileHandleRegister(&fh, &rfd), HIPFILE_SUCCESS);
    ASSERT_NE(fh, nullptr);
    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
    ASSERT_EQ(hipFileHandleRegister(&fh, &rfd), HipFileOpError(hipFileHandleAlreadyRegistered));
}

TEST_F(HipFileHandle, register_handle_windows_handle_not_supported)
{
    hipFileHandle_t fh{};
    hipFileDescr_t  rfd{};

    rfd.type      = hipFileHandleTypeOpaqueWin32;
    rfd.handle.fd = 0xBADF00D;

    ASSERT_EQ(hipFileHandleRegister(&fh, &rfd), HipFileOpError(hipFileIONotSupported));
}

TEST_F(HipFileHandle, register_handle_userspace_fs_not_supported)
{
    hipFileHandle_t fh{};
    hipFileDescr_t  rfd{};

    rfd.type      = hipFileHandleTypeUserspaceFS;
    rfd.handle.fd = 0xBADF00D;

    ASSERT_EQ(hipFileHandleRegister(&fh, &rfd), HipFileOpError(hipFileIONotSupported));
}

TEST_F(HipFileHandle, deregister_handle_internal_throws_if_not_registered)
{
    ASSERT_THROW(Context<DriverState>::get()->deregisterFile(reinterpret_cast<hipFileHandle_t>(0xdeadbeef)),
                 FileNotRegistered);
}

TEST_F(HipFileHandle, deregister_handle_doesnt_throw_if_not_registered)
{
    hipFileHandleDeregister(reinterpret_cast<hipFileHandle_t>(0xdeadbeef));
}

TEST_F(HipFileHandle, deregister_handle_internal)
{
    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
    auto fh = Context<DriverState>::get()->registerFile(0xBADF00D);
    Context<DriverState>::get()->deregisterFile(fh);
    ASSERT_THROW(Context<DriverState>::get()->deregisterFile(fh), FileNotRegistered);
}

TEST_F(HipFileHandle, deregister_handle)
{
    hipFileHandle_t fh{};
    hipFileDescr_t  rfd{};

    rfd.type      = hipFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
    ASSERT_EQ(hipFileHandleRegister(&fh, &rfd), HIPFILE_SUCCESS);
    hipFileHandleDeregister(fh);
    hipFileHandleDeregister(fh);
}

TEST_F(HipFileHandle, deregister_handle_internal_fails_when_operations_are_oustanding)
{
    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
    auto fh = Context<DriverState>::get()->registerFile(0xBADF00D);
    {
        auto file = Context<DriverState>::get()->getFile(fh);
        ASSERT_THROW(Context<DriverState>::get()->deregisterFile(fh), FileOperationsOutstanding);
    }
    Context<DriverState>::get()->deregisterFile(fh);
}

TEST_F(HipFileHandle, deregister_handle_fails_when_operations_are_oustanding)
{
    hipFileHandle_t fh{};
    hipFileDescr_t  rfd{};

    rfd.type      = hipFileHandleTypeOpaqueFD;
    rfd.handle.fd = 0xBADF00D;

    ExpectUnregisteredFileBuilder(msys, mlibmounthelper).build();
    ASSERT_EQ(hipFileHandleRegister(&fh, &rfd), HIPFILE_SUCCESS);
    {
        auto file = Context<DriverState>::get()->getFile(fh);
        hipFileHandleDeregister(fh);
    }
    hipFileHandleDeregister(fh);
}

TEST_F(HipFileHandle, UnregisteredFileUnsetsODirectOnFdBufferedWhenInstantiatedWithUnbufferedFile)
{
    int           fd{777777};
    int           fd_buffered{888888};
    int           fd_unbuffered{999999};
    unsigned long fd_flags{~0UL}; // O_DIRECT flag (and all other flags) set

    // Expectations for construction of UnregisteredFile
    EXPECT_CALL(msys, statx);
    EXPECT_CALL(msys, fcntl(fd, F_GETFL, 0)).WillOnce(Return(fd_flags));
    EXPECT_CALL(mlibmounthelper, getMountInfo);
    EXPECT_CALL(msys, fcntl(fd, F_DUPFD_CLOEXEC, 0))
        .Times(2)
        .WillOnce(Return(fd_buffered))
        .WillOnce(Return(fd_unbuffered));
    EXPECT_CALL(msys, fcntl(fd_buffered, F_SETFL, fd_flags & ~static_cast<unsigned long>(O_DIRECT))).Times(1);

    // Expectations for destruction of UnregisteredFile
    EXPECT_CALL(msys, close(fd_buffered));
    EXPECT_CALL(msys, close(fd_unbuffered));

    UnregisteredFile uf{fd};
}

TEST_F(HipFileHandle, UnregisteredFileSetsODirectOnFdUnbufferedWhenInstantiatedWithBufferedFile)
{
    int           fd{777777};
    int           fd_buffered{888888};
    int           fd_unbuffered{999999};
    unsigned long fd_flags{~static_cast<unsigned long>(O_DIRECT)}; // All flags other than O_DIRECT are set

    // Expectations for construction of UnregisteredFile
    EXPECT_CALL(msys, statx);
    EXPECT_CALL(msys, fcntl(fd, F_GETFL, 0)).WillOnce(Return(fd_flags));
    EXPECT_CALL(mlibmounthelper, getMountInfo);
    EXPECT_CALL(msys, fcntl(fd, F_DUPFD_CLOEXEC, 0))
        .Times(2)
        .WillOnce(Return(fd_buffered))
        .WillOnce(Return(fd_unbuffered));
    EXPECT_CALL(msys, fcntl(fd_unbuffered, F_SETFL, ~0UL)).Times(1);

    // Expectations for destruction of UnregisteredFile
    EXPECT_CALL(msys, close(fd_buffered));
    EXPECT_CALL(msys, close(fd_unbuffered));

    UnregisteredFile uf{fd};
}

TEST_F(HipFileHandle, UnregistgeredFileClosesDupedFileDescriptorsWhenDestroyed)
{
    int fd_buffered{0x0D06F00D};
    int fd_unbuffered{0x0BADD00D};
    ExpectUnregisteredFileBuilder(msys, mlibmounthelper)
        .fd_buffered(fd_buffered)
        .fd_unbuffered(fd_unbuffered)
        .build();
    EXPECT_CALL(msys, close(fd_buffered));
    EXPECT_CALL(msys, close(fd_unbuffered));
    UnregisteredFile(97);
}

TEST_F(HipFileHandle, FileClosesDupedFileDescriptorsWhenDestroyed)
{
    int fd{0xABBA};
    int fd_buffered{0x0D06F00D};
    int fd_unbuffered{0x0BADF00D};

    // To get a File we need to go through the registration process which
    // creates and consumes an UnregisteredFile to create a file
    ExpectUnregisteredFileBuilder(msys, mlibmounthelper)
        .fd_buffered(fd_buffered)
        .fd_unbuffered(fd_unbuffered)
        .build();

    // FileDescriptors managing fd_buffered and fd_unbuffered are moved from
    // UnregisteredFile to File.  The file descriptors should not be closed when
    // they are transferred
    auto file_handle{Context<DriverState>::get()->registerFile(UnregisteredFile{fd})};

    // Now that file owns fd_buffered and fd_unbuffered, they should be closed
    // when file is unregistered and ultimately destroyed
    EXPECT_CALL(msys, close(fd_buffered));
    EXPECT_CALL(msys, close(fd_unbuffered));
    Context<DriverState>::get()->deregisterFile(file_handle);
}

HIPFILE_WARN_NO_GLOBAL_CTOR_ON
