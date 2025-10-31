/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

// Common rocFile test functionality

#include "magic-word.h"
#include "rocfile-test.h"

#include "mhip.h"

#include "rocfile.h"

#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>

#include <array>
#include <cassert>

// ***********************************************************************
//  ERRORS AND ERROR HANDLING
// ***********************************************************************

// Set a particular rocFile error
constexpr rocFileError_t
RocFileHipError(hipError_t err)
{
    return {rocFileHipDriverError, err};
}

// Set a particular HIP error
constexpr rocFileError_t
RocFileOpError(rocFileOpError_t err)
{
    return {err, hipSuccess};
}

// == overload for rocFileError_t values
inline bool
operator==(const rocFileError_t &lhs, const rocFileError_t &rhs)
{
    return lhs.err == rhs.err && lhs.hip_drv_err == rhs.hip_drv_err;
}

// != overload for rocFileError_t values
inline bool
operator!=(const rocFileError_t &lhs, const rocFileError_t &rhs)
{
    return lhs.err != rhs.err || lhs.hip_drv_err != rhs.hip_drv_err;
}

// << overload for rocFileError_t values
//
// Unused in the test code, but kept here for iostream debugging
#ifndef NDEBUG
#include <iostream>
inline std::ostream &
operator<<(std::ostream &os, const rocFileError_t &rfe)
{
    return os << "rocFileError_t{ err: " << rfe.err << ", hip_drv_err: " << rfe.hip_drv_err << " }";
}
#endif

// Convenience "success" value
inline constexpr rocFileError_t ROCFILE_SUCCESS{rocFileSuccess, hipSuccess};

// Convenience "invalid argument" value
inline constexpr rocFileError_t ROCFILE_INVALID_VALUE{rocFileInvalidValue, hipSuccess};

// ***********************************************************************
//  BASE ERROR CLASSES
// ***********************************************************************

// Base class for tests that open the driver
struct RocFileOpened : public ::testing::Test {

    RocFileOpened()
    {
        assert(rocFileUseCount() == 0);
        assert(rocFileDriverOpen() == ROCFILE_SUCCESS);
    }

    virtual ~RocFileOpened() override
    {
        while (rocFileUseCount()) {
            assert(rocFileDriverClose() == ROCFILE_SUCCESS);
        }
        assert(rocFileUseCount() == 0);
    }
};

// Base class for tests that do NOT open the driver
struct RocFileUnopened : public ::testing::Test {

    RocFileUnopened()
    {
        assert(rocFileUseCount() == 0);
    }

    virtual ~RocFileUnopened() override
    {
        while (rocFileUseCount()) {
            assert(rocFileDriverClose() == ROCFILE_SUCCESS);
        }
        assert(rocFileUseCount() == 0);
    }
};

// ***********************************************************************
//  BUFFER FUNCTIONALITY
// ***********************************************************************

constexpr std::array<hipMemoryType, 6> HipMemoryTypes{
    hipMemoryTypeArray,   hipMemoryTypeDevice,  hipMemoryTypeHost,
    hipMemoryTypeManaged, hipMemoryTypeUnified, hipMemoryTypeUnregistered,
};

/// @brief Set up mocks for buffer registration
void expect_buffer_registration(rocFile::MHip &mhip, hipMemoryType memory_type);
