# cython: language_level=3
"""
Low-level Cython wrappers for the hipFile C API.

Every function mirrors the C API as closely as possible.
Functions that return ``hipFileError_t`` in C return a
``(hipFileOpError_t, hipError_t)`` 2-tuple here.
"""

from libc.string cimport memset
from libc.stdint cimport uintptr_t

from hipfile._chipfile cimport *


# ---------------------------------------------------------------------------
#  Module-level constants
# ---------------------------------------------------------------------------

VERSION_MAJOR = HIPFILE_VERSION_MAJOR
VERSION_MINOR = HIPFILE_VERSION_MINOR
VERSION_PATCH = HIPFILE_VERSION_PATCH
BASE_ERR      = HIPFILE_BASE_ERR


# ---------------------------------------------------------------------------
#  Internal helpers
# ---------------------------------------------------------------------------

cdef inline tuple _err(hipFileError_t e):
    return (<int>e.err, <int>e.hip_drv_err)


# ---------------------------------------------------------------------------
#  Error-handling helpers (replacements for C macros)
# ---------------------------------------------------------------------------

def is_hipfile_err(int err_code):
    """Equivalent of the ``IS_HIPFILE_ERR`` C macro."""
    return abs(err_code) > HIPFILE_BASE_ERR


def hipfile_errstr(int err_code):
    """Equivalent of the ``HIPFILE_ERRSTR`` C macro."""
    cdef const char *s = hipFileGetOpErrorString(<hipFileOpError_t>abs(err_code))
    if s == NULL:
        return ""
    return s.decode("utf-8")


def is_hip_drv_err(tuple err):
    """Equivalent of the ``IS_HIP_DRV_ERR`` C macro.

    Takes an error tuple as returned by the wrapper functions.
    """
    return err[0] == <int>hipFileHipDriverError


def hip_drv_err(tuple err):
    """Equivalent of the ``HIP_DRV_ERR`` C macro.

    Takes an error tuple and returns the ``hipError_t`` component.
    """
    return err[1]


def get_op_error_string(int status):
    """Wrapper for ``hipFileGetOpErrorString``."""
    cdef const char *s = hipFileGetOpErrorString(<hipFileOpError_t>status)
    if s == NULL:
        return ""
    return s.decode("utf-8")


# ---------------------------------------------------------------------------
#  Driver lifecycle
# ---------------------------------------------------------------------------

def driver_open():
    """Wrapper for ``hipFileDriverOpen``."""
    return _err(hipFileDriverOpen())


def driver_close():
    """Wrapper for ``hipFileDriverClose``."""
    return _err(hipFileDriverClose())


def use_count():
    """Wrapper for ``hipFileUseCount``."""
    return <int>hipFileUseCount()


# ---------------------------------------------------------------------------
#  Version
# ---------------------------------------------------------------------------

def get_version():
    """Wrapper for ``hipFileGetVersion``.

    Returns ``((major, minor, patch), error_tuple)``.
    """
    cdef unsigned major = 0, minor = 0, patch = 0
    cdef hipFileError_t e = hipFileGetVersion(&major, &minor, &patch)
    return ((major, minor, patch), _err(e))


# ---------------------------------------------------------------------------
#  File handles
# ---------------------------------------------------------------------------

def handle_register(int fd):
    """Wrapper for ``hipFileHandleRegister`` (POSIX fd path).

    Returns ``(handle_int, error_tuple)``.  The handle is an opaque
    integer that must be passed back to other hipFile calls.
    """
    cdef hipFileHandle_t fh = NULL
    cdef hipFileDescr_t descr
    memset(&descr, 0, sizeof(descr))
    descr.type = hipFileHandleTypeOpaqueFD
    descr.fd = fd
    cdef hipFileError_t e = hipFileHandleRegister(&fh, &descr)
    return (<uintptr_t>fh, _err(e))


def handle_deregister(uintptr_t handle):
    """Wrapper for ``hipFileHandleDeregister``."""
    hipFileHandleDeregister(<hipFileHandle_t>handle)


# ---------------------------------------------------------------------------
#  Buffer registration
# ---------------------------------------------------------------------------

def buf_register(uintptr_t buffer_base, size_t length, int flags=0):
    """Wrapper for ``hipFileBufRegister``."""
    return _err(hipFileBufRegister(<const void *>buffer_base, length, flags))


def buf_deregister(uintptr_t buffer_base):
    """Wrapper for ``hipFileBufDeregister``."""
    return _err(hipFileBufDeregister(<const void *>buffer_base))


# ---------------------------------------------------------------------------
#  Synchronous I/O
# ---------------------------------------------------------------------------

def read(uintptr_t handle, uintptr_t buffer_base, size_t size,
         hoff_t file_offset, hoff_t buffer_offset):
    """Wrapper for ``hipFileRead``.

    Returns raw ``ssize_t``:
      * ``>= 0`` — number of bytes read
      * ``-1``   — system error (check ``errno``)
      * ``< -1`` — negate to get a ``hipFileOpError_t``
    """
    return hipFileRead(<hipFileHandle_t>handle,
                       <void *>buffer_base, size,
                       file_offset, buffer_offset)


def write(uintptr_t handle, uintptr_t buffer_base, size_t size,
          hoff_t file_offset, hoff_t buffer_offset):
    """Wrapper for ``hipFileWrite``.

    Returns raw ``ssize_t``:
      * ``>= 0`` — number of bytes written
      * ``-1``   — system error (check ``errno``)
      * ``< -1`` — negate to get a ``hipFileOpError_t``
    """
    return hipFileWrite(<hipFileHandle_t>handle,
                        <const void *>buffer_base, size,
                        file_offset, buffer_offset)


# ---------------------------------------------------------------------------
#  Driver properties
# ---------------------------------------------------------------------------

def driver_get_properties():
    """Wrapper for ``hipFileDriverGetProperties``.

    Returns ``(props_dict, error_tuple)``.
    """
    cdef hipFileDriverProps_t props
    memset(&props, 0, sizeof(props))
    cdef hipFileError_t e = hipFileDriverGetProperties(&props)
    d = {
        "nvfs_major_version":        props.nvfs_major_version,
        "nvfs_minor_version":        props.nvfs_minor_version,
        "nvfs_poll_thresh_size":     props.nvfs_poll_thresh_size,
        "nvfs_max_direct_io_size":   props.nvfs_max_direct_io_size,
        "nvfs_driver_status_flags":  props.nvfs_driver_status_flags,
        "nvfs_driver_control_flags": props.nvfs_driver_control_flags,
        "feature_flags":             props.feature_flags,
        "max_device_cache_size":     props.max_device_cache_size,
        "per_buffer_cache_size":     props.per_buffer_cache_size,
        "max_device_pinned_mem_size": props.max_device_pinned_mem_size,
        "max_batch_io_count":        props.max_batch_io_count,
        "max_batch_io_timeout_msecs": props.max_batch_io_timeout_msecs,
    }
    return (d, _err(e))
