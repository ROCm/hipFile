# Changelog for hipFile

## UNRELEASED - hipFile 0.2.0
### Added

### Changed
* Renamed `hipFileOpStatusError()` to `hipFileGetOpErrorString()`

### Removed
* The rocFile library has been completely removed and the code is now a part of hipFile.
* The hipify patch was removed. You can get hipify with hipFile support at https://github.com/derobins/HIPIFY/tree/hipFile.

### Limitations
* The batch API calls are not supported w/ an AMD backend
* The async API calls are not supported w/ an AMD backend

### Known issues
