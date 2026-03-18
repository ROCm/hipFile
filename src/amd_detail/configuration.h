/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

namespace hipFile {

class Configuration {

public:
    virtual ~Configuration() = default;

    /// @brief Checks if the fastpath backend is enabled
    /// @return true if the fastpath backend is enabled, false otherwise
    virtual bool fastpath() const noexcept;

    /// @brief Checks if the fallback backend is enabled
    /// @return true if the fallback backend is enabled, false otherwise
    virtual bool fallback() const noexcept;

    /// @brief Shows the level of detail for stats collection
    /// @return 0 if stats collection disabled, higher levels of detail as value increases
    virtual unsigned int statsLevel() const noexcept;
};

}
