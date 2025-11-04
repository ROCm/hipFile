/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "rocfile.h"
#include "mountinfo.h"

#include <memory>
#include <optional>
#include <shared_mutex>
#include <stdexcept>
#include <sys/stat.h>
#include <unordered_map>

namespace rocFile {

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

    /// @return Returns the file descriptor
    int getFd() const noexcept;

    /// @return Returns the information provided by fstat (2)
    struct stat getStat() const noexcept;

    /// @return Returns the flags provided by fcntl (2)
    int getFlags() const noexcept;

    /// @brief Returns information obtained from /proc/self/mountinfo
    std::optional<MountInfo> getMountInfo() const noexcept;

private:
    /// @brief The file descriptor
    int m_fd;

    /// @brief Information provided by fstat (2)
    struct stat m_stat;

    /// @brief Flags provided by fcntl(2)
    int m_flags;

    /// @brief Information obtained from /proc/self/mountinfo
    std::optional<MountInfo> m_mountinfo;
};

class IFile {
public:
    virtual ~IFile() = default;

    /// @brief Get the handle for this file
    /// @return The handle for this file
    virtual rocFileHandle_t getHandle() const;

    virtual int                      getFd() const          = 0;
    virtual dev_t                    getDevice() const      = 0;
    virtual mode_t                   getMode() const        = 0;
    virtual int                      getStatusFlags() const = 0;
    virtual std::optional<MountInfo> getMountInfo() const   = 0;
};

class File : public IFile {

public:
    virtual ~File() override = default;

    // Don't allow copying
    File(const File &)            = delete;
    File &operator=(const File &) = delete;

    // Don't allow moving
    File(File &&)            = delete;
    File &operator=(File &&) = delete;

    virtual int                      getFd() const override;
    virtual dev_t                    getDevice() const override;
    virtual mode_t                   getMode() const override;
    virtual int                      getStatusFlags() const override;
    virtual std::optional<MountInfo> getMountInfo() const override;

private:
    /// @brief Construct a registered file
    /// @param uf An unregistered file
    File(const UnregisteredFile &uf);

    /// @brief The file descriptor
    int fd;

    /// @brief The ID of the device containing file
    ///
    /// Used to lookup mount information. See fstat(2), inode(7), and proc_pid_mountinfo(5)
    dev_t device;

    /// @brief The file type and mode
    ///
    /// Specifies the file type (regular file, block device, ...) and mode. See fstat(2) and inode(7)
    mode_t mode;

    /// @brief The file's status flags. See fcntl(2)
    ///
    /// Used to determine if the O_DIRECT flag is set
    int status_flags;

    /// @brief Mount information for the filesystem backing fd
    std::optional<MountInfo> mountinfo;

    friend class FileMap;
};

class FileMap {
public:
    virtual ~FileMap();

    /// @brief Registers a file. Files must be registered before they can be used with rocFile IO APIs
    /// @attention A unique_lock on RocFileMutex must be held
    /// @param uf An unregistered file
    virtual rocFileHandle_t registerFile(const UnregisteredFile &uf);

    /// @brief Deregisters the file associated with the provided file handle
    /// @attention A unique_lock on RocFileMutex must be held
    /// @param fh The handle of the file to deregister
    virtual void deregisterFile(rocFileHandle_t fh);

    /// @brief Look up a file given a rocFileHandle_t
    /// @attention A shared_lock on RocFileMutex must be held
    /// @param fh The file handle to lookup the file with
    /// @return If file handle is valid, return a shared pointer to the file, otherwise throw NotRegistered.
    virtual std::shared_ptr<IFile> getFile(rocFileHandle_t fh);

    virtual void clear();

private:
    /// File lookup tables.
    std::unordered_map<int, std::shared_ptr<IFile>>             from_fd;
    std::unordered_map<rocFileHandle_t, std::shared_ptr<IFile>> from_fh;
};

}
