import os
import stat

from hipfile._hipfile import (
    # File handles
    handle_register,
    handle_deregister
)
from hipfile.error import HipFileException

class FileHandle():
    DEFAULT_MODE = stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP | stat.S_IROTH

    def __init__(self, path, flags, mode = DEFAULT_MODE, io_type = None):
        self._fd = None
        self._flags = flags
        self._handle = None
        # io_type currently unsupported - C bindings only use hipFileHandleTypeOpaqueFD
        self._io_type = io_type
        self._mode = mode
        self._path = path

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
    def mode(self):
        return self._mode
    
    @property
    def path(self):
        return self._path

    def open(self):
        if (self._handle is not None):
            raise RuntimeError("The FileHandle is already open.")
        self._fd = os.open(self._path, self._flags, self._mode)
        (handle, err) = handle_register(self._fd)
        if (err[0] != 0):
            os.close(self._fd)
            self._fd = None
            raise HipFileException(err[0], err[1])
        self._handle = handle
    
    def close(self):
        if (self._handle is not None):
            handle_deregister(self._handle)
            self._handle = None
        if (self._fd is not None):
            os.close(self._fd)
            self._fd = None
