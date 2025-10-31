/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "hipfile.h"
#include "magic-word.h"

#include <cstdint>
#include <iostream>
#include <random>
#include <stdexcept>
#include <unistd.h>

constexpr hipFileError_t
HipFileOpError(hipFileOpError_t err)
{
    return {err, hipSuccess};
}

constexpr hipFileError_t
HipFileDriverError(hipError_t err)
{
    return {hipFileHipDriverError, err};
}

inline bool
operator==(const hipFileError_t &lhs, const hipFileError_t &rhs)
{
    return lhs.err == rhs.err && lhs.hip_drv_err == rhs.hip_drv_err;
}

static std::ostream &
operator<<(std::ostream &os, const hipFileError_t &hfe)
{
    return os << "hipFileError_t{ err: " << hfe.err << ", hip_drv_err: " << hfe.hip_drv_err << " }";
}

// Convenience "success" value
inline constexpr hipFileError_t HIPFILE_SUCCESS{hipFileSuccess, hipSuccess};

inline void
rfill(void *buffer, uint64_t len, uint32_t seed = 97)
{
    std::mt19937                            gen(seed);
    std::uniform_int_distribution<uint16_t> dist(0, 255);

    for (uint64_t i = 0; i < len; i++) {
        static_cast<uint8_t *>(buffer)[i] = static_cast<uint8_t>(dist(gen));
    }
}

struct Tmpfile {
    int fd;

    Tmpfile()
    {
        char path_template[] = "hipFile.XXXXXX";
        if ((fd = mkstemp(path_template)) == -1) {
            throw std::runtime_error("Could not create temporary file");
        }
        if (unlink(path_template) == -1) {
            throw std::runtime_error("Could not unlink temporary file)");
        }
    }

    ~Tmpfile()
    {
        close(fd);
    }
};
