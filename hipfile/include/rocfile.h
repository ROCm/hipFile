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
//  GPU IO DRIVER API
// ***********************************************************************

/* See this section in hipfile.h for a discussion about the
 * following enums.
 */

/*!
 * @brief GPU IO configuration
 * @ingroup driver
 */
typedef struct rocFileDriverProps {
    /*!
     * @brief GPU IO Driver Configuration
     */
    struct {
        unsigned major_version; //!< Major version of the GPU IO driver
        unsigned minor_version; //!< Minor version of the GPU IO driver

        uint64_t poll_thresh_size;   //!< Maximum IO size (in KiB) for which polling is used when poll mode is
                                     //!< enabled
        uint64_t max_direct_io_size; //!< Maximum IO size (in KiB) used by the GPU IO driver

        unsigned driver_status_flags;  //!< Bitfield that maps to rocFileDriverStatusFlags_t
        unsigned driver_control_flags; //!< Bitfield that maps to rocFileDriverControlFlags_t
    } nvfs;

    unsigned feature_flags; //!< Bitfield that maps to rocFileFeatureFlags_t

    uint64_t
        max_device_cache_size; //!< Maximum amount of GPU memory (in KiB) that can be used for bounce buffers
    uint64_t per_buffer_cache_size;      //!< Amount of GPU memory (in KiB) allocated for each bounce buffer
    uint64_t max_device_pinned_mem_size; //!< Maximum amount of GPU memory (in KiB) that can be pinned

    unsigned max_batch_io_count;         //!< Maximum number of batch operations that can be submitted at once
    unsigned max_batch_io_timeout_msecs; //!< Timeout (in msec) for a batch operation to complete
} rocFileDriverProps_t;

// ***********************************************************************
//  RDMA API
// ***********************************************************************

/*!
 * @brief Userspace RDMA configuration
 * @ingroup rdma
 */
typedef struct rocFileRDMAInfo {
    int         version;  //!< Version of the RDMA API to use
    int         desc_len; //!< Length of the description string
    const char *desc_str; //!< Describes the configuration of the RDMA operation
} rocFileRDMAInfo_t;

/// Flag for if the RDMA operation is registered
#define ROCFILE_RDMA_REGISTER 1

/// Flag for if the RDMA operation has relaxed ordering
#define ROCFILE_RDMA_RELAXED_ORDERING (1 << 1)

// ***********************************************************************
//  FILE HANDLE API
// ***********************************************************************

/*!
 * @brief IO operations for RDMA filesystems
 * @ingroup file
 */
typedef struct rocFileFSOps {
    /*!
     * Type of remote FS used. If NULL, use fstat to discover.
     */
    const char *(*fs_type)(void *handle);
    /*!
     * Get a list of host RDMA addresses. If NULL, use any address.
     */
    int (*getRDMADeviceList)(void *handle, struct sockaddr **hostaddrs);
    /*!
     * Get the assigned priority of a RDMA device. If -1, there is no preference.
     */
    int (*getRDMADevicePriority)(void *handle, char *, size_t, hoff_t, struct sockaddr *hostaddr);
    /*!
     * Read from the remote filesystem. If NULL, use the Linux VFS.
     */
    ssize_t (*read)(void *handle, char *, size_t, hoff_t, rocFileRDMAInfo_t *);
    /*!
     * Write to the remote filesystem. If NULL, use the Linux VFS.
     */
    ssize_t (*write)(void *handle, const char *, size_t, hoff_t, rocFileRDMAInfo_t *);
} rocFileFSOps_t;

/*!
 * @brief Type of file handle being used
 * @ingroup file
 */
typedef enum rocFileFileHandleType {
    rocFileHandleTypeOpaqueFD    = 1, //!< POSIX file descriptor
    rocFileHandleTypeOpaqueWin32 = 2, //!< Windows HANDLE file handle
    rocFileHandleTypeUserspaceFS = 3, //!< Userspace RDMA filesystem
} rocFileFileHandleType_t;

/*!
 * @brief Top-level structure for performing GPU IO
 * @ingroup file
 *
 *  rocFileHandleTypeOpaqueFD    -> handle.fd non-negative, fs_ops ignored
 *  rocFileHandleTypeOpaqueWin32 -> handle.hFile non-NULL, fs_ops ignored
 *  rocFileHandleTypeUserspaceFS -> handle.fd non-negative, fs_ops non-NULL
 */
typedef struct rocFileDescr {
    rocFileFileHandleType_t type; //!< Type of file handle being used
    union {
        int   fd;                 //!< POSIX file descriptor
        void *hFile;              //!< Win32 HANDLE file handle
    } handle;                     //!< Union containing the OS-specific file handle
    const rocFileFSOps_t *fs_ops; //!< Userspace RDMA filesystem operations
} rocFileDescr_t;

/*!
 * Opaque file handle used by the GPU IO driver/library
 * @ingroup file
 */
typedef void *rocFileHandle_t;

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
hipFileError_t rocFileHandleRegister(rocFileHandle_t *fh, rocFileDescr_t *descr);

/*!
 * @brief Deregisters a file from GPU IO
 * @ingroup file
 *
 * @param [in] fh Opaque file handle for GPU IO operations
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileHandleDeregister(rocFileHandle_t fh);

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
ssize_t rocFileRead(rocFileHandle_t fh, void *buffer_base, size_t size, hoff_t file_offset,
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
ssize_t rocFileWrite(rocFileHandle_t fh, const void *buffer_base, size_t size, hoff_t file_offset,
                     hoff_t buffer_offset);

// ***********************************************************************
//  BATCH API
// ***********************************************************************

/*!
 * @brief The direction of data movement in a batch IO request
 * @ingroup batch
 */
typedef enum rocFileOpcode {
    rocFileBatchRead  = 0, //!< Batch IO read operation
    rocFileBatchWrite = 1, //!< Batch IO write operation
} rocFileOpcode_t;

/*!
 * @brief The status of a batch IO operation
 * @ingroup batch
 */
typedef enum rocFileStatus {
    rocFileWaiting  = 1 << 0, //!< Batch IO request pending acceptance
    rocFilePending  = 1 << 1, //!< Batch IO request accepted and queued
    rocFileInvalid  = 1 << 2, //!< Batch IO request was rejected for being invalid or could not be queued
    rocFileCanceled = 1 << 3, //!< Batch IO request was canceled
    rocFileComplete = 1 << 4, //!< Batch IO request completed
    rocFileTimeout  = 1 << 5, //!< Batch IO request timed out
    rocFileFailed   = 1 << 6, //!< Batch IO request failed
} rocFileStatus_t;

/*!
 * @brief Mode of a batch IO operation
 * @ingroup batch
 */
typedef enum rocFileBatchMode {
    rocFileBatch = 1, //!< Normal batch IO operation
} rocFileBatchMode_t;

/*!
 * @brief Input parameters for a batch IO request
 * @ingroup batch
 */
typedef struct rocFileIOParams {
    rocFileBatchMode_t mode; //!< Mode of the batch IO request
    union {
        struct {
            void   *devPtr_base;   //!< Base address of the GPU buffer where data should be read/written
            int64_t file_offset;   //!< Offset into the file that should be read/written
            int64_t devPtr_offset; //!< Offset of the GPU buffer that should be read/written
            size_t  size;          //!< Number of bytes to read or write
        } batch;                   //!< Parameters for the read/write batch request
    } u;                           //!< Wrapping union for batch IO parameters
    rocFileHandle_t fh;            //!< Registered rocFile handle for the target file
    rocFileOpcode_t opcode;        //!< Direction data is moving for the batch request
    void           *cookie;        //!< Optionally used to track IO operations (e.g. self-reference pointer)
} rocFileIOParams_t;

/*!
 * @brief Status of a batch IO operation
 * @ingroup batch
 */
typedef struct rocFileIOEvents {
    void *cookie; //!< Optionally used to track IO operations (e.g. pointer to corresponding rocFileIOParams)
    rocFileStatus_t status; //!< Status of the batch IO operation
    size_t          ret;    //!< Number of bytes transferred or POSIX error code if negative
} rocFileIOEvents_t;

/*!
 * @brief Opaque batch operations handle
 * @ingroup batch
 */
typedef void *rocFileBatchHandle_t;

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
hipFileError_t rocFileBatchIOSetUp(rocFileBatchHandle_t *batch_idp, unsigned max_nr);

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
hipFileError_t rocFileBatchIOSubmit(rocFileBatchHandle_t batch_idp, unsigned nr, rocFileIOParams_t *iocbp,
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
hipFileError_t rocFileBatchIOGetStatus(rocFileBatchHandle_t batch_idp, unsigned min_nr, unsigned *nr,
                                       rocFileIOEvents_t *iocbp, struct timespec *timeout);

/*!
 * @brief Cancels all pending batch IO operations
 * @ingroup batch
 *
 * @param [in] batch_idp Opaque handle for batch operations
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileBatchIOCancel(rocFileBatchHandle_t batch_idp);

/*!
 * @brief Destroys the batch IO handle and frees the associated resources
 * @ingroup batch
 *
 * @param [in] batch_idp Opaque handle for batch operations
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileBatchIODestroy(rocFileBatchHandle_t batch_idp);

// ***********************************************************************
//  ASYNC API
// ***********************************************************************

/*!
 * @brief Perform an asynchronous read from a stream
 * @ingroup async
 *
 * @param [in]  fh              Opaque file handle for GPU IO operations
 * @param [in]  buffer_base     Base address of the GPU buffer
 * @param [in]  size_p          Number of bytes that should be read
 * @param [in]  file_offset_p   Offset into the file that should be read from
 * @param [in]  buffer_offset_p Offset of the GPU buffer that that the data should be written to
 * @param [out] bytes_read_p    Number of bytes read
 * @param [in]  stream          The hipStream to enqueue this async IO request.
 *                              If NULL, this request will be synchronous.
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileReadAsync(rocFileHandle_t fh, void *buffer_base, size_t *size_p, hoff_t *file_offset_p,
                                hoff_t *buffer_offset_p, ssize_t *bytes_read_p, hipStream_t stream);

/*!
 * @brief Perform an asynchronous write to a stream
 * @ingroup async
 *
 * @param [in]  fh              Opaque file handle for GPU IO operations
 * @param [in]  buffer_base     Base address of the GPU buffer
 * @param [in]  size_p          Number of bytes that should be written
 * @param [in]  file_offset_p   Offset into the file that should be written to
 * @param [in]  buffer_offset_p Offset of the GPU buffer that that the data should be read from
 * @param [out] bytes_written_p Number of bytes written
 * @param [in]  stream          The hipStream to enqueue this async IO request.
 *                              If NULL, this request will be synchronous.
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileWriteAsync(rocFileHandle_t fh, void *buffer_base, size_t *size_p, hoff_t *file_offset_p,
                                 hoff_t *buffer_offset_p, ssize_t *bytes_written_p, hipStream_t stream);

/*!
 * @brief Register a stream to be used by for asynchronous GPU IO
 * @ingroup async
 *
 * @param [in] stream The stream used for for async IO requests
 * @param [in] flags Flags that can optimize stream processing if parameters
 *                   are known/are aligned at time of request submission
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileStreamRegister(hipStream_t stream, unsigned flags);

/*!
 * @brief Deregister a stream and free the associated resources
 * @ingroup async
 *
 * @param [in] stream The stream used for async IO requests
 *
 * @return A rocFile error
 */
ROCFILE_API
hipFileError_t rocFileStreamDeregister(hipStream_t stream);

#ifdef __cplusplus
}
#endif
