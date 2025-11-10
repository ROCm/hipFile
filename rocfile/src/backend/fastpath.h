/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "backend.h"
#include "hipfile-types.h"

#include <memory>
#include <sys/types.h>

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

    int score(std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size, hoff_t file_offset,
              hoff_t buffer_offset) const override;

    ssize_t io(IoType type, std::shared_ptr<IFile> file, std::shared_ptr<IBuffer> buffer, size_t size,
               hoff_t file_offset, hoff_t buffer_offset) override;
};

}
