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
    MOCK_METHOD(hipFileHandle_t, getHandle, (), (const override));
    MOCK_METHOD(int, getFd, (), (const override));
    MOCK_METHOD(const struct statx &, getStatx, (), (const, noexcept, override));
    MOCK_METHOD(int, getStatusFlags, (), (const override));
    MOCK_METHOD(std::optional<MountInfo>, getMountInfo, (), (const override));
};

class MFileMap : public FileMap {
public:
    MFileMap()
    {
    }
    MOCK_METHOD(hipFileHandle_t, registerFile, (const UnregisteredFile &uf), (override));
    MOCK_METHOD(void, deregisterFile, (hipFileHandle_t fh), (override));
    MOCK_METHOD(std::shared_ptr<IFile>, getFile, (hipFileHandle_t), (override));
    MOCK_METHOD(void, clear, (), (override));
};

}
