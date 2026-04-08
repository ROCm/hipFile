import os
import stat

from hipfile._hipfile import (
    # File handles
    handle_register,
    handle_deregister,
    # Synchronous I/O
    read as _read,
    write as _write,
)
from hipfile.enums import OpError, FileHandleType
from hipfile.error import HipFileException

DefaultHandleType = None
if os.name == "posix":
    DefaultHandleType = FileHandleType.OpaqueFD
elif os.name == "nt":
    DefaultHandleType = FileHandleType.OpaqueWin32


class FileHandle:
    DEFAULT_MODE = stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH

    def __init__(self, path, flags, mode=DEFAULT_MODE, handle_type=DefaultHandleType):
        self._fd = None
        self._flags = flags
        self._handle = None
        self._handle_type = None
        self._mode = mode
        self._path = path

        self.handle_type = handle_type

    def __del__(self):
        self.close()

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

    @property
    def flags(self):
        return self._flags

    @property
    def handle(self):
        return self._handle

    @property
    def handle_type(self):
        return self._handle_type

    @handle_type.setter
    def handle_type(self, _handle_type):
        if _handle_type not in FileHandleType:
            raise ValueError(f"'{_handle_type}' is not a member of enum FileHandleType")
        self._handle_type = _handle_type

    @property
    def mode(self):
        return self._mode

    @property
    def path(self):
        return self._path

    def open(self):
        if self._handle is not None:
            raise RuntimeError("The FileHandle is already open.")
        self._fd = os.open(self._path, self._flags, self._mode)
        handle, err = handle_register(self._fd, self._handle_type)
        if err[0] != 0:
            os.close(self._fd)
            self._fd = None
            raise HipFileException(err[0], err[1])
        self._handle = handle

    def close(self):
        if self._handle is not None:
            handle_deregister(self._handle)
            self._handle = None
        if self._fd is not None:
            os.close(self._fd)
            self._fd = None

    def read(self, buffer, size, file_offset, buffer_offset):
        if self._handle is None:
            raise RuntimeError("The FileHandle is not open.")
        bytes_read, extra_err = _read(
            self._handle, buffer.ptr, size, file_offset, buffer_offset
        )
        if bytes_read == -1:
            # extra_err is errno
            raise OSError(extra_err, os.strerror(extra_err))
        elif bytes_read < -1:
            # hipFile Error
            # If -bytes_read == OpError.HipDriverError, extra_err is hipError_t.
            # Otherwise, extra_err is 0.
            raise HipFileException(-bytes_read, extra_err)
        return bytes_read

    def write(self, buffer, size, file_offset, buffer_offset):
        if self._handle is None:
            raise RuntimeError("The FileHandle is not open.")
        bytes_written, extra_err = _write(
            self._handle, buffer.ptr, size, file_offset, buffer_offset
        )
        if bytes_written == -1:
            # extra_err is errno
            raise OSError(extra_err, os.strerror(extra_err))
        elif bytes_written < -1:
            # hipFile Error
            # If -bytes_written == OpError.HipDriverError, extra_err is hipError_t.
            # Otherwise, extra_err is 0.
            raise HipFileException(-bytes_written, extra_err)
        return bytes_written
