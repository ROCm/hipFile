/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "context.h"
#include "sys.h"

#include <gmock/gmock.h>

// Mock implementations for Sys. Enables unit tests to mock system calls.
namespace rocFile {

struct MSys : Sys {
    context::ContextOverride<Sys> co;
    MSys() : co{this}
    {
    }
    MOCK_METHOD(void *, mmap, (void *addr, size_t length, int prot, int flags, int fd, off_t offset),
                (const override));
    MOCK_METHOD(void, munmap, (void *addr, size_t length), (const override));

    MOCK_METHOD(ssize_t, pread, (int fd, void *buf, size_t count, off_t offset), (const override));
    MOCK_METHOD(ssize_t, pwrite, (int fd, void *buf, size_t count, off_t offset), (const override));

    MOCK_METHOD(void, syslog, (int priority, const char *msg), (const override));

    MOCK_METHOD(struct stat, fstat, (int fd), (const override));

    MOCK_METHOD(int, fcntl, (int fd, int op, uintptr_t arg), (const override));
};
}
