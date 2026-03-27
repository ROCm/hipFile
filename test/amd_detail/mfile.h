/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "file.h"
#include "hipfile.h"

#include <gmock/gmock.h>

/*
 * A place to create mocks for the file module.
 */

namespace hipFile {

class MFile : public IFile {
public:
    MOCK_METHOD(hipFileHandle_t, getHandle, (), (const, override));
    MOCK_METHOD(int, getClientFd, (), (const, override));
    MOCK_METHOD(int, getBufferedFd, (), (const, override));
    MOCK_METHOD(std::optional<int>, getUnbufferedFd, (), (const, override));
    MOCK_METHOD(const struct statx &, getStatx, (), (const, noexcept, override));
    MOCK_METHOD(int, getStatusFlags, (), (const, override));
    MOCK_METHOD(std::optional<MountInfo>, getMountInfo, (), (const, override));
    MOCK_METHOD(uint32_t, dioMemAlign, (), (const, noexcept, override));
    MOCK_METHOD(uint32_t, dioOffsetAlign, (), (const, noexcept, override));
    MOCK_METHOD(bool, isBlockDevice, (), (const, noexcept, override));
    MOCK_METHOD(bool, isRegularFile, (), (const, noexcept, override));
};

class MFileMap : public FileMap {
public:
    MFileMap()
    {
    }
    MOCK_METHOD(hipFileHandle_t, registerFile, (UnregisteredFile && uf), (override));
    MOCK_METHOD(void, deregisterFile, (hipFileHandle_t fh), (override));
    MOCK_METHOD(std::shared_ptr<IFile>, getFile, (hipFileHandle_t), (override));
    MOCK_METHOD(void, clear, (), (override));
};

}
