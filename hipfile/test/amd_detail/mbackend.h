/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "backend.h"

#include <gmock/gmock.h>

namespace hipFile {

struct MBackend : Backend {
    MOCK_METHOD(int, score, (std::shared_ptr<IFile>, std::shared_ptr<IBuffer>, size_t, hoff_t, hoff_t),
                (const, override));
    MOCK_METHOD(ssize_t, io,
                (hipFile::IoType type, std::shared_ptr<IFile>, std::shared_ptr<IBuffer>, size_t, hoff_t,
                 hoff_t),
                (override));
};

}
