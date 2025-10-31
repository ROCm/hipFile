Driver API
==========

Basic API Usage
---------------

The rocFile driver API is used to initialize and shut down the "driver" (i.e.,
the library state). The driver is reference counted, and calls to open and
close the driver simply increase and decrease the reference count. When the
reference count drops to zero, the driver's data structures are cleaned up,
including any open files, buffers, etc., which will be closed. The driver
does not currently maintain significant state, so there is little need to be
aggressive about closing the driver. All driver state will be properly
closed when the process exits.

The term "driver" is essentially synonymous with "library". The driver
is implemented as a library singleton and it is not possible to have multiple
drivers open in a single process.

The basic driver API calls:

* ``rocFileError_t rocFileDriverOpen(void)``
* ``rocFileError_t rocFileDriverClose(void)``
* ``int64_t rocFileUseCount(void)``

The open call should be called before making any other rocFile API calls
and the close call should be called when all rocFile API calls are complete.
The ``rocFileUseCount()`` call can be used to check the current reference
count of the driver.

Note that some API calls will automatically open the driver and bump the
reference count if it has not yet been explicitly opened:

* ``rocFileHandleRegister()``
* ``rocFileBufRegister()``

Also note that this "implicit opening" will not be tracked, so it's up the
application to add an extra call to ``rocFileDriverClose()`` to fully
close the driver.

These API calls will only increment the driver count if they succeed.
Because of this hidden driver opening, the reference count should be checked
if the intent is to completely shut down the driver, as simply matching open
and close calls may not reduce the reference count to zero.

These API calls have been coded to behave like cuFile's calls, regarding
initialization and errors, but there is no guarantee that rocFile's driver
calls will match unpublished cuFile API behaviour.

Driver Getters and Setters
--------------------------

These API calls exist in rocFile, but currently have no effect and will return
an error:

* ``rocFileError_t rocFileDriverGetProperties(rocFileDriverProps_t *props)``
* ``rocFileError_t rocFileDriverSetPollMode(bool poll, size_t poll_threshold_size)``
* ``rocFileError_t rocFileDriverSetMaxDirectIOSize(size_t max_direct_io_size)``
* ``rocFileError_t rocFileDriverSetMaxCacheSize(size_t max_cache_size)``
* ``rocFileError_t rocFileDriverSetMaxPinnedMemSize(size_t max_pinned_size)``

These API calls also exist in rocFile, but have no effect, cannot be used to set
driver properties, and will return an error:

* ``rocFileError_t rocFileSetParameterSizeT(rocFileSizeTConfigParameter_t param, size_t value)``
* ``rocFileError_t rocFileSetParameterBool(rocFileBoolConfigParameter_t param, bool value)``
* ``rocFileError_t rocFileSetParameterString(rocFileStringConfigParameter_t param, const char *desc_str)``
