/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <hip/hip_runtime_api.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

#if defined(__GNUC__)
#define ROCFILE_API __attribute__((visibility("default")))
#else
#define ROCFILE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ***********************************************************************
//  FILE HANDLE API
// ***********************************************************************

/*!
 * @brief Registers an open file for GPU IO
 * @ingroup file
 *
 * @note If the library has not already been initialized, the first call to
 *       `rocFileHandleRegister()` will initialize the library and increment
 *       the reference count (if it succeeds).
 *
 * @param [out] fh    Opaque file handle for GPU IO operations
 * @param [in]  descr Parameters for opening the file
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileHandleRegister(hipFileHandle_t *fh, hipFileDescr_t *descr);

/*!
 * @brief Deregisters a file from GPU IO
 * @ingroup file
 *
 * @param [in] fh Opaque file handle for GPU IO operations
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileHandleDeregister(hipFileHandle_t fh);

/*!
 * @brief Registers a GPU memory region to be used with GPU IO
 * @ingroup file
 *
 * The memory region should be allocated by the user before being passed to the API call
 *
 * @note If the library has not already been initialized, the first call to
 *       `rocFileBufRegister()` will initialize the library and increment
 *       the reference count.
 *
 * @param [in] buffer_base Base address of the GPU buffer
 * @param [in] length      Size of the allocated buffer in bytes
 * @param [in] flags       Control flags for this buffer
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileBufRegister(const void *buffer_base, size_t length, int flags);

/*!
 * @brief Deregisters a GPU memory region from being used with GPU IO
 * @ingroup file
 *
 * @param [in] buffer_base Base address of the GPU buffer
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileBufDeregister(const void *buffer_base);

/*!
 * @brief Synchronously read data from a file into a GPU buffer
 * @ingroup file
 *
 * @param [in] fh            Opaque file handle for GPU IO operations
 * @param [in] buffer_base   Base address of the GPU buffer
 * @param [in] size          Number of bytes that should be read
 * @param [in] file_offset   Offset into the file that should be read from
 * @param [in] buffer_offset Offset of the GPU buffer that that the data should be written to
 *
 * @return if >= 0: Number of bytes read
 * @return if -1:   System error (check `errno` for the specific error)
 * @return else:    Negative value of the related hipFileOpError_t
 */
ROCFILE_API
ssize_t rocFileRead(hipFileHandle_t fh, void *buffer_base, size_t size, hoff_t file_offset,
                    hoff_t buffer_offset);

/*!
 * @brief Synchronously write data from a GPU buffer to a file
 * @ingroup file
 *
 * @param [in] fh            Opaque file handle for GPU IO operations
 * @param [in] buffer_base   Base address of the GPU buffer
 * @param [in] size          Number of bytes that should be written
 * @param [in] file_offset   Offset into the file that should be written to
 * @param [in] buffer_offset Offset of the GPU buffer that the data should be read from
 *
 * @return if >= 0: Number of bytes written
 * @return if -1:   System error (check `errno` for the specific error)
 * @return else:    Negative value of the related hipFileOpError_t
 */
ROCFILE_API
ssize_t rocFileWrite(hipFileHandle_t fh, const void *buffer_base, size_t size, hoff_t file_offset,
                     hoff_t buffer_offset);

// ***********************************************************************
//  BATCH API
// ***********************************************************************

/*!
 * @brief Prepare the system to perform a batch IO operation
 * @ingroup batch
 *
 * @param [out] batch_idp Opaque handle for batch operations
 * @param [in] max_nr     Maximum number of requests that can be submitted to this batch handle
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileBatchIOSetUp(hipFileBatchHandle_t *batch_idp, unsigned max_nr);

/*!
 * @brief Enqueue a batch of IO requests for the GPU to complete asynchronously
 * @ingroup batch
 *
 * @param [in]     batch_idp     Opaque handle for batch operations
 * @param [in]     nr            Number of batch IO requests to submit
 * @param [in]     iocbp         An array of `nr` batch IO requests to submit to the GPU.
 *                               Data will be read into or written from the buffer specified in
 *                               each request.
 * @param [in]     flags Control Flags for the batch IO. Currently unused.
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileBatchIOSubmit(hipFileBatchHandle_t batch_idp, unsigned nr, hipFileIOParams_t *iocbp,
                                    unsigned flags);

/*!
 * @brief Poll for the status of completed batch IO operations
 * @ingroup batch
 *
 * @param [in] batch_idp Opaque handle for batch operations.
 * @param [in] min_nr Minimum number of batch operation statuses that should be returned.
 *                    If `timeout` is exceeded, fewer statuses may be returned.
 * @param [in,out] nr Maximum number of batch operation statuses that can be returned.
 *                    This is parameter is modified to return the number of statuses returned in `iocbp`.
 *
 * @param [out] iocbp An array containing up to `nr` statuses from the overall batch operation
 * @param [in] timeout Maximum amount of time this function should poll for status updates
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileBatchIOGetStatus(hipFileBatchHandle_t batch_idp, unsigned min_nr, unsigned *nr,
                                       hipFileIOEvents_t *iocbp, struct timespec *timeout);

/*!
 * @brief Cancels all pending batch IO operations
 * @ingroup batch
 *
 * @param [in] batch_idp Opaque handle for batch operations
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileBatchIOCancel(hipFileBatchHandle_t batch_idp);

/*!
 * @brief Destroys the batch IO handle and frees the associated resources
 * @ingroup batch
 *
 * @param [in] batch_idp Opaque handle for batch operations
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileBatchIODestroy(hipFileBatchHandle_t batch_idp);

#ifdef __cplusplus
}
#endif
