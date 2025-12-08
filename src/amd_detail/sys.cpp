/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "sys.h"

#include <bits/statx-generic.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <syslog.h>
#include <unistd.h>

namespace hipFile {

template <typename ExceptionType, typename L, typename R>
static inline R
throwOn(const L throw_value, R value)
{
    if (throw_value == value) {
        throw ExceptionType();
    }
    return value;
}

int
Sys::open(const char *pathname, int flags)
{
    return throwOn<Sys::RuntimeError>(-1, ::open(pathname, flags));
}

int
Sys::open(const char *pathname, int flags, mode_t mode)
{
    return throwOn<Sys::RuntimeError>(-1, ::open(pathname, flags, mode));
}

void
Sys::close(int fd) const
{
    throwOn<Sys::RuntimeError>(-1, ::close(fd));
}

void *
Sys::mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) const
{
    return throwOn<Sys::RuntimeError>(reinterpret_cast<void *>(-1),
                                      ::mmap(addr, length, prot, flags, fd, offset));
}

void
Sys::munmap(void *addr, size_t length) const
{
    throwOn<Sys::RuntimeError>(-1, ::munmap(addr, length));
}

ssize_t
Sys::pread(int fd, void *buf, size_t count, off_t offset) const
{
    return throwOn<Sys::RuntimeError>(-1, ::pread(fd, buf, count, offset));
}

ssize_t
Sys::pwrite(int fd, void *buf, size_t count, off_t offset) const
{
    return throwOn<Sys::RuntimeError>(-1, ::pwrite(fd, buf, count, offset));
}

ssize_t
Sys::readlink(const char *pathname, char *buf, size_t bufsize)
{
    return throwOn<Sys::RuntimeError>(-1, ::readlink(pathname, buf, bufsize));
}

void
Sys::syslog(int priority, const char *msg) const
{
    ::syslog(priority, "%s", msg);
}

struct stat
Sys::fstat(int fd) const
{
    struct stat statbuf;

    throwOn<Sys::RuntimeError>(-1, ::fstat(fd, &statbuf));

    return statbuf;
}

int
Sys::fcntl(int fd, int op, uintptr_t arg) const
{
    return throwOn<Sys::RuntimeError>(-1, ::fcntl(fd, op, arg));
}

struct statx
Sys::statx(int dirfd, const char *pathname, int flags, unsigned int mask) const
{
    struct statx statxbuf;
    throwOn<Sys::RuntimeError>(-1, ::statx(dirfd, pathname, flags, mask, &statxbuf));
    return statxbuf;
}

char *
Sys::getenv(const char *name) const noexcept
{
    return ::getenv(name);
}

}
