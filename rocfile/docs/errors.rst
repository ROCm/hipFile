Errors and Error Handling
=========================

Functions that return a rocFileOpError_t struct
-----------------------------------------------
Errors are handled identically to cuFile. Most API calls return
a ``rocFileError_t`` struct, which includes ``rocFileOpError_t``
field for returning rocFile error codes, and a ``hipError_t``
field for returning GPU error codes. These fields should be checked
for ``rocFileSuccess`` and ``hipSuccess``, respectively. Any other
values indicate an error.

::

    typedef struct __ROCFILE_NODISCARD rocFileError {
        rocFileOpError_t err;         //!< Errors related to rocFile or the GPU IO driver
        hipError_t       hip_drv_err; //!< Errors related to the GPU driver
    } rocFileError_t;

Note that the struct is marked with the ``[[nodiscard]]`` attribute
so in C++17 / C23 or greater, the compiler will complain if you do
not check error values.

If a GPU driver error occurs the ``rocFileOpError_t`` value will be
``rocFileHipDriverError`` and the ``hipError_t`` field will be set to
the appropriate HIP driver error value.

When any other rocFile error is returned, the ``rocFileOpError_t`` field will be
set to the appropriate rocFile error and the ``hipError_t`` field will
be set to ``hipSuccess``.

Several helper macros are included in ``rocfile.h`` that help with error checking:

* ``IS_ROCFILE_ERR()``
* ``ROCFILE_ERRSTR()``
* ``IS_HIP_DRV_ERR()``
* ``HIP_DRV_ERR()``

See the error section of the rocFile reference manual for documentation for
each of these macros.

Functions that return an integer value
--------------------------------------
Several read and write functions (e.g., ``rocFileRead()``) return a ``ssize_t`` value.
Like the POSIX ``read(3)`` call, these API calls return negative values for errors.
Unlike the POSIX call, however, which only returns -1 on errors, the rocFile IO
calls return a value that reflects the ``rocFileOpError_t`` or ``hipError_t``
value that would have been returned.

``hipError_t`` and its values are defined in ``hip/hip_runtime_api.h``. The enum
values are assigned integers up to ~1000. ``rocFileOpError_t`` enum values are
assigned values of 5000+.

When rocFile IO calls that return a ``size_t`` fail, the returned value is the
negative of the ``rocFileOpError_t`` value (rocFile errors) or the negative of
the ``hipError_t`` value (GPU driver errors) that would normally have been returned
via the ``rocFileError_t`` struct.

Other functions
---------------
* ``rocFileOpStatusError()`` returns a string that corresponds to a ``rocFileOpError_t`` value. It cannot fail.
* ``rocFileUseCount()`` returns the reference count of the library. It returns -1 on errors.
