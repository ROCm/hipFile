/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

// Common hipFile test functionality

#include "hipfile.h"
#include "magic-word.h"
#include "mhip.h"
#include "mmountinfo.h"
#include "msys.h"
#include "test-common.h"

#include <array>
#include <cassert>
#include <gtest/gtest.h>
#include <hip/hip_runtime_api.h>
#include <optional>

// ***********************************************************************
//  BASE ERROR CLASSES
// ***********************************************************************

// Base class for tests that open the driver
struct HipFileOpened : public ::testing::Test {

    HipFileOpened()
    {
        assert(hipFileUseCount() == 0);
        assert(hipFileDriverOpen() == HIPFILE_SUCCESS);
    }

    virtual ~HipFileOpened() override
    {
        while (hipFileUseCount()) {
            assert(hipFileDriverClose() == HIPFILE_SUCCESS);
        }
        assert(hipFileUseCount() == 0);
    }
};

// Base class for tests that do NOT open the driver
struct HipFileUnopened : public ::testing::Test {

    HipFileUnopened()
    {
        assert(hipFileUseCount() == 0);
    }

    virtual ~HipFileUnopened() override
    {
        while (hipFileUseCount()) {
            assert(hipFileDriverClose() == HIPFILE_SUCCESS);
        }
        assert(hipFileUseCount() == 0);
    }
};

// ***********************************************************************
//  BUFFER FUNCTIONALITY
// ***********************************************************************

/// @brief Set up mocks for buffer registration
void expect_buffer_registration(hipFile::MHip &mhip, hipMemoryType memory_type);

// ***********************************************************************
//  FILE FUNCTIONALITY
// ***********************************************************************

/// @brief Setup mocks for file registration
///
/// Mock methods will return default values
void expect_file_registration(hipFile::MSys &msys, hipFile::MLibMountHelper &mlibmounthelper);

/// @brief Setup mocks for file registration
///
/// Mock methods will return the specified values
void expect_file_registration(hipFile::MSys &msys, hipFile::MLibMountHelper &mlibmounthelper,
                              struct statx stxbuf, int fcntl_flags, hipFile::MountInfo mountinfo);

// ***********************************************************************
//  ENUM VALUE HELPERS
// ***********************************************************************

constexpr std::array<hipMemoryType, 1> SupportedHipMemoryTypes{hipMemoryTypeDevice};
constexpr std::array<hipMemoryType, 5> UnsupportedHipMemoryTypes{
    hipMemoryTypeArray,   hipMemoryTypeHost,         hipMemoryTypeManaged,
    hipMemoryTypeUnified, hipMemoryTypeUnregistered,
};
