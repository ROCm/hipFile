/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "context.h"
#include "file-descriptor.h"
#include "sys.h"

using namespace hipFile;

FileDescriptor::FileDescriptor(int fd) : m_fd(fd)
{
}

FileDescriptor::~FileDescriptor()
{
    if (m_fd != -1) {
        Context<Sys>::get()->close(m_fd);
    }
}

FileDescriptor::FileDescriptor(FileDescriptor &&other) noexcept : m_fd(other.m_fd)
{
    other.m_fd = -1;
}

FileDescriptor &
FileDescriptor::operator=(FileDescriptor &&other) noexcept
{
    if (this != &other) {
        if (m_fd != -1) {
            Context<Sys>::get()->close(m_fd);
        }
        m_fd       = other.m_fd;
        other.m_fd = -1;
    }
    return *this;
}

int
FileDescriptor::get() const
{
    return m_fd;
}
