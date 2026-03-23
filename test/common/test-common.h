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

// ON_CALL is not compatible with StrictMock's
#define MOCK_PASSTHROUGH(base_class, func) \
    ON_CALL(*this, func).WillByDefault( \
        [this](auto&&... args) { \
            return this->base_class::func(std::forward<decltype(args)>(args)...); \
        } \
    )

// Set a particular hipfile error
constexpr hipFileError_t
HipFileOpError(hipFileOpError_t err)
{
    return {err, hipSuccess};
}

// Set a particular HIP error
constexpr hipFileError_t
HipFileDriverError(hipError_t err)
{
    return {hipFileHipDriverError, err};
}

// == overload for hipFileError_t values
inline bool
operator==(const hipFileError_t &lhs, const hipFileError_t &rhs)
{
    return lhs.err == rhs.err && lhs.hip_drv_err == rhs.hip_drv_err;
}

// != overload for hipFileError_t values
inline bool
operator!=(const hipFileError_t &lhs, const hipFileError_t &rhs)
{
    return lhs.err != rhs.err || lhs.hip_drv_err != rhs.hip_drv_err;
}

// Unused in the test code, but kept here for iostream debugging
#ifndef NDEBUG
#include <iostream>
inline std::ostream &
operator<<(std::ostream &os, const hipFileError_t &hfe)
{
    return os << "hipFileError_t{ err: " << hfe.err << ", hip_drv_err: " << hfe.hip_drv_err << " }";
}
#endif

// Convenience "success" value
inline constexpr hipFileError_t HIPFILE_SUCCESS{hipFileSuccess, hipSuccess};

// Convenience "invalid argument" value
inline constexpr hipFileError_t HIPFILE_INVALID_VALUE{hipFileInvalidValue, hipSuccess};

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
    int         fd;
    std::string path;

    Tmpfile() : Tmpfile{"."}
    {
    }

    Tmpfile(std::string directory) : path{directory}
    {
        path += "/hipFile.XXXXXX";
        if ((fd = mkstemp(path.data())) == -1) {
            throw std::runtime_error("Could not create temporary file");
        }

#ifdef __HIP_PLATFORM_AMD__
        if (unlink(path.c_str()) == -1) {
            throw std::runtime_error("Could not unlink temporary file");
        }
#endif
    }

    ~Tmpfile()
    {
#ifdef __HIP_PLATFORM_NVIDIA__
        unlink(path.c_str());
#endif
        close(fd);
    }
};
