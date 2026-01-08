<head>
    <meta charset="UTF-8">
    <meta name="description" content="CUDA APIs supported by HIPIFY">
    <meta name="keywords" content="HIPIFY, HIP, ROCm, CUDA, CUDA2HIP, hipification, hipify-clang, hipify-perl, hipFile, cuFile">
</head>

# cuFile API supported by HIP


**Note\:** In the tables that follow the columns marked `A`, `D`, `C`, `R`, `U`, and `E` mean the following:
**A** - Added; **D** - Deprecated; **C** - Changed; **R** - Removed; **U** - Unsupported for CUDA version(s); **E** - Experimental

## **1. cuFile Types**

|**CUDA**|**A**|**D**|**C**|**R**|**HIP**|**A**|**D**|**C**|**R**|**U**|**E**|
|:--|:-:|:-:|:-:|:-:|:--|:-:|:-:|:-:|:-:|:-:|:-:|
|`CUDA_DRV_ERR`|15.0| | | |`HIP_DRV_ERR`|8.0.0| | | | | |
|`CUFILE_BATCH`|15.0| | | |`hipFileBatch`|8.0.0| | | | | |
|`CUFILE_CANCELED`|15.0| | | |`hipFileCanceled`|8.0.0| | | | | |
|`CUFILE_COMPLETE`|15.0| | | |`hipFileComplete`|8.0.0| | | | | |
|`CUFILE_ERRSTR`|15.0| | | |`HIPFILE_ERRSTR`|8.0.0| | | | | |
|`CUFILE_FAILED`|15.0| | | |`hipFileFailed`|8.0.0| | | | | |
|`CUFILE_INVALID`|15.0| | | |`hipFileInvalid`|8.0.0| | | | | |
|`CUFILE_PARAM_ENV_LOGFILE_PATH`|15.0| | | |`hipFileParamEnvLogfilePath`|8.0.0| | | | | |
|`CUFILE_PARAM_EXECUTION_MAX_IO_QUEUE_DEPTH`|15.0| | | |`hipFileParamExecutionMaxIOQueueDepth`|8.0.0| | | | | |
|`CUFILE_PARAM_EXECUTION_MAX_IO_THREADS`|15.0| | | |`hipFileParamExecutionMaxIOThreads`|8.0.0| | | | | |
|`CUFILE_PARAM_EXECUTION_MAX_REQUEST_PARALLELISM`|15.0| | | |`hipFileParamExecutionMaxRequestParallelism`|8.0.0| | | | | |
|`CUFILE_PARAM_EXECUTION_MIN_IO_THRESHOLD_SIZE_KB`|15.0| | | |`hipFileParamExecutionMinIOThresholdSizeKB`|8.0.0| | | | | |
|`CUFILE_PARAM_EXECUTION_PARALLEL_IO`|15.0| | | |`hipFileParamExecutionParallelIO`|8.0.0| | | | | |
|`CUFILE_PARAM_FORCE_COMPAT_MODE`|15.0| | | |`hipFileParamForceCompatMode`|8.0.0| | | | | |
|`CUFILE_PARAM_FORCE_ODIRECT_MODE`|15.0| | | |`hipFileParamForceOdirectMode`|8.0.0| | | | | |
|`CUFILE_PARAM_FS_MISC_API_CHECK_AGGRESSIVE`|15.0| | | |`hipFileParamFsMiscApiCheckAggressive`|8.0.0| | | | | |
|`CUFILE_PARAM_LOGGING_LEVEL`|15.0| | | |`hipFileParamLoggingLevel`|8.0.0| | | | | |
|`CUFILE_PARAM_LOG_DIR`|15.0| | | |`hipFileParamLogDir`|8.0.0| | | | | |
|`CUFILE_PARAM_POLLTHRESHOLD_SIZE_KB`|15.0| | | |`hipFileParamPollthresholdSizeKB`|8.0.0| | | | | |
|`CUFILE_PARAM_PREFER_IO_URING`|15.0| | | |`hipFileParamPreferIOUring`|8.0.0| | | | | |
|`CUFILE_PARAM_PROFILE_NVTX`|15.0| | | |`hipFileParamProfileNvtx`|8.0.0| | | | | |
|`CUFILE_PARAM_PROFILE_STATS`|15.0| | | |`hipFileParamProfileStats`|8.0.0| | | | | |
|`CUFILE_PARAM_PROPERTIES_ALLOW_COMPAT_MODE`|15.0| | | |`hipFileParamPropertiesAllowCompatMode`|8.0.0| | | | | |
|`CUFILE_PARAM_PROPERTIES_ALLOW_SYSTEM_MEMORY`|15.0| | | |`hipFileParamPropertiesAllowSystemMemory`|8.0.0| | | | | |
|`CUFILE_PARAM_PROPERTIES_BATCH_IO_TIMEOUT_MS`|15.0| | | |`hipFileParamPropertiesBatchIOTimeoutMs`|8.0.0| | | | | |
|`CUFILE_PARAM_PROPERTIES_IO_BATCHSIZE`|15.0| | | |`hipFileParamPropertiesIOBatchsize`|8.0.0| | | | | |
|`CUFILE_PARAM_PROPERTIES_MAX_DEVICE_CACHE_SIZE_KB`|15.0| | | |`hipFileParamPropertiesMaxDeviceCacheSizeKB`|8.0.0| | | | | |
|`CUFILE_PARAM_PROPERTIES_MAX_DEVICE_PINNED_MEM_SIZE_KB`|15.0| | | |`hipFileParamPropertiesMaxDevicePinnedMemSizeKB`|8.0.0| | | | | |
|`CUFILE_PARAM_PROPERTIES_MAX_DIRECT_IO_SIZE_KB`|15.0| | | |`hipFileParamPropertiesMaxDirectIOSizeKB`|8.0.0| | | | | |
|`CUFILE_PARAM_PROPERTIES_PER_BUFFER_CACHE_SIZE_KB`|15.0| | | |`hipFileParamPropertiesPerBufferCacheSizeKB`|8.0.0| | | | | |
|`CUFILE_PARAM_PROPERTIES_USE_POLL_MODE`|15.0| | | |`hipFileParamPropertiesUsePollMode`|8.0.0| | | | | |
|`CUFILE_PARAM_SKIP_TOPOLOGY_DETECTION`|15.0| | | |`hipFileParamSkipTopologyDetection`|8.0.0| | | | | |
|`CUFILE_PARAM_STREAM_MEMOPS_BYPASS`|15.0| | | |`hipFileParamStreamMemopsBypass`|8.0.0| | | | | |
|`CUFILE_PARAM_USE_PCIP2PDMA`|15.0| | | |`hipFileParamUsePcip2pdma`|8.0.0| | | | | |
|`CUFILE_PENDING`|15.0| | | |`hipFilePending`|8.0.0| | | | | |
|`CUFILE_READ`|15.0| | | |`hipFileBatchRead`|8.0.0| | | | | |
|`CUFILE_TIMEOUT`|15.0| | | |`hipFileTimeout`|8.0.0| | | | | |
|`CUFILE_WAITING`|15.0| | | |`hipFileWaiting`|8.0.0| | | | | |
|`CUFILE_WRITE`|15.0| | | |`hipFileBatchWrite`|8.0.0| | | | | |
|`CUFileBoolConfigParameter_t`|15.0| | | |`hipFileBoolConfigParameter_t`|8.0.0| | | | | |
|`CUFileSizeTConfigParameter_t`|15.0| | | |`hipFileSizeTConfigParameter_t`|8.0.0| | | | | |
|`CUFileStringConfigParameter_t`|15.0| | | |`hipFileStringConfigParameter_t`|8.0.0| | | | | |
|`CU_FILE_ALLOW_COMPAT_MODE`|15.0| | | |`hipFileAllowCompatMode`|8.0.0| | | | | |
|`CU_FILE_ASYNC_NOT_SUPPORTED`|15.0| | | |`hipFileAsyncNotSupported`|8.0.0| | | | | |
|`CU_FILE_BATCH_FULL`|15.0| | | |`hipFileBatchFull`|8.0.0| | | | | |
|`CU_FILE_BATCH_IO_SUPPORTED`|15.0| | | |`hipFileBatchIOSupported`|8.0.0| | | | | |
|`CU_FILE_BATCH_SUBMIT_FAILED`|15.0| | | |`hipFileBatchSubmitFailed`|8.0.0| | | | | |
|`CU_FILE_BEEGFS_SUPPORTED`|15.0| | | |`hipFileBEEGFSSupported`|8.0.0| | | | | |
|`CU_FILE_CUDA_CONTEXT_MISMATCH`|15.0| | | |`hipFileHipContextMismatch`|8.0.0| | | | | |
|`CU_FILE_CUDA_DRIVER_ERROR`|15.0| | | |`hipFileHipDriverError`|8.0.0| | | | | |
|`CU_FILE_CUDA_MEMORY_TYPE_INVALID`|15.0| | | |`hipFileHipMemoryTypeInvalid`|8.0.0| | | | | |
|`CU_FILE_CUDA_POINTER_INVALID`|15.0| | | |`hipFileHipPointerInvalid`|8.0.0| | | | | |
|`CU_FILE_CUDA_POINTER_RANGE_ERROR`|15.0| | | |`hipFileHipPointerRangeError`|8.0.0| | | | | |
|`CU_FILE_DEVICE_NOT_FOUND`|15.0| | | |`hipFileDeviceNotFound`|8.0.0| | | | | |
|`CU_FILE_DEVICE_NOT_SUPPORTED`|15.0| | | |`hipFileDeviceNotSupported`|8.0.0| | | | | |
|`CU_FILE_DIO_NOT_SET`|15.0| | | |`hipFileDIONotSet`|8.0.0| | | | | |
|`CU_FILE_DRIVER_ALREADY_OPEN`|15.0| | | |`hipFileDriverAlreadyOpen`|8.0.0| | | | | |
|`CU_FILE_DRIVER_CLOSING`|15.0| | | |`hipFileDriverClosing`|8.0.0| | | | | |
|`CU_FILE_DRIVER_INVALID_PROPS`|15.0| | | |`hipFileDriverInvalidProps`|8.0.0| | | | | |
|`CU_FILE_DRIVER_NOT_INITIALIZED`|15.0| | | |`hipFileDriverNotInitialized`|8.0.0| | | | | |
|`CU_FILE_DRIVER_UNSUPPORTED_LIMIT`|15.0| | | |`hipFileDriverUnsupportedLimit`|8.0.0| | | | | |
|`CU_FILE_DRIVER_VERSION_MISMATCH`|15.0| | | |`hipFileDriverVersionMismatch`|8.0.0| | | | | |
|`CU_FILE_DRIVER_VERSION_READ_ERROR`|15.0| | | |`hipFileDriverVersionReadError`|8.0.0| | | | | |
|`CU_FILE_DYN_ROUTING_SUPPORTED`|15.0| | | |`hipFileDynRoutingSupported`|8.0.0| | | | | |
|`CU_FILE_GETNEWFD_FAILED`|15.0| | | |`hipFileGetNewFDFailed`|8.0.0| | | | | |
|`CU_FILE_GPFS_SUPPORTED`|15.0| | | |`hipFileGPFSSupported`|8.0.0| | | | | |
|`CU_FILE_GPU_MEMORY_PINNING_FAILED`|15.0| | | |`hipFileGPUMemoryPinningFailed`|8.0.0| | | | | |
|`CU_FILE_HANDLE_ALREADY_REGISTERED`|15.0| | | |`hipFileHandleAlreadyRegistered`|8.0.0| | | | | |
|`CU_FILE_HANDLE_NOT_REGISTERED`|15.0| | | |`hipFileHandleNotRegistered`|8.0.0| | | | | |
|`CU_FILE_HANDLE_TYPE_OPAQUE_FD`|15.0| | | |`hipFileHandleTypeOpaqueFD`|8.0.0| | | | | |
|`CU_FILE_HANDLE_TYPE_OPAQUE_WIN32`|15.0| | | |`hipFileHandleTypeOpaqueWin32`|8.0.0| | | | | |
|`CU_FILE_HANDLE_TYPE_USERSPACE_FS`|15.0| | | |`hipFileHandleTypeUserspaceFS`|8.0.0| | | | | |
|`CU_FILE_INTERNAL_ERROR`|15.0| | | |`hipFileInternalError`|8.0.0| | | | | |
|`CU_FILE_INVALID_FILE_OPEN_FLAG`|15.0| | | |`hipFileInvalidFileOpenFlag`|8.0.0| | | | | |
|`CU_FILE_INVALID_FILE_TYPE`|15.0| | | |`hipFileInvalidFileType`|8.0.0| | | | | |
|`CU_FILE_INVALID_MAPPING_RANGE`|15.0| | | |`hipFileInvalidMappingRange`|8.0.0| | | | | |
|`CU_FILE_INVALID_MAPPING_SIZE`|15.0| | | |`hipFileInvalidMappingSize`|8.0.0| | | | | |
|`CU_FILE_INVALID_VALUE`|15.0| | | |`hipFileInvalidValue`|8.0.0| | | | | |
|`CU_FILE_IO_DISABLED`|15.0| | | |`hipFileIODisabled`|8.0.0| | | | | |
|`CU_FILE_IO_MAX_ERROR`|15.0| | | |`hipFileIOMaxError`|8.0.0| | | | | |
|`CU_FILE_IO_NOT_SUPPORTED`|15.0| | | |`hipFileIONotSupported`|8.0.0| | | | | |
|`CU_FILE_LUSTRE_SUPPORTED`|15.0| | | |`hipFileLustreSupported`|8.0.0| | | | | |
|`CU_FILE_MEMORY_ALREADY_REGISTERED`|15.0| | | |`hipFileMemoryAlreadyRegistered`|8.0.0| | | | | |
|`CU_FILE_MEMORY_NOT_REGISTERED`|15.0| | | |`hipFileMemoryNotRegistered`|8.0.0| | | | | |
|`CU_FILE_NFS_SUPPORTED`|15.0| | | |`hipFileNFSSupported`|8.0.0| | | | | |
|`CU_FILE_NVFS_DRIVER_ERROR`|15.0| | | |`hipFileDriverError`|8.0.0| | | | | |
|`CU_FILE_NVFS_SETUP_ERROR`|15.0| | | |`hipFileDriverSetupError`|8.0.0| | | | | |
|`CU_FILE_NVMEOF_SUPPORTED`|15.0| | | |`hipFileNVMeoFSupported`|8.0.0| | | | | |
|`CU_FILE_NVMESH_SUPPORTED`|15.0| | | |`hipFileNVMeshSupported`|8.0.0| | | | | |
|`CU_FILE_NVME_P2P_SUPPORTED`|15.0| | | |`hipFileNVMeP2PSsupported`|8.0.0| | | | | |
|`CU_FILE_NVME_SUPPORTED`|15.0| | | |`hipFileNVMeSupported`|8.0.0| | | | | |
|`CU_FILE_PARALLEL_IO_SUPPORTED`|15.0| | | |`hipFileParallelIOSupported`|8.0.0| | | | | |
|`CU_FILE_PERMISSION_DENIED`|15.0| | | |`hipFilePermissionDenied`|8.0.0| | | | | |
|`CU_FILE_PLATFORM_NOT_SUPPORTED`|15.0| | | |`hipFilePlatformNotSupported`|8.0.0| | | | | |
|`CU_FILE_RDMA_REGISTER`|15.0| | | |`HIPFILE_RDMA_REGISTER`|8.0.0| | | | | |
|`CU_FILE_RDMA_RELAXED_ORDERING`|15.0| | | |`HIPFILE_RDMA_RELAXED_ORDERING`|8.0.0| | | | | |
|`CU_FILE_SCALEFLUX_CSD_SUPPORTED`|15.0| | | |`hipFileScaleFluxCSDSupported`|8.0.0| | | | | |
|`CU_FILE_SCSI_SUPPORTED`|15.0| | | |`hipFileSCSISupported`|8.0.0| | | | | |
|`CU_FILE_STREAMS_SUPPORTED`|15.0| | | |`hipFileStreamsSupported`|8.0.0| | | | | |
|`CU_FILE_STREAM_FIXED_BUF_OFFSET`|15.0| | | |`HIPFILE_STREAM_FIXED_BUF_OFFSET`|8.0.0| | | | | |
|`CU_FILE_STREAM_FIXED_FILE_OFFSET`|15.0| | | |`HIPFILE_STREAM_FIXED_FILE_OFFSET`|8.0.0| | | | | |
|`CU_FILE_STREAM_FIXED_FILE_SIZE`|15.0| | | |`HIPFILE_STREAM_FIXED_FILE_SIZE`|8.0.0| | | | | |
|`CU_FILE_STREAM_PAGE_ALIGNED_INPUTS`|15.0| | | |`HIPFILE_STREAM_PAGE_ALIGNED_INPUTS`|8.0.0| | | | | |
|`CU_FILE_SUCCESS`|15.0| | | |`hipFileSuccess`|8.0.0| | | | | |
|`CU_FILE_USE_POLL_MODE`|15.0| | | |`hipFileUsePollMode`|8.0.0| | | | | |
|`CU_FILE_WEKAFS_SUPPORTED`|15.0| | | |`hipFileWekaFSSupported`|8.0.0| | | | | |
|`CUfileBatchHandle_t`|15.0| | | |`hipFileBatchHandle_t`|8.0.0| | | | | |
|`CUfileBatchMode_t`|15.0| | | |`hipFileBatchMode_t`|8.0.0| | | | | |
|`CUfileDescr_t`|15.0| | | |`hipFileDescr_t`|8.0.0| | | | | |
|`CUfileDriverControlFlags_t`|15.0| | | |`hipFileDriverControlFlags_t`|8.0.0| | | | | |
|`CUfileDriverStatusFlags_t`|15.0| | | |`hipFileDriverStatusFlags_t`|8.0.0| | | | | |
|`CUfileDrvProps_t`|15.0| | | |`hipFileDriverProps_t`|8.0.0| | | | | |
|`CUfileError_t`|15.0| | | |`hipFileError_t`|8.0.0| | | | | |
|`CUfileFSOps_t`| | | | |`hipFileFSOps_t`|8.0.0| | | | | |
|`CUfileFeatureFlags_t`|15.0| | | |`hipFileFeatureFlags_t`|8.0.0| | | | | |
|`CUfileFileHandleType`|15.0| | | |`hipFileFileHandleType`|8.0.0| | | | | |
|`CUfileHandle_t`|15.0| | | |`hipFileHandle_t`|8.0.0| | | | | |
|`CUfileIOEvents_t`|15.0| | | |`hipFileIOEvents_t`|8.0.0| | | | | |
|`CUfileIOParams_t`|15.0| | | |`hipFileIOParams_t`|8.0.0| | | | | |
|`CUfileOpError`|15.0| | | |`hipFileOpError_t`|8.0.0| | | | | |
|`CUfileOpcode_t`| | | | |`hipFileOpcode_t`|8.0.0| | | | | |
|`CUfileStatus_t`| | | | |`hipFileStatus_t`|8.0.0| | | | | |
|`IS_CUDA_ERR`|15.0| | | |`IS_HIP_DRV_ERR`|8.0.0| | | | | |
|`IS_CUFILE_ERR`|15.0| | | |`IS_HIPFILE_ERR`|8.0.0| | | | | |
|`cufileRDMAInfo_t`|15.0| | | |`hipFileRDMAInfo_t`|8.0.0| | | | | |

## **2. cuFile Functions**

|**CUDA**|**A**|**D**|**C**|**R**|**HIP**|**A**|**D**|**C**|**R**|**U**|**E**|
|:--|:-:|:-:|:-:|:-:|:--|:-:|:-:|:-:|:-:|:-:|:-:|
|`cuFileBatchIOCancel`|15.0| | | |`hipFileBatchIOCancel`|8.0.0| |8.0.0| | | |
|`cuFileBatchIODestroy`|15.0| | | |`hipFileBatchIODestroy`|8.0.0| |8.0.0| | | |
|`cuFileBatchIOGetStatus`|15.0| | | |`hipFileBatchIOGetStatus`|8.0.0| |8.0.0| | | |
|`cuFileBatchIOSetUp`|15.0| | | |`hipFileBatchIOSetUp`|8.0.0| |8.0.0| | | |
|`cuFileBatchIOSubmit`|15.0| | | |`hipFileBatchIOSubmit`|8.0.0| |8.0.0| | | |
|`cuFileBufDeregister`|15.0| | | |`hipFileBufDeregister`|8.0.0| |8.0.0| | | |
|`cuFileBufRegister`|15.0| | | |`hipFileBufRegister`|8.0.0| |8.0.0| | | |
|`cuFileDriverClose`|15.0| | | |`hipFileDriverClose`|8.0.0| |8.0.0| | | |
|`cuFileDriverClose_v2`|15.0| | | |`hipFileDriverClose`|8.0.0| |8.0.0| | | |
|`cuFileDriverGetProperties`|15.0| | | |`hipFileDriverGetProperties`|8.0.0| |8.0.0| | | |
|`cuFileDriverOpen`|15.0| | | |`hipFileDriverOpen`|8.0.0| |8.0.0| | | |
|`cuFileDriverSetMaxCacheSize`|15.0| | | |`hipFileDriverSetMaxCacheSize`|8.0.0| |8.0.0| | | |
|`cuFileDriverSetMaxDirectIOSize`|15.0| | | |`hipFileDriverSetMaxDirectIOSize`|8.0.0| |8.0.0| | | |
|`cuFileDriverSetMaxPinnedMemSize`|15.0| | | |`hipFileDriverSetMaxPinnedMemSize`|8.0.0| |8.0.0| | | |
|`cuFileDriverSetPollMode`|15.0| | | |`hipFileDriverSetPollMode`|8.0.0| |8.0.0| | | |
|`cuFileGetParameterBool`|15.0| | | |`hipFileGetParameterBool`|8.0.0| |8.0.0| | | |
|`cuFileGetParameterSizeT`|15.0| | | |`hipFileGetParameterSizeT`|8.0.0| |8.0.0| | | |
|`cuFileGetParameterString`|15.0| | | |`hipFileGetParameterString`|8.0.0| |8.0.0| | | |
|`cuFileHandleDeregister`|15.0| | | |`hipFileHandleDeregister`|8.0.0| |8.0.0| | | |
|`cuFileHandleRegister`|15.0| | | |`hipFileHandleRegister`|8.0.0| |8.0.0| | | |
|`cuFileRead`|15.0| | | |`hipFileRead`|8.0.0| |8.0.0| | | |
|`cuFileReadAsync`|15.0| | | |`hipFileReadAsync`|8.0.0| |8.0.0| | | |
|`cuFileSetParameterBool`|15.0| | | |`hipFileSetParameterBool`|8.0.0| |8.0.0| | | |
|`cuFileSetParameterSizeT`|15.0| | | |`hipFileSetParameterSizeT`|8.0.0| |8.0.0| | | |
|`cuFileSetParameterString`|15.0| | | |`hipFileSetParameterString`|8.0.0| |8.0.0| | | |
|`cuFileStreamDeregister`|15.0| | | |`hipFileStreamDeregister`|8.0.0| |8.0.0| | | |
|`cuFileStreamRegister`|15.0| | | |`hipFileStreamRegister`|8.0.0| |8.0.0| | | |
|`cuFileUseCount`|15.0| | | |`hipFileUseCount`|8.0.0| |8.0.0| | | |
|`cuFileWrite`|15.0| | | |`hipFileWrite`|8.0.0| |8.0.0| | | |
|`cuFileWriteAsync`|15.0| | | |`hipFileWriteAsync`|8.0.0| |8.0.0| | | |
|`cufileop_status_error`|15.0| | | |`hipFileGetOpErrorString`|8.0.0| | | | | |

