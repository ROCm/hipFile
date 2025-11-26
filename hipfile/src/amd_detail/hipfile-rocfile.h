/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

// Conversions between hipFile and rocFile types

#pragma once

#include "hipfile.h"

// rocFile to hipFile ////////////////////////////////////////////////////////

hipFileStatus_t toHipFileStatus(rocFileStatus_t rf_status);

hipFileIOEvents_t toHipFileIOEvents(const rocFileIOEvents_t &rf_io_event);

// hipFile to rocFile ////////////////////////////////////////////////////////

rocFileDescr_t toRocFileDescr(const hipFileDescr_t &fd);

rocFileFileHandleType_t toRocFileFileHandleType(hipFileFileHandleType_t hf_type);

rocFileIOParams_t toRocFileIOParams(const hipFileIOParams_t &hf_io_params);

rocFileOpcode_t toRocFileOpcode(hipFileOpcode_t hf_opcode);

rocFileBatchMode toRocFileBatchMode(hipFileBatchMode_t hf_batch_mode);
