Core API
========

API Versioning
--------------
rocFile uses (`semantic versioning <https://semver.org/>`_) so library
releases have major, minor, and patch versions. The soversion number
on POSIX systems matches the major API number, so we guarantee that minor
and patch releases will not break the ABI.

These version numbers are defined in ``rocfile.h``:
* ``ROCFILE_VERSION_MAJOR``
* ``ROCFILE_VERSION_MINOR``
* ``ROCFILE_VERSION_PATCH``

These symbols can be used to ifdef around API differences. They are always set
to integer values and will never contain text strings like "-patch1".

rocFile also includes an API call for determining the library version at
runtime:

    ``rocFileError_t rocFileGetVersion(int *version)``

The returned version number uses the same formula as cuFile:

    ``1000 * ROCFILE_MAJOR_VERSION + 10 * ROCFILE_MINOR_VERSION``

Parameter Getters and Setters
-----------------------------
Like cuFile, rocFile includes a set of functions for getting and setting library
parameters. These comprise several API calls that get/set parameters
of a particular type based on an enum selector (e.g., ``rocFileGetParameterSizeT()``).
These API calls are all unsupported at this time and will return errors
if called.

* ``rocFileError_t rocFileGetParameterSizeT(rocFileSizeTConfigParameter_t param, size_t *value)``
* ``rocFileError_t rocFileGetParameterBool(rocFileBoolConfigParameter_t param, bool *value)``
* ``rocFileError_t rocFileGetParameterString(rocFileStringConfigParameter_t param, char *desc_str, int len)``
* ``rocFileError_t rocFileSetParameterSizeT(rocFileSizeTConfigParameter_t param, size_t value)``
* ``rocFileError_t rocFileSetParameterBool(rocFileBoolConfigParameter_t param, bool value)``
* ``rocFileError_t rocFileSetParameterString(rocFileStringConfigParameter_t param, const char *desc_str)``
