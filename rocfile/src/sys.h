/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* sys wraps system APIs used by rocFile which enables unit tests to mock system calls.
 *
 * The wrapper methods should
 *   - Throw a Sys::RuntimeError if the wrapped function fails
 *   - Use return values instead of out arguments when possible
 */

namespace rocFile {

struct Sys {
    virtual ~Sys() = default;

    virtual void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) const;
    virtual void  munmap(void *addr, size_t length) const;

    virtual ssize_t pread(int fd, void *buf, size_t count, off_t offset) const;
    virtual ssize_t pwrite(int fd, void *buf, size_t count, off_t offset) const;

    virtual void syslog(int priority, const char *msg) const;

    virtual struct stat fstat(int fd) const;

    virtual int fcntl(int fd, int op, uintptr_t arg) const;

    struct RuntimeError : public std::runtime_error {
        /// The value of errno when RuntimeError was thrown
        int error;

        RuntimeError() : RuntimeError(errno)
        {
        }

        // This constructor should be protected so that only unit tests can
        // access it. But we need to modify the cmake target macros to allow us
        // to include gtest.h
        RuntimeError(int _error) : std::runtime_error("System Error"), error(_error)
        {
        }
    };
};

}
