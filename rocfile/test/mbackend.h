/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "backend.h"

#include <gmock/gmock.h>

namespace rocFile::backend {

struct MBackend : Backend {
    MOCK_METHOD(int, score,
                (std::shared_ptr<file::IFile>, std::shared_ptr<buffer::IBuffer>, size_t, off_t, off_t),
                (const override));
    MOCK_METHOD(ssize_t, io,
                (rocFile::io::IoType type, std::shared_ptr<file::IFile>, std::shared_ptr<buffer::IBuffer>,
                 size_t, off_t, off_t),
                (override));
};

}
