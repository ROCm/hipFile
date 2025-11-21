/* Copyright (c) Advanced Micro Devices, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <hipfile-types.h>
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
//  DOXYGEN SETUP
// ***********************************************************************

// Without the `file`/`defgroup` directives, Doxgyen will not emit
// documentation for anything aside from structs in a C header.
//
// *ALL* Doxygen comment blocks should have an `ingroup` directive so they
// group properly in the output.

/**
 * @file
 *
 * @mainpage rocFile API Reference
 *
 * @section contents Contents
 * - @ref core
 * - @ref error
 * - @ref driver
 * - @ref rdma
 * - @ref file
 * - @ref batch
 * - @ref async
 */

/*!
 * @defgroup core Core Functionality
 *
 * @defgroup error Errors and Error Handling
 *
 * @defgroup driver GPU IO Driver API
 *
 * @defgroup rdma Userspace RDMA API
 *
 * @defgroup file File Handle API
 *
 * @defgroup batch Batch API
 *
 * @defgroup async Async API
 */

// ***********************************************************************
//  LIBRARY VERSION NUMBERS
// ***********************************************************************

/*!
 * @brief rocFile major version number
 * @ingroup core
 */
#define ROCFILE_VERSION_MAJOR 0
/*!
 * @brief rocFile minor version number
 * @ingroup core
 */
#define ROCFILE_VERSION_MINOR 2
/*!
 * @brief rocFile patch version number
 * @ingroup core
 */
#define ROCFILE_VERSION_PATCH 0

// ***********************************************************************
//  ERROR HANDLING
// ***********************************************************************

/*!
 * @brief The base value for rocFile error codes
 * @ingroup error
 */
#define ROCFILE_BASE_ERR 5000

/* clang-format off */
/*!
 * @brief rocFile function return codes
 * @ingroup error
 *
 * An error code of -1 indicates a that a C or POSIX error has occurred and
 * errno is likely to have been set.
 *
 * @note ROCFILE_BASE_ERR + 21 and 32 are intentionally omitted.
 */
typedef enum rocFileOpError {
    rocFileSuccess                 = 0,                     //!< rocFile success
    rocFileDriverNotInitialized    = ROCFILE_BASE_ERR + 1,  //!< GPU IO driver is not loaded
    rocFileDriverInvalidProps      = ROCFILE_BASE_ERR + 2,  //!< Invalid GPU IO driver property provided
    rocFileDriverUnsupportedLimit  = ROCFILE_BASE_ERR + 3,  //!< GPU IO Driver property value is unsupported
    rocFileDriverVersionMismatch   = ROCFILE_BASE_ERR + 4,  //!< rocFile version does not match GPU IO driver version
    rocFileDriverVersionReadError  = ROCFILE_BASE_ERR + 5,  //!< Unable to read the GPU IO driver version
    rocFileDriverClosing           = ROCFILE_BASE_ERR + 6,  //!< GPU IO driver is closing and not accepting new requests
    rocFilePlatformNotSupported    = ROCFILE_BASE_ERR + 7,  //!< rocFile is not supported on the current platform
    rocFileIONotSupported          = ROCFILE_BASE_ERR + 8,  //!< rocFile is not supported on the selected file
    rocFileDeviceNotSupported      = ROCFILE_BASE_ERR + 9,  //!< The selected GPU does not support rocFile
    rocFileDriverError             = ROCFILE_BASE_ERR + 10, //!< GPU IO driver error
    rocFileHipDriverError          = ROCFILE_BASE_ERR + 11, //!< GPU driver error: Inspect the hipError_t value for additional information
    rocFileHipPointerInvalid       = ROCFILE_BASE_ERR + 12, //!< Invalid GPU pointer
    rocFileHipMemoryTypeInvalid    = ROCFILE_BASE_ERR + 13, //!< Memory type backing pointer is incompatible with rocFile
    rocFileHipPointerRangeError    = ROCFILE_BASE_ERR + 14, //!< Pointer range exceeds allocated memory region
    rocFileHipContextMismatch      = ROCFILE_BASE_ERR + 15, //!< GPU driver context mismatch
    rocFileInvalidMappingSize      = ROCFILE_BASE_ERR + 16, //!< Accessing memory beyond pinned memory buffer
    rocFileInvalidMappingRange     = ROCFILE_BASE_ERR + 17, //!< Accessing memory beyond mapped memory region
    rocFileInvalidFileType         = ROCFILE_BASE_ERR + 18, //!< Unsupported file type
    rocFileInvalidFileOpenFlag     = ROCFILE_BASE_ERR + 19, //!< Unsupported file open flags
    rocFileDIONotSet               = ROCFILE_BASE_ERR + 20, //!< O_DIRECT flag not set
    /* Value 5021 intentionally unused */
    rocFileInvalidValue            = ROCFILE_BASE_ERR + 22, //!< One or more arguments have an invalid value
    rocFileMemoryAlreadyRegistered = ROCFILE_BASE_ERR + 23, //!< Device pointer is already registered
    rocFileMemoryNotRegistered     = ROCFILE_BASE_ERR + 24, //!< Device pointer is not registered
    rocFilePermissionDenied        = ROCFILE_BASE_ERR + 25, //!< Permission error on device or file access
    rocFileDriverAlreadyOpen       = ROCFILE_BASE_ERR + 26, //!< GPU IO driver is already open
    rocFileHandleNotRegistered     = ROCFILE_BASE_ERR + 27, //!< File handle for GPU IO is not registered
    rocFileHandleAlreadyRegistered = ROCFILE_BASE_ERR + 28, //!< File handle for GPU IO is already registered
    rocFileDeviceNotFound          = ROCFILE_BASE_ERR + 29, //!< Selected device not found
    rocFileInternalError           = ROCFILE_BASE_ERR + 30, //!< Internal GPU IO library error
    rocFileGetNewFDFailed          = ROCFILE_BASE_ERR + 31, //!< Unable to obtain a new file descriptor
    /* Value 5032 intentionally unused */
    rocFileDriverSetupError        = ROCFILE_BASE_ERR + 33, //!< GPU IO Driver initialization error
    rocFileIODisabled              = ROCFILE_BASE_ERR + 34, //!< GPU IO config file prohibits GPU IO on specified file
    rocFileBatchSubmitFailed       = ROCFILE_BASE_ERR + 35, //!< Failed to submit request for batch operation
    rocFileGPUMemoryPinningFailed  = ROCFILE_BASE_ERR + 36, //!< Failed to allocated pinned device memory
    rocFileBatchFull               = ROCFILE_BASE_ERR + 37, //!< Batch operation queue is full
    rocFileAsyncNotSupported       = ROCFILE_BASE_ERR + 38, //!< rocFile async IO is not supported
    rocFileIOMaxError              = ROCFILE_BASE_ERR + 39, //!< Internal flag to mark largest rocFile error code
} rocFileOpError_t;
/* clang-format on */

/*!
 * @brief Return a descriptive string for a rocFile error
 * @ingroup error
 *
 * @param [in] status Return code provided by rocFile
 *
 * @return Description of the error encountered
 */
ROCFILE_API
const char *rocFileOpStatusError(rocFileOpError_t status);

// Ignoring return values from rocFile APIs is discouraged.
// On C++17 and C23 and up, we can make that emit a warning.
#if __cplusplus >= 201703L || __STDC_VERSION__ >= 202311L
#define __ROCFILE_NODISCARD [[nodiscard]]
#else
#define __ROCFILE_NODISCARD
#endif

/*!
 * @brief Error status returned from rocFile API calls
 * @ingroup error
 *
 * @note This struct has the `[[nodiscard]]` attribute in C++ >= 17 and
 *       C >= 23 so unhandled return values will generate warnings
 */
typedef struct __ROCFILE_NODISCARD rocFileError {
    rocFileOpError_t err;         //!< Errors related to rocFile or the GPU IO driver
    hipError_t       hip_drv_err; //!< Errors related to the GPU driver
} rocFileError_t;

/*!
 * @brief Determine if an error code is a rocFile error
 * @ingroup error
 * @hideinitializer
 *
 * Signature
 * @code
 * bool IS_ROCFILE_ERR(rocFileOpError err)
 * @endcode
 *
 * @return true/false
 */
#define IS_ROCFILE_ERR(err) (abs((int)err) > ROCFILE_BASE_ERR)

/*!
 * @brief Get an error string for a rocFile error
 * @ingroup error
 * @hideinitializer
 *
 * Signature
 * @code
 * const char *ROCFILE_ERRSTR(rocFileOpError err)
 * @endcode
 *
 * @return A string description of a rocFile error
 */
#define ROCFILE_ERRSTR(err) rocFileOpStatusError((rocFileOpError_t)abs(err))

/*!
 * @brief Determine if an error is a rocFile driver error
 * @ingroup error
 * @hideinitializer
 *
 * Signature
 * @code
 * bool IS_HIP_DRV_ERR(rocFileError_t err)
 * @endcode
 *
 * @return true/false
 */
#ifndef IS_HIP_DRV_ERR
#define IS_HIP_DRV_ERR(err) ((err).err == rocFileHipDriverError)
#endif

/*!
 * @brief Get the driver error from a rocFileError_t
 * @ingroup error
 * @hideinitializer
 *
 * Signature
 * @code
 * hipError_t HIP_DRV_ERR(rocFileError_t err)
 * @endcode
 *
 * @return The hipError_t component of err
 */
#ifndef HIP_DRV_ERR
#define HIP_DRV_ERR(err) ((err).hip_drv_err)
#endif

// ***********************************************************************
//  GPU IO DRIVER API
// ***********************************************************************

/* See this section in hipfile.h for a discussion about the
 * following enums.
 */

/*!
 * @brief Filesystems/storage protocols supported by GPU IO on this system
 * @ingroup driver
 *
 * @note Value 10 is reserved for YRCloudFile
 */
typedef enum rocFileDriverStatusFlags {
    rocFileLustreSupported       = 0, //!< Lustre is supported (UNSUPPORTED in rocFile)
    rocFileWekaFSSupported       = 1, //!< Weka is supported (UNSUPPORTED in rocFile)
    rocFileNFSSupported          = 2, //!< NFS is supported
    rocFileGPFSSupported         = 3, //!< GPFS/IBM Storage Scale is supported (UNSUPPORTED in rocFile)
    rocFileNVMeSupported         = 4, //!< Local NVMe is supported
    rocFileNVMeoFSupported       = 5, //!< NVMeoF is supported
    rocFileSCSISupported         = 6, //!< SCSI is supported (UNSUPPORTED in rocFile)
    rocFileScaleFluxCSDSupported = 7, //!< ScaleFlux CSD is supported (UNSUPPORTED in rocFile)
    rocFileNVMeshSupported       = 8, //!< NVMesh is supported (UNSUPPORTED in rocFile)
    rocFileBeeGFSSupported       = 9, //!< BeeGFS is supported (UNSUPPORTED in rocFile)
    /* 10 is reserved for YRCloudFile */
    rocFileNVMeP2PSsupported = 11, //!< NVMeP2P is supported (UNSUPPORTED in rocFile)
    rocFileScatefsSupported  = 12, //!< ScateFS is supported (UNSUPPORTED in rocFile)
} rocFileDriverStatusFlags_t;

/*!
 * @brief Control flags for passing to the GPU IO driver
 * @ingroup driver
 */
typedef enum rocFileDriverControlFlags {
    rocFileUsePollMode     = 0, //!< Enable polling for IO completion
    rocFileAllowCompatMode = 1, //!< Allow GPU IO to fall back to POSIX IO
} rocFileDriverControlFlags_t;

/*!
 * @brief GPU IO Transport & Features supported by the system
 * @ingroup driver
 */
typedef enum rocFileFeatureFlags {
    rocFileDynRoutingSupported = 0, //!< RDMA dynamic routing is supported
    rocFileBatchIOSupported    = 1, //!< Batch operations are supported
    rocFileStreamsSupported    = 2, //!< Streams are supported
    rocFileParallelIOSupported = 3, //!< Parallel IO is supported
} rocFileFeatureFlags_t;

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

/*
 * NOTE: !! AIS UNSUPPORTED !! (Not related to NVMeoRDMA)
 */

/*!
 * @brief Userspace RDMA configuration
 * @ingroup rdma
 */
typedef struct rocFileRDMAInfo {
    int         version;  //!< Version of the RDMA API to use
    int         desc_len; //!< Length of the description string
    const char *desc_str; //!< Describes the configuration of the RDMA operation
} rocFileRDMAInfo_t;

/*!
 * @defgroup rdma_flags RDMA Characteristic Flags
 * @ingroup rdma
 *
 * @brief Bitwise flags for RDMA characteristics
 * @{
 */

/// Flag for if the RDMA operation is registered
#define ROCFILE_RDMA_REGISTER 1

/// Flag for if the RDMA operation has relaxed ordering
#define ROCFILE_RDMA_RELAXED_ORDERING (1 << 1)

/*! @} */

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
rocFileError_t rocFileHandleRegister(rocFileHandle_t *fh, rocFileDescr_t *descr);

/*!
 * @brief Deregisters a file from GPU IO
 * @ingroup file
 *
 * @param [in] fh Opaque file handle for GPU IO operations
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileHandleDeregister(rocFileHandle_t fh);

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
rocFileError_t rocFileBufRegister(const void *buffer_base, size_t length, int flags);

/*!
 * @brief Deregisters a GPU memory region from being used with GPU IO
 * @ingroup file
 *
 * @param [in] buffer_base Base address of the GPU buffer
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileBufDeregister(const void *buffer_base);

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
 * @return else:    Negative value of the related rocFileOpError_t
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
 * @return else:    Negative value of the related rocFileOpError_t
 */
ROCFILE_API
ssize_t rocFileWrite(rocFileHandle_t fh, const void *buffer_base, size_t size, hoff_t file_offset,
                     hoff_t buffer_offset);

// ***********************************************************************
//  GPU IO DRIVER API
// ***********************************************************************

/*!
 * @brief Initialize the GPU IO driver for this process
 * @ingroup driver
 *
 * Each call to `rocFileDriverOpen()` increments the library's reference
 * count. If a call to `rocFileDriverOpen()` results in the reference count
 * transitioning from zero to one, the library's state will be initialized.
 *
 * Calling `rocFileDriverOpen()` is optional. The first call to
 * `rocFileBufRegister()` or `rocFileHandleRegister()` will trigger
 * library initialization and increment the library's reference count.
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileDriverOpen(void);

/*!
 * @brief Close the GPU IO driver for this process
 * @ingroup driver
 *
 * Each call to `rocFileDriverClose()` decrements the library's reference
 * count. If a call to `rocFileDriverClose()` results in the reference count
 * transitioning from one to zero, the library's state will be destroyed.
 *
 * Explicitly closing the library is not required; the library's state will be
 * destroyed automatically at program exit.
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileDriverClose(void);

/*!
 * @brief Obtain the current reference count for the library
 * @ingroup driver
 *
 * @see rocFileDriverOpen()
 * @see rocFileDriverClose()
 *
 * @return The library's reference count
 */
ROCFILE_API
int64_t rocFileUseCount(void);

/*!
 * @brief Get a list of GPU IO driver properties
 * @ingroup driver
 *
 * @param [out] props See `rocFileDriverProps_t` for what properties are reported
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileDriverGetProperties(rocFileDriverProps_t *props);

/*!
 * @brief Enable/disable polling mode for GPU IO
 * @ingroup driver
 *
 * @param [in] poll `true` to enable polling, `false` to disable
 * @param [in] poll_threshold_size Maximum IO size (in KiB) for which polling is
 *                                 used when enabled
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileDriverSetPollMode(bool poll, size_t poll_threshold_size);

/*!
 * @brief Set the maximum IO chunk size
 * @ingroup driver
 *
 * @param [in] max_direct_io_size Maximum IO chunk size (in KiB) for each IO request
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileDriverSetMaxDirectIOSize(size_t max_direct_io_size);

/*!
 * @brief Set the maximum amount of GPU memory that can be used for bounce buffers
 * @ingroup driver
 *
 * @param [in] max_cache_size Maximum GPU memory (in KiB) that can be reserved for bounce buffers
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileDriverSetMaxCacheSize(size_t max_cache_size);

/*!
 * @brief Set the maximum amount of GPU memory that can be pinned
 * @ingroup driver
 *
 * @param [in] max_pinned_size Maximum GPU memory (in KiB) that can be pinned
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileDriverSetMaxPinnedMemSize(size_t max_pinned_size);

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
rocFileError_t rocFileBatchIOSetUp(rocFileBatchHandle_t *batch_idp, unsigned max_nr);

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
rocFileError_t rocFileBatchIOSubmit(rocFileBatchHandle_t batch_idp, unsigned nr, rocFileIOParams_t *iocbp,
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
rocFileError_t rocFileBatchIOGetStatus(rocFileBatchHandle_t batch_idp, unsigned min_nr, unsigned *nr,
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
rocFileError_t rocFileBatchIOCancel(rocFileBatchHandle_t batch_idp);

/*!
 * @brief Destroys the batch IO handle and frees the associated resources
 * @ingroup batch
 *
 * @param [in] batch_idp Opaque handle for batch operations
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileBatchIODestroy(rocFileBatchHandle_t batch_idp);

// ***********************************************************************
//  ASYNC API
// ***********************************************************************

/*!
 * @defgroup stream_flags Stream registration flags
 * @ingroup async
 *
 * @brief Flags to configure async GPU IO behaviour at stream registration
 * @{
 */

/// Buffer offset is fixed at time of submission
#define ROCFILE_STREAM_FIXED_BUF_OFFSET 1

/// File offset is fixed at time of submission
#define ROCFILE_STREAM_FIXED_FILE_OFFSET (1 << 1)

/// File size is fixed at time of submission
#define ROCFILE_STREAM_FIXED_FILE_SIZE (1 << 2)

/// Offsets and size are 4k aligned
#define ROCFILE_STREAM_PAGE_ALIGNED_INPUTS (1 << 3)

/// Mask for selecting flag bits
#define ROCFILE_STREAM_FLAGS_MASK 0xf
/*! @} */

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
rocFileError_t rocFileReadAsync(rocFileHandle_t fh, void *buffer_base, size_t *size_p, hoff_t *file_offset_p,
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
rocFileError_t rocFileWriteAsync(rocFileHandle_t fh, void *buffer_base, size_t *size_p, hoff_t *file_offset_p,
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
rocFileError_t rocFileStreamRegister(hipStream_t stream, unsigned flags);

/*!
 * @brief Deregister a stream and free the associated resources
 * @ingroup async
 *
 * @param [in] stream The stream used for async IO requests
 *
 * @return A rocFile error
 */
ROCFILE_API
rocFileError_t rocFileStreamDeregister(hipStream_t stream);

// ***********************************************************************
//  CORE API
// ***********************************************************************

/*!
 * @brief Get the rocFile version
 * @ingroup core
 *
 * @param [out] major The major version
 * @param [out] minor The minor version
 * @param [out] patch The patch version
 *
 * @note Parameters can be set to NULL to ignore that part of the version
 *
 * @return rocFileSuccess
 */
ROCFILE_API
rocFileError_t rocFileGetVersion(unsigned *major, unsigned *minor, unsigned *patch);

// ***********************************************************************
//  PROPERTIES API
// ***********************************************************************

/*!
 * @brief size_t configuration parameters
 * @ingroup core
 */
typedef enum rocFileSizeTConfigParameter_t {
    rocFileParamProfileStats,                       //!<
    rocFileParamExecutionMaxIOQueueDepth,           //!<
    rocFileParamExecutionMaxIOThreads,              //!<
    rocFileParamExecutionMinIOThresholdSizeKB,      //!<
    rocFileParamExecutionMaxRequestParallelism,     //!<
    rocFileParamPropertiesMaxDirectIOSizeKB,        //!<
    rocFileParamPropertiesMaxDeviceCacheSizeKB,     //!<
    rocFileParamPropertiesPerBufferCacheSizeKB,     //!<
    rocFileParamPropertiesMaxDevicePinnedMemSizeKB, //!<
    rocFileParamPropertiesIOBatchsize,              //!<
    rocFileParamPollthresholdSizeKB,                //!<
    rocFileParamPropertiesBatchIOTimeoutMs,         //!<
} rocFileSizeTConfigParameter_t;

/*!
 * @brief Boolean configuration parameters
 * @ingroup core
 */
typedef enum rocFileBoolConfigParameter_t {
    rocFileParamPropertiesUsePollMode,       //!<
    rocFileParamPropertiesAllowCompatMode,   //!<
    rocFileParamForceCompatMode,             //!<
    rocFileParamFsMiscApiCheckAggressive,    //!<
    rocFileParamExecutionParallelIO,         //!<
    rocFileParamProfileNvtx,                 //!<
    rocFileParamPropertiesAllowSystemMemory, //!<
    rocFileParamUsePcip2pdma,                //!<
    rocFileParamPreferIOUring,               //!<
    rocFileParamForceOdirectMode,            //!<
    rocFileParamSkipTopologyDetection,       //!<
    rocFileParamStreamMemopsBypass,          //!<
} rocFileBoolConfigParameter_t;

/*!
 * @brief String configuration parameters
 * @ingroup core
 */
typedef enum rocFileStringConfigParameter_t {
    rocFileParamLoggingLevel,   //!<
    rocFileParamEnvLogfilePath, //!<
    rocFileParamLogDir,         //!<
} rocFileStringConfigParameter_t;

/*!
 * @brief Get the value of a size_t configuration parameter
 * @ingroup core
 *
 * @param param The configuration parameter
 * @param value The location to store the value of the configuration parameter
 *
 * @return rocFileSuccess
 */
ROCFILE_API
rocFileError_t rocFileGetParameterSizeT(rocFileSizeTConfigParameter_t param, size_t *value);

/*!
 * @brief Get the value of a Boolean configuration parameter
 * @ingroup core
 *
 * @param param The configuration parameter
 * @param value The location to store the value of the configuration parameter
 *
 * @return rocFileSuccess
 */
ROCFILE_API
rocFileError_t rocFileGetParameterBool(rocFileBoolConfigParameter_t param, bool *value);

/*!
 * @brief Get the value of a string configuration parameter
 * @ingroup core
 *
 * @param param    The configuration parameter
 * @param desc_str The location to store the value of the configuration parameter
 * @param len      The length of the desc_str parameter
 *
 * @return rocFileSuccess
 */
ROCFILE_API
rocFileError_t rocFileGetParameterString(rocFileStringConfigParameter_t param, char *desc_str, int len);

/*!
 * @brief Set the value of a size_t configuration parameter
 * @ingroup core
 *
 * @param param The configuration parameter
 * @param value The value of the configuration parameter
 *
 * @return rocFileSuccess
 */
ROCFILE_API
rocFileError_t rocFileSetParameterSizeT(rocFileSizeTConfigParameter_t param, size_t value);

/*!
 * @brief Set the value of a Boolean configuration parameter
 * @ingroup core
 *
 * @param param The configuration parameter
 * @param value The value of the configuration parameter
 *
 * @return rocFileSuccess
 */
ROCFILE_API
rocFileError_t rocFileSetParameterBool(rocFileBoolConfigParameter_t param, bool value);

/*!
 * @brief Set the value of a string configuration parameter
 * @ingroup core
 *
 * @param param    The configuration parameter
 * @param desc_str The value of the configuration parameter
 *
 * @return rocFileSuccess
 */
ROCFILE_API
rocFileError_t rocFileSetParameterString(rocFileStringConfigParameter_t param, const char *desc_str);

// Not a part of the public API
#undef __ROCFILE_NODISCARD

#ifdef __cplusplus
}
#endif
