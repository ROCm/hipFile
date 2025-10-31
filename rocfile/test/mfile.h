/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "file.h"
#include "rocfile.h"

#include <gmock/gmock.h>

/*
 * A place to create mocks for the file module.
 */

namespace rocFile {

class MFile : public file::IFile {
public:
    MOCK_METHOD(rocFileHandle_t, getHandle, (), (const override));
    MOCK_METHOD(int, getFd, (), (const override));
    MOCK_METHOD(dev_t, getDevice, (), (const override));
    MOCK_METHOD(mode_t, getMode, (), (const override));
    MOCK_METHOD(int, getStatusFlags, (), (const override));
    MOCK_METHOD(std::optional<MountInfo>, getMountInfo, (), (const override));
};

class MFileMap : public file::FileMap {
public:
    MFileMap()
    {
    }
    MOCK_METHOD(rocFileHandle_t, registerFile,
                (int fd, struct stat &fstat, int _status_flags, std::optional<MountInfo> mountinfo),
                (override));
    MOCK_METHOD(void, deregisterFile, (rocFileHandle_t fh), (override));
    MOCK_METHOD(std::shared_ptr<file::IFile>, getFile, (rocFileHandle_t), (override));
    MOCK_METHOD(void, clear, (), (override));
};

}
