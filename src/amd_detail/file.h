/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "file-descriptor.h"
#include "hipfile.h"
#include "mountinfo.h"

#include <linux/stat.h>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>

template <typename T> class PassKey;

namespace hipFile {

/// @brief File is not registered
struct FileNotRegistered : public std::runtime_error {
    FileNotRegistered() : std::runtime_error("File not registered")
    {
    }
};

/// @brief File is already registered
struct FileAlreadyRegistered : public std::runtime_error {
    FileAlreadyRegistered() : std::runtime_error("File already registered")
    {
    }
};

/// @brief File has operations outstanding
struct FileOperationsOutstanding : public std::runtime_error {
    FileOperationsOutstanding() : std::runtime_error("File operations outstanding")
    {
    }
};

class UnregisteredFile {
public:
    /// @brief Construct an unregistered file
    ///
    /// During construction of an unregistered file, information about the file
    /// is collected from the system.
    ///
    /// @param fd A valid file descriptor
    UnregisteredFile(int fd);

    /// @brief The file descriptor provided by the client
    FileDescriptor client_fd;

    /// @brief Buffered file descriptor (!O_DIRECT)
    FileDescriptor buffered_fd;

    /// @brief Unbuffered file descriptor (O_DIRECT)
    std::optional<FileDescriptor> unbuffered_fd;

    /// @brief Information provided by statx (2)
    struct statx stx;

    /// @brief Flags provided by fcntl(2)
    int flags;

    /// @brief Information obtained from /proc/self/mountinfo
    std::optional<MountInfo> mountinfo;

    /// @brief Memory alignment (in bytes) requirement for direct IO. If the file does not support direct IO,
    /// this will be 0. If alignment information is not available from statx, this will be set to 4096.
    uint32_t m_dio_mem_align;

    /// @brief Alignment (in bytes) required for file offsets and IO segment lengths for direct IO. If the
    /// file does not support direct IO, this will be 0. If alignment information is not available from statx,
    /// this will be set to 4096.
    uint32_t m_dio_offset_align;
};

class IFile {
public:
    virtual ~IFile() = default;

    /// @brief Get the handle for this file
    /// @return The handle for this file
    virtual hipFileHandle_t getHandle() const;

    virtual int                      getClientFd() const             = 0;
    virtual int                      getBufferedFd() const           = 0;
    virtual std::optional<int>       getUnbufferedFd() const         = 0;
    virtual const struct statx      &getStatx() const noexcept       = 0;
    virtual int                      getStatusFlags() const          = 0;
    virtual std::optional<MountInfo> getMountInfo() const            = 0;
    virtual uint32_t                 dioMemAlign() const noexcept    = 0;
    virtual uint32_t                 dioOffsetAlign() const noexcept = 0;
    virtual bool                     isBlockDevice() const noexcept  = 0;
    virtual bool                     isRegularFile() const noexcept  = 0;
};

class FileMap;

class File : public IFile {

public:
    virtual ~File() override = default;

    // Don't allow copying
    File(const File &)            = delete;
    File &operator=(const File &) = delete;

    // Don't allow moving
    File(File &&)            = delete;
    File &operator=(File &&) = delete;

    virtual int                      getClientFd() const override;
    virtual int                      getBufferedFd() const override;
    virtual std::optional<int>       getUnbufferedFd() const override;
    virtual const struct statx      &getStatx() const noexcept override;
    virtual int                      getStatusFlags() const override;
    virtual std::optional<MountInfo> getMountInfo() const override;

    /// @brief Get the memory (in bytes) alignment requirement for direct IO on this file. If the file does
    /// not support direct IO, this will return 0.
    /// @return The memory alignment (in bytes) requirement for direct IO, 0 if direct IO is not supported
    virtual uint32_t dioMemAlign() const noexcept override;

    /// @brief The alignment (in bytes) required for file offsets and I/O segment lengths for direct I/O
    /// (O_DIRECT) on this file, or 0 if direct I/O is not supported on this file
    /// @return The alignment (in bytes) required for file offsets and I/O segment lengths for direct I/O, 0
    /// if direct I/O is not supported
    virtual uint32_t dioOffsetAlign() const noexcept override;

    /// @brief Whether this file is a block device. If statx did not return type information, this will return
    /// false.
    /// @return True if the file is a block device, false otherwise
    virtual bool isBlockDevice() const noexcept override;

    /// @brief Whether this file is a regular file. If statx did not return type information, this will return
    /// false.
    /// @return True if the file is a regular file, false otherwise
    virtual bool isRegularFile() const noexcept override;

    /// @brief Construct a registered file
    /// @param uf An unregistered file
    /// @param k  Key class instance (see passkey.h)
    File(UnregisteredFile &&uf, const PassKey<FileMap> &k);

private:
    /// @brief The file descriptor provided by the client
    FileDescriptor client_fd;

    /// @brief Buffered file descriptor (!O_DIRECT)
    FileDescriptor buffered_fd;

    /// @brief Unbuffered file descriptor (O_DIRECT)
    std::optional<FileDescriptor> unbuffered_fd;

    /// @brief File status information obtained from statx (2)
    struct statx stx;

    /// @brief The file's status flags. See fcntl(2)
    ///
    /// Used to determine if the O_DIRECT flag is set
    int status_flags;

    /// @brief Mount information for the filesystem backing fd
    std::optional<MountInfo> mountinfo;

    /// @brief Memory alignment (in bytes) requirement for direct IO. If the file does not support direct IO,
    /// this will be 0.
    uint32_t m_dio_mem_align;

    /// @brief Alignment (in bytes) required for file offsets and I/O segment lengths for direct I/O
    /// (O_DIRECT).
    uint32_t m_dio_offset_align;

    /// @brief Whether statx reported that the file is a block device
    bool m_is_block_device;

    /// @brief Whether statx reported that the file is a regular file
    bool m_is_regular_file;
};

class FileMap {
public:
    virtual ~FileMap();

    /// @brief Registers a file. Files must be registered before they can be used with hipFile IO APIs
    /// @attention A unique_lock on HipFileMutex must be held
    /// @param uf An unregistered file
    virtual hipFileHandle_t registerFile(UnregisteredFile &&uf);

    /// @brief Deregisters the file associated with the provided file handle
    /// @attention A unique_lock on HipFileMutex must be held
    /// @param fh The handle of the file to deregister
    virtual void deregisterFile(hipFileHandle_t fh);

    /// @brief Look up a file given a hipFileHandle_t
    /// @attention A shared_lock on HipFileMutex must be held
    /// @param fh The file handle to lookup the file with
    /// @return If file handle is valid, return a shared pointer to the file, otherwise throw NotRegistered.
    virtual std::shared_ptr<IFile> getFile(hipFileHandle_t fh);

    virtual void clear();

private:
    /// File lookup tables.
    std::unordered_map<int, std::shared_ptr<IFile>>             from_fd;
    std::unordered_map<hipFileHandle_t, std::shared_ptr<IFile>> from_fh;
};

}
