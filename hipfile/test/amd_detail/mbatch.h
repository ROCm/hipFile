/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "batch/batch.h"

#include <gmock/gmock.h>

/*
 * A place to create mocks for the batch module.
 */

namespace rocFile {

class MBatchContext : public IBatchContext {
public:
    MOCK_METHOD(unsigned, get_capacity, (), (const, noexcept, override));
    MOCK_METHOD(void, submit_operations, (const rocFileIOParams_t *params, const unsigned num_params),
                (override));
};

}
