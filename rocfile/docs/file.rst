File Handle API
===============

File Handles
------------
rocFile files are accessed via an opaque ``rocFileHandle_t`` pointer
obtained from ``rocFileHandleRegister()``. This registration API call
takes a ``rocFileDescr_t`` struct, which contains the filesystem file
descriptor to be used for rocFile IO. This rocFile handle must be closed
using ``rocFileHandleDeregister()`` to avoid leaking resources.

Ideally, ``rocFileDriverOpen()`` should be used to initialize the driver
before registering rocFile handles. rocFile will automatically perform
this initialization for the caller, though, if the driver has not been
initialized when ``rocFileHandleRegister()`` is called. See the driver
section of the documentation for a more thorough discussion of this.

Buffers
-------
Memory buffers that will be used with multiple rocFile IO operations
should be registered via ``rocFileBufRegister()``. If this is not
done, a temporary internal buffer will be used for IO, though this
may not be as performant as using registered buffers.

When registering a buffer, no special handle or pointer is returned
to the caller. Instead, the registered buffer will be tracked internally
and used for IO when appropriate. When no longer necessary for IO,
registered buffers should be freed using ``rocFileBufDeregister()``.

As in ``rocFileHandleRegister()``, the driver will automatically be
initialized by ``rocFileBufRegister()`` if it has not already.

IO Operations
-------------
rocFile read and write operations are performed using ``rocFileRead()``
and ``rocFileWrite()``, respectively. These API calls take a rocFile
handle, a buffer (registered or not), the size of the IO operation
in bytes, and the file and buffer offsets. If using a registered buffer,
the buffer pointer should be the one that was registered and not
a pointer inside that buffer.
