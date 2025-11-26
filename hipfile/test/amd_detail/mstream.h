/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "stream.h"

namespace hipFile {

class MStream : public IStream {
public:
    MOCK_METHOD(hipStream_t, getHipStream, (), (const override));
    MOCK_METHOD(bool, fixedBufferOffset, (), (const override));
    MOCK_METHOD(bool, fixedFileOffset, (), (const override));
    MOCK_METHOD(bool, fixedIOSize, (), (const override));
    MOCK_METHOD(bool, pageAligned, (), (const override));
};

}
