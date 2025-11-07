/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "backend.h"

#include <sys/types.h>
#include <memory>

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

struct Fastpath : public Backend {
    virtual ~Fastpath() override = default;

    int score(std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size, off_t file_offset,
              off_t buffer_offset) const override;

    ssize_t io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size,
               off_t file_offset, off_t buffer_offset) override;
};

}
