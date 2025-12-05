/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

template <typename T> class Key {
    friend T;

    Key()
    {
    }
    Key(Key const &)
    {
    }
};
