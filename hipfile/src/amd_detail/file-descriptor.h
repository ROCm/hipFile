/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

namespace hipFile {

/// @brief Manages the lifetime of a file descriptor
class FileDescriptor {
public:
    explicit FileDescriptor(int fd);

    ~FileDescriptor();

    FileDescriptor(const FileDescriptor &)            = delete;
    FileDescriptor &operator=(const FileDescriptor &) = delete;

    FileDescriptor(FileDescriptor &&other) noexcept;

    FileDescriptor &operator=(FileDescriptor &&other) noexcept;

    int get() const;

private:
    int m_fd;
};

}
