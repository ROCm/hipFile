/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "backend.h"
#include "file.h"
#include "io.h"

namespace rocFile {
class IBuffer;
}
namespace rocFile {
class IFile;
}
namespace rocFile {
enum class IoType;
}

namespace rocFile {

struct Fallback : public Backend {
    virtual ~Fallback() override = default;

    int score(std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size, hoff_t file_offset,
              hoff_t buffer_offset) const override;

    ssize_t io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size,
               hoff_t file_offset, hoff_t buffer_offset) override;

    // Once we can import gtest.h and make test suites or test friends everything
    // below here should be made protected.
    // protected:

    ssize_t io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size,
               hoff_t file_offset, hoff_t buffer_offset, size_t chunk_size);
};

}
