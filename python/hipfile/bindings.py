"""
Low-level ctypes bindings for libhipfile.so

Structured as a direct mirror of cufile/bindings.py so that the two modules
are interchangeable. Variable and function names are prefixed hip/Hip instead
of cu/Cu, but signatures, field layouts, and calling conventions are identical.

Reference: /usr/local/lib/python3.12/dist-packages/cufile/bindings.py
"""

import ctypes

libhipfile = ctypes.CDLL("libhipfile.so")


# ---------------------------------------------------------------------------
# Structs  (mirror cufile exactly, renamed hip*)
# ---------------------------------------------------------------------------

class hipFileStatus(ctypes.Structure):
    """Maps to CUfileError / hipFileStatus_t."""
    _fields_ = [
        ("err",    ctypes.c_int),   # hipFile error code
        ("cu_err", ctypes.c_int),   # underlying HIP runtime error
    ]


hipFileHandle_t = ctypes.c_void_p   # opaque handle (mirrors CUfileHandle_t)


class DescrUnion(ctypes.Union):
    """Union holding either a file descriptor or an opaque handle."""
    _fields_ = [
        ("fd",     ctypes.c_int),
        ("handle", ctypes.c_void_p),
    ]


class hipFileDescr(ctypes.Structure):
    """File descriptor passed to hipFileHandleRegister (mirrors CUfileDescr)."""
    _fields_ = [
        ("type",   ctypes.c_int),       # handle type: 1 = opaque fd
        ("handle", DescrUnion),
        ("fs_ops", ctypes.c_void_p),    # reserved / filesystem ops ptr
    ]


# ---------------------------------------------------------------------------
# Function signatures
# ---------------------------------------------------------------------------

libhipfile.hipFileDriverOpen.restype             = hipFileStatus
libhipfile.hipFileDriverClose.restype            = hipFileStatus
libhipfile.hipFileHandleRegister.restype         = hipFileStatus
libhipfile.hipFileBufRegister.restype            = hipFileStatus
libhipfile.hipFileBufDeregister.restype          = hipFileStatus
libhipfile.hipFileRead.restype                   = ctypes.c_ssize_t
libhipfile.hipFileWrite.restype                  = ctypes.c_ssize_t
libhipfile.hipFileHandleRegister.argtypes        = [ctypes.POINTER(hipFileHandle_t), ctypes.POINTER(hipFileDescr)]
libhipfile.hipFileHandleDeregister.argtypes      = [hipFileHandle_t]
libhipfile.hipFileBufRegister.argtypes           = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int]
libhipfile.hipFileBufDeregister.argtypes         = [ctypes.c_void_p]
libhipfile.hipFileRead.argtypes                  = [hipFileHandle_t, ctypes.c_void_p, ctypes.c_size_t,
                                                    ctypes.c_longlong, ctypes.c_longlong]
libhipfile.hipFileWrite.argtypes                 = [hipFileHandle_t, ctypes.c_void_p, ctypes.c_size_t,
                                                    ctypes.c_longlong, ctypes.c_longlong]

# convenience


class HipFileError(Exception):
    """Exception raised for hipFile errors."""
    pass


def _ck(status: hipFileStatus, name: str) -> None:
    if status.err != 0:
        raise HipFileError(
            f"{name} failed (hipFile err={status.err}, hip_err={status.cu_err})"
        )


def hipFileDriverOpen() -> None:
    _ck(libhipfile.hipFileDriverOpen(), "hipFileDriverOpen")


def hipFileDriverClose() -> None:
    _ck(libhipfile.hipFileDriverClose(), "hipFileDriverClose")


def hipFileHandleRegister(fd: int) -> hipFileHandle_t:
    descr        = hipFileDescr()
    descr.type   = 1                  # opaque fd  (matches cufile type=1)
    descr.handle = DescrUnion(fd=fd)
    handle       = hipFileHandle_t()
    _ck(
        libhipfile.hipFileHandleRegister(
            ctypes.byref(handle), ctypes.byref(descr)
        ),
        "hipFileHandleRegister",
    )
    return handle


def hipFileHandleDeregister(handle: hipFileHandle_t) -> None:
    libhipfile.hipFileHandleDeregister(handle)


def hipFileBufRegister(buf: ctypes.c_void_p, size: int, flags: int) -> None:
    _ck(libhipfile.hipFileBufRegister(buf, size, flags), "hipFileBufRegister")


def hipFileBufDeregister(buf: ctypes.c_void_p) -> None:
    _ck(libhipfile.hipFileBufDeregister(buf), "hipFileBufDeregister")


def hipFileRead(
    handle: hipFileHandle_t,
    buf: ctypes.c_void_p,
    size: int,
    file_offset: int,
    dev_offset: int,
) -> int:
    ret = libhipfile.hipFileRead(handle, buf, size, file_offset, dev_offset)
    if ret < 0:
        raise HipFileError(f"hipFileRead failed (error code {ret})")
    return ret


def hipFileWrite(
    handle: hipFileHandle_t,
    buf: ctypes.c_void_p,
    size: int,
    file_offset: int,
    dev_offset: int,
) -> int:
    ret = libhipfile.hipFileWrite(handle, buf, size, file_offset, dev_offset)
    if ret < 0:
        raise HipFileError(f"hipFileWrite failed (error code {ret})")
    return ret
