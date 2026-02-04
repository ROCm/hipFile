# Changelog for hipFile

## UNRELEASED - hipFile 0.2.0
### Added

### Changed
* Renamed `hipFileOpStatusError()` to `hipFileGetOpErrorString()`
* The `hipfile-doc` CMake target no longer exists (just use `doc`)
* The `doc` CMake target only exists if the `AIS_BUILD_DOCS` option is enabled
* `hipFileRead()`/`hipFileWrite()` will transfer at most 0x7ffff000 (2,147,479,552) bytes returning the number of bytes actually transferred
* The CMake namespace was changed from `roc::` to `hip::`

### Removed
* The rocFile library has been completely removed and the code is now a part of hipFile.
* The hipify patch was removed. You can get hipify with hipFile support at https://github.com/derobins/HIPIFY/tree/hipFile.

### Limitations
* The batch API calls are not supported w/ an AMD backend
* The async API calls are not supported w/ an AMD backend

### Known issues
