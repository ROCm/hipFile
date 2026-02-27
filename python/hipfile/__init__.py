"""
hipfile – Python bindings for AMD hipFile (GPU-direct storage).

Drop-in replacement for the cufile Python package.
Mirrors cufile/__init__.py structure with hip* naming.

Quick start::

    import hipfile

    with hipfile.CuFile("data.bin", "r+") as f:
        f.write(ctypes.c_void_p(gpu_ptr), size, file_offset=0)
"""

# High-level (mirrors cufile.cufile exports)
from .hipfile import (
    CuFile,
    CuFileDriver,
)

# Low-level convenience functions (mirrors cufile.bindings exports)
from .bindings import (
    HipFileError,
    hipFileDriverOpen,
    hipFileDriverClose,
    hipFileHandleRegister,
    hipFileHandleDeregister,
    hipFileBufRegister,
    hipFileBufDeregister,
    hipFileRead,
    hipFileWrite,
    hipFileHandle_t,
    hipFileStatus,
    hipFileDescr,
    DescrUnion,
)

__version__ = "0.1.0"

__all__ = [
    # High-level
    "CuFile",
    "CuFileDriver",
    "HipFileError",
    # Low-level
    "hipFileDriverOpen",
    "hipFileDriverClose",
    "hipFileHandleRegister",
    "hipFileHandleDeregister",
    "hipFileBufRegister",
    "hipFileBufDeregister",
    "hipFileRead",
    "hipFileWrite",
    "hipFileHandle_t",
    "hipFileStatus",
    "hipFileDescr",
    "DescrUnion",
]
