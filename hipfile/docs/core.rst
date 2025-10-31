Core API
========

API Versioning
--------------
hipFile uses (`semantic versioning <https://semver.org/>`_) so library
releases have major, minor, and patch versions. The soversion number
on POSIX systems matches the major API number, so we guarantee that minor
and patch releases will not break the ABI.

These version numbers are defined in ``hipfile.h``:
* ``HIPFILE_VERSION_MAJOR``
* ``HIPFILE_VERSION_MINOR``
* ``HIPFILE_VERSION_PATCH``

These symbols can be used to ifdef around API differences. They are always set
to integer values and will never contain text strings like "-patch1".

hipFile also includes an API call for determining the library version at
runtime:

    ``hipFileError_t hipFileGetVersion(unsigned *major, unsigned *minor, unsigned *patch)``

Any of the parameters can be ignored by passing in a NULL pointer.


The Version of the Back-end Library
-----------------------------------
hipFile also includes an API call for determining the back-end library version
(e.g., cuFile, rocFile) at runtime:

    ``hipFileError_t hipFileGetBackendVersion(int *version)``

The version number reported will depend on the underlying library. Currently,
both cuFile and rocFile use the following formula:

    ``1000 * <major version> + 10 * <minor version>``
