from enum import IntEnum

from hipfile._hipfile import (
    # hipFileOpError_t values (resolved from C at build time)
    hipFileSuccess,
    hipFileDriverNotInitialized,
    hipFileDriverInvalidProps,
    hipFileDriverUnsupportedLimit,
    hipFileDriverVersionMismatch,
    hipFileDriverVersionReadError,
    hipFileDriverClosing,
    hipFilePlatformNotSupported,
    hipFileIONotSupported,
    hipFileDeviceNotSupported,
    hipFileDriverError,
    hipFileHipDriverError,
    hipFileHipPointerInvalid,
    hipFileHipMemoryTypeInvalid,
    hipFileHipPointerRangeError,
    hipFileHipContextMismatch,
    hipFileInvalidMappingSize,
    hipFileInvalidMappingRange,
    hipFileInvalidFileType,
    hipFileInvalidFileOpenFlag,
    hipFileDIONotSet,
    hipFileInvalidValue,
    hipFileMemoryAlreadyRegistered,
    hipFileMemoryNotRegistered,
    hipFilePermissionDenied,
    hipFileDriverAlreadyOpen,
    hipFileHandleNotRegistered,
    hipFileHandleAlreadyRegistered,
    hipFileDeviceNotFound,
    hipFileInternalError,
    hipFileGetNewFDFailed,
    hipFileDriverSetupError,
    hipFileIODisabled,
    hipFileBatchSubmitFailed,
    hipFileGPUMemoryPinningFailed,
    hipFileBatchFull,
    hipFileAsyncNotSupported,
    hipFileIOMaxError,
    # hipFileFileHandleType_t values (resolved from C at build time)
    hipFileHandleTypeOpaqueFD,
    hipFileHandleTypeOpaqueWin32,
    hipFileHandleTypeUserspaceFS,
)


class OpError(IntEnum):
    """Python enum mirroring hipFileOpError_t.

    Values are sourced from the C enum via the Cython layer, not
    redefined.  Rebuilding the extension picks up any value changes
    in hipfile.h automatically.
    """

    Success = hipFileSuccess
    DriverNotInitialized = hipFileDriverNotInitialized
    DriverInvalidProps = hipFileDriverInvalidProps
    DriverUnsupportedLimit = hipFileDriverUnsupportedLimit
    DriverVersionMismatch = hipFileDriverVersionMismatch
    DriverVersionReadError = hipFileDriverVersionReadError
    DriverClosing = hipFileDriverClosing
    PlatformNotSupported = hipFilePlatformNotSupported
    IONotSupported = hipFileIONotSupported
    DeviceNotSupported = hipFileDeviceNotSupported
    DriverError = hipFileDriverError
    HipDriverError = hipFileHipDriverError
    HipPointerInvalid = hipFileHipPointerInvalid
    HipMemoryTypeInvalid = hipFileHipMemoryTypeInvalid
    HipPointerRangeError = hipFileHipPointerRangeError
    HipContextMismatch = hipFileHipContextMismatch
    InvalidMappingSize = hipFileInvalidMappingSize
    InvalidMappingRange = hipFileInvalidMappingRange
    InvalidFileType = hipFileInvalidFileType
    InvalidFileOpenFlag = hipFileInvalidFileOpenFlag
    DIONotSet = hipFileDIONotSet
    InvalidValue = hipFileInvalidValue
    MemoryAlreadyRegistered = hipFileMemoryAlreadyRegistered
    MemoryNotRegistered = hipFileMemoryNotRegistered
    PermissionDenied = hipFilePermissionDenied
    DriverAlreadyOpen = hipFileDriverAlreadyOpen
    HandleNotRegistered = hipFileHandleNotRegistered
    HandleAlreadyRegistered = hipFileHandleAlreadyRegistered
    DeviceNotFound = hipFileDeviceNotFound
    InternalError = hipFileInternalError
    GetNewFDFailed = hipFileGetNewFDFailed
    DriverSetupError = hipFileDriverSetupError
    IODisabled = hipFileIODisabled
    BatchSubmitFailed = hipFileBatchSubmitFailed
    GPUMemoryPinningFailed = hipFileGPUMemoryPinningFailed
    BatchFull = hipFileBatchFull
    AsyncNotSupported = hipFileAsyncNotSupported
    IOMaxError = hipFileIOMaxError


class FileHandleType(IntEnum):
    """Python enum mirroring hipFileFileHandleType_t.

    Values are sourced from the C enum via the Cython layer.
    """

    OpaqueFD = hipFileHandleTypeOpaqueFD
    OpaqueWin32 = hipFileHandleTypeOpaqueWin32
    UserspaceFS = hipFileHandleTypeUserspaceFS
