"""
hipfile.py – High-level interface to AMD hipFile (GPU-direct storage).

Mirrors cufile/cufile.py exactly, with hip* naming.
The CuFile class is a drop-in replacement for cufile.CuFile.
"""

import ctypes
import os

from .bindings import (
    hipFileDriverOpen,
    hipFileDriverClose,
    hipFileHandleRegister,
    hipFileHandleDeregister,
    hipFileRead,
    hipFileWrite,
)


# ---------------------------------------------------------------------------
# CuFileDriver singleton  (mirrors cufile.CuFileDriver exactly)
# ---------------------------------------------------------------------------

def _singleton(cls):
    _instances = {}
    def wrapper(*args, **kwargs):
        if cls not in _instances:
            _instances[cls] = cls(*args, **kwargs)
        return _instances[cls]
    return wrapper

@_singleton
class CuFileDriver:
    def __init__(self):
        hipFileDriverOpen()

    def __del__(self):
        hipFileDriverClose()


# ---------------------------------------------------------------------------
# Mode helper  (mirrors cufile._os_mode exactly)
# ---------------------------------------------------------------------------

def _os_mode(mode: str) -> int:
    modes = {
        "r":  os.O_RDONLY,
        "r+": os.O_RDWR,
        "w":  os.O_CREAT | os.O_WRONLY | os.O_TRUNC,
        "w+": os.O_CREAT | os.O_RDWR   | os.O_TRUNC,
        "a":  os.O_CREAT | os.O_WRONLY | os.O_APPEND,
        "a+": os.O_CREAT | os.O_RDWR   | os.O_APPEND,
    }
    if mode not in modes:
        raise ValueError(
            f"Unsupported mode {mode!r}. Supported: {list(modes)}"
        )
    return modes[mode]


# ---------------------------------------------------------------------------
# CuFile  (mirrors cufile.CuFile exactly)
# ---------------------------------------------------------------------------

class CuFile:
    """
    Main class for hipFile file operations.
    Drop-in replacement for cufile.CuFile.
    """

    def __init__(self, path: str, mode: str = "r", use_direct_io: bool = False):
        self._driver = CuFileDriver()
        self._path = path
        self._mode = mode
        self._os_mode = _os_mode(mode)
        if use_direct_io:
            try:
                self._os_mode |= os.O_DIRECT
            except AttributeError:
                pass
        self._handle = None
        self._hip_file_handle = None

    def __del__(self):
        try:
            if getattr(self, "_handle", None) is not None:
                self.close()
        except Exception:
            pass

    @property
    def is_open(self) -> bool:
        return self._handle is not None

    def open(self):
        """Opens the file and registers the handle."""
        if self.is_open:
            return
        self._handle = os.open(self._path, self._os_mode, 0o644)
        self._hip_file_handle = hipFileHandleRegister(self._handle)

    def close(self):
        """Deregisters the handle and closes the file."""
        if not self.is_open:
            return
        hipFileHandleDeregister(self._hip_file_handle)
        os.close(self._handle)
        self._handle = None
        self._hip_file_handle = None

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def read(
        self,
        dest: ctypes.c_void_p,
        size: int,
        file_offset: int = 0,
        dev_offset: int = 0,
    ) -> int:
        """Read from the file."""
        if not self.is_open:
            raise IOError("File is not open.")
        return hipFileRead(
            self._hip_file_handle, dest, size, file_offset, dev_offset
        )

    def write(
        self,
        src: ctypes.c_void_p,
        size: int,
        file_offset: int = 0,
        dev_offset: int = 0,
    ) -> int:
        """Write to the file."""
        if not self.is_open:
            raise IOError("File is not open.")
        return hipFileWrite(
            self._hip_file_handle, src, size, file_offset, dev_offset
        )

    def get_handle(self):
        """Get the raw file descriptor."""
        return self._handle

    def __repr__(self) -> str:
        return (
            f"<CuFile path={self._path!r} mode={self._mode!r} "
            f"open={self.is_open}>"
        )

