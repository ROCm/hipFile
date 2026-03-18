# hipFile Architecture

hipFile is AMD's Infinity Storage library enabling GPU-direct IO on ROCm -- data
transfer directly between NVMe storage and GPU memory, bypassing CPU staging
buffers. It is the ROCm equivalent of NVIDIA's cuFile/GDS (GPUDirect Storage).

## High-Level Overview

```
                     +---------------------------+
                     |     User Application      |
                     +------+-----------+--------+
                            |           |
               C/C++ API    |           |  Python API
                            v           v
               +------------+     +-----+-----------+
               | hipfile.h  |     | python/hipfile/  |
               | (Public C  |     | (ctypes bindings)|
               |  Header)   |     +-----+-----------+
               +------+-----+           |
                      |                  v
                      v            libhipfile.so
               +------+------+<---------+
               | libhipfile  |
               +------+------+
                      |
         +------------+-------------+
         |                          |
         v                          v
+--------+---------+    +-----------+----------+
|   amd_detail/    |    |    nvidia_detail/     |
|  (Native AMD     |    |   (cuFile shim for    |
|   ROCm backend)  |    |    NVIDIA GPUs)       |
+--------+---------+    +----------+-----------+
         |                         |
         v                         v
    HIP Runtime               cuFile / GDS
    + amdgpu KFD              (NVIDIA driver)
    + P2P DMA
```

Only one backend is compiled at a time, selected by `CMAKE_HIP_PLATFORM`
(`"amd"` or `"nvidia"`).

---

## Directory Structure

```
hipFile/
+-- include/
|   +-- hipfile.h                  Public C API header (sole public header)
+-- src/
|   +-- common/
|   |   +-- hipfile-common.cpp     Shared: error strings, version query
|   +-- amd_detail/                AMD/ROCm native implementation
|   |   +-- hipfile.cpp            C API entry points
|   |   +-- hipfile-private.h      Internal API (hipFileIo)
|   |   +-- state.{h,cpp}          DriverState singleton (central state)
|   |   +-- context.h              Context<T> singleton template
|   |   +-- backend.h              Backend abstract interface
|   |   +-- backend/
|   |   |   +-- fastpath.{h,cpp}   GPU-direct P2P DMA backend
|   |   |   +-- fallback.{h,cpp}   CPU bounce-buffer backend
|   |   |   +-- asyncop-fallback.{h,cpp}  Async state for fallback path
|   |   |   +-- memcpy-kernel.{h,hip}     HIP GPU kernel for async copy
|   |   +-- batch/
|   |   |   +-- batch.{h,cpp}      Batch IO context and operations
|   |   +-- file.{h,cpp}           File handle management (FileMap)
|   |   +-- file-descriptor.{h,cpp} RAII file descriptor wrapper
|   |   +-- buffer.{h,cpp}         GPU buffer management (BufferMap)
|   |   +-- stream.{h,cpp}         HIP stream management (StreamMap)
|   |   +-- async.{h,cpp}          AsyncOp, AsyncMonitor
|   |   +-- io.{h,cpp}             IoType enum + parameter validation
|   |   +-- hip.{h,cpp}            Mockable HIP runtime wrapper
|   |   +-- sys.{h,cpp}            Mockable POSIX syscall wrapper
|   |   +-- configuration.{h,cpp}  Runtime backend configuration
|   |   +-- environment.{h,cpp}    Environment variable reader
|   |   +-- mountinfo.{h,cpp}      Filesystem mount info (via libmount)
|   |   +-- stats.{h,cpp}          IO telemetry (StatsServer/StatsClient)
|   |   +-- passkey.h              PassKey idiom for access control
|   |   +-- util.h                 Variant pointer helper
|   +-- nvidia_detail/             NVIDIA cuFile translation layer
|       +-- hipfile.cpp            C API -> cuFile delegation
|       +-- hipfile-cufile.{h,cpp} Bidirectional type converters
+-- python/
|   +-- hipfile/
|   |   +-- __init__.py            Package exports
|   |   +-- bindings.py            Low-level ctypes FFI to libhipfile.so
|   |   +-- hipfile.py             High-level CuFile/CuFileDriver classes
|   +-- tests/                     Python test suite
|   +-- examples/                  PyTorch integration example
+-- tools/
|   +-- ais-check/                 System readiness checker (Python)
|   +-- ais-stats/                 IO statistics reporter (C++)
+-- test/
|   +-- system/                    Public API integration tests (require GPU)
|   +-- amd_detail/                AMD backend unit tests (mock-based)
|   +-- nvidia_detail/             cuFile API compatibility tests
|   +-- legacy/                    Legacy test programs
|   +-- common/                    Shared test fixtures and utilities
+-- shared/                        Headers shared across source and tests
+-- examples/aiscp/                GPU-accelerated file copy example
+-- cmake/                         Custom CMake modules
+-- docs/                          Doxygen and RST documentation
```

---

## Public C API

The public API is defined entirely in `include/hipfile.h` and is organized
into the following groups:

| Group       | Key Functions                                                   | Purpose                             |
|-------------|-----------------------------------------------------------------|-------------------------------------|
| **Core**    | `hipFileGetVersion`, `hipFileGet/SetParameter*`                 | Library version and configuration   |
| **Error**   | `hipFileError_t`, `hipFileGetOpErrorString`                     | Dual error codes (hipFile + HIP)    |
| **Driver**  | `hipFileDriverOpen/Close`, `hipFileDriverGetProperties`         | Lifecycle and driver configuration  |
| **File**    | `hipFileHandleRegister/Deregister`, `hipFileRead/Write`         | Synchronous GPU-direct IO           |
| **Batch**   | `hipFileBatchIOSetUp/Submit/GetStatus/Cancel/Destroy`           | Batched async IO                    |
| **Async**   | `hipFileReadAsync/WriteAsync`, `hipFileStreamRegister`          | Stream-based async IO               |

### Error Handling

Most API calls return `hipFileError_t`, a struct containing both a
`hipFileOpError_t` (hipFile-specific error, base value 5000) and a
`hipError_t` (HIP runtime error). Both fields must be checked. The struct
is `[[nodiscard]]` in C++17 and C23+.

`hipFileRead`/`hipFileWrite` return `ssize_t`: non-negative values are byte
counts; negative values encode error codes.

---

## AMD Backend (`src/amd_detail/`)

This is the native implementation for AMD GPUs on ROCm (~2600 lines).

### State Management

```
Context<DriverState>::get()  --->  DriverState (singleton)
                                      +-- FileMap        (fd -> IFile)
                                      +-- BufferMap      (ptr -> IBuffer)
                                      +-- StreamMap      (hipStream_t -> IStream)
                                      +-- BatchContextMap
                                      +-- backends[]     (Fastpath, Fallback)
                                      +-- ref_count
                                      +-- shared_mutex
```

**`Context<T>`** is a thread-safe singleton template. When compiled with
`-DAIS_TESTING`, it supports test-time replacement via `ContextOverride<T>`,
enabling dependency injection of mocks.

**`DriverState`** is the central state manager. It is reference-counted:
- `hipFileDriverOpen()` increments the count
- `hipFileDriverClose()` decrements it
- When the count reaches zero, all maps are cleared
- First call to `hipFileHandleRegister()` or `hipFileBufRegister()` auto-initializes

### Backend Strategy Pattern

```
          Backend (abstract interface)
          |  score()  -- eagerness to handle a request
          |  io()     -- perform the read or write
          |
     +----+----+
     |         |
 Fastpath   Fallback
```

IO dispatch iterates all backends, calls `score()` on each, and selects the
one with the highest score.

**Fastpath** (score = 100): Uses `hipAmdFileRead`/`hipAmdFileWrite` for
GPU-direct P2P DMA over PCIe. Eligible when the file supports O_DIRECT, the
buffer is device memory, and offsets/sizes are properly aligned. The call chain
goes through: HIP Runtime -> ROCR -> HSA -> Thunk -> KFD kernel driver.

**Fallback** (score = 0): CPU bounce-buffer approach using `pread`/`pwrite`
plus `hipMemcpy`. Processes data in 16 MiB chunks. Always available as a
compatibility path. Also provides `async_io()` for stream-based async operations.

### File and Buffer Management

**`UnregisteredFile`** gathers system metadata at construction time (statx,
fcntl flags, mountinfo) and creates both a buffered FD and an unbuffered
(O_DIRECT) FD where supported.

**`File`** (via `FileMap`) is the registered form, holding dual file
descriptors. The `PassKey` pattern restricts construction to `FileMap`.

**`Buffer`** (via `BufferMap`) wraps HIP-allocated GPU memory, querying
`hipPointerGetAttributes` for memory type and GPU ID. Supports transparent
creation of temporary unregistered buffers when a registered buffer lookup
misses.

### Async IO Pipeline

For async operations on the fallback path, the pipeline uses HIP graph node
callbacks:

```
hipLaunchHostFunc(bind_params)        Bind parameters to the operation
    |
hipLaunchHostFunc(cpu_copy/pread)     Copy data between file and bounce buffer
    |
hipLaunchKernel(memcpyKernel)         GPU kernel: copy between bounce buffer and device
    |
hipLaunchHostFunc(cleanup)            Free resources
```

**`AsyncOp`** uses `std::variant<T, T*>` to hold either a snapshot value or a
live pointer, depending on stream flags (fixed vs variable offsets).

**`AsyncOpFallback`** allocates pinned host bounce buffers via `hipHostMalloc`.
The object itself lives in pinned memory (custom `operator new/delete`) so it
can be accessed from GPU kernels.

**`AsyncMonitor`** runs a background thread that manages async operation
lifecycle to prevent deadlocks with `hipDeviceSynchronize`.

### Configuration and Environment

**`Configuration`** determines which backends are active at runtime:
- Fastpath is enabled if `hipAmdFileRead`/`hipAmdFileWrite` symbols exist in the
  HIP runtime (discovered via `hipGetProcAddress`) AND `HIPFILE_FORCE_COMPAT_MODE`
  is not `true`
- Fallback is enabled unless `HIPFILE_ALLOW_COMPAT_MODE` is `false`

**`Environment`** reads these environment variables:

| Variable                        | Type   | Effect                                         |
|---------------------------------|--------|-------------------------------------------------|
| `HIPFILE_FORCE_COMPAT_MODE`     | bool   | Force fallback path only                        |
| `HIPFILE_ALLOW_COMPAT_MODE`     | bool   | Allow/disallow fallback (default: allowed)      |
| `HIPFILE_STATS_LEVEL`           | uint   | 0=disabled, 1=basic telemetry, 2=reserved       |

### IO Telemetry

**`StatsServer`** creates a `memfd` and maps it to shared memory. It runs a
Unix domain socket server thread that passes the fd to clients via `SCM_RIGHTS`.
Four atomic counters are tracked:
- Fast-path read bytes
- Fast-path write bytes
- Fallback-path read bytes
- Fallback-path write bytes

**`StatsClient`** (used by the `ais-stats` tool) connects to the server, maps
the shared memory, and generates human-readable reports.

### Platform Wrappers (Mockability)

All external dependencies go through virtual wrapper classes:

- **`Hip`** (`hip.h`): Wraps 14 HIP runtime calls (`hipPointerGetAttributes`,
  `hipMemcpy`, `hipHostMalloc`, `hipAmdFileRead/Write`, `hipLaunchKernel`, etc.)
- **`Sys`** (`sys.h`): Wraps 14 POSIX calls (`open`, `close`, `pread`,
  `pwrite`, `mmap`, `statx`, `syslog`, etc.)
- **`LibMount`/`LibMountHelper`** (`mountinfo.h`): Wraps libmount for querying
  filesystem type and ext4 journaling mode

All wrappers use the `Context<T>` singleton pattern so they can be replaced
with mocks during testing.

---

## NVIDIA Backend (`src/nvidia_detail/`)

A thin translation layer (~1220 lines) that delegates all hipFile calls to
NVIDIA's cuFile library. No custom IO logic, state management, or backends.

```
hipFile C API  -->  type conversion (hipFile <-> cuFile)  -->  cuFile API
```

- `hipfile-cufile.{h,cpp}` provides bidirectional type converters for all enums,
  structs, and error codes (855 lines)
- `hipfile.cpp` implements each `hipFile*` function by: (1) converting arguments,
  (2) calling the corresponding `cuFile*` function, (3) converting the result back

---

## Python Bindings (`python/hipfile/`)

Two-layer architecture designed as a drop-in replacement for NVIDIA's
`cufile-python` package:

```
User Code
    |
    v
hipfile.py         High-level: CuFile (context manager), CuFileDriver (singleton)
    |
    v
bindings.py        Low-level: ctypes FFI (struct defs, function signatures)
    |
    v
libhipfile.so      Native shared library
```

### Low-Level (`bindings.py`)

Pure `ctypes` FFI with no compilation step. Loads `libhipfile.so` at import
time. Defines ctypes structures (`hipFileStatus`, `hipFileDescr`, `DescrUnion`)
mirroring the C header. Wraps 8 C functions with Python error checking:

- `hipFileDriverOpen/Close`
- `hipFileHandleRegister/Deregister`
- `hipFileBufRegister/Deregister`
- `hipFileRead/Write`

### High-Level (`hipfile.py`)

- **`CuFileDriver`**: Singleton that calls `hipFileDriverOpen()` on creation.
  Naming intentionally matches NVIDIA's cuFile for drop-in compatibility.
- **`CuFile`**: Context-manager class with `open()`/`close()`/`read()`/`write()`.
  Opens the file with `os.open()`, registers it with `hipFileHandleRegister()`.

Usage pattern for cuFile-compatible applications:

```python
try:
    import cufile as gds_lib
except ImportError:
    import hipfile as gds_lib
```

---

## Tools

### `ais-check` (Python)

System readiness checker that validates three prerequisites:

| Check                    | Method                                                |
|--------------------------|-------------------------------------------------------|
| Kernel P2PDMA support    | Scans kernel config for `CONFIG_PCI_P2PDMA=y\|m`     |
| HIP runtime AIS support  | Checks `hipAmdFileRead/Write` symbols via ctypes      |
| amdgpu driver support    | Scans `/proc/kallsyms` for `kfd_ais_rw_file`         |

### `ais-stats` (C++)

IO statistics reporter. Two usage modes:
- **Launch**: `ais-stats <program> [args...]` -- fork/exec and collect stats
- **Attach**: `ais-stats -p <PID> [-i]` -- attach to a running process

Connects to the in-process `StatsServer` via Unix socket and reports
fast-path/fallback read/write byte counts.

---

## Build System

The build uses CMake (>= 3.21) with C, C++17, and HIP languages.

### Main Targets

| Target                  | Type        | Description                              |
|-------------------------|-------------|------------------------------------------|
| `hipfile`               | Library     | Shared library (`libhipfile.so`)         |
| `hipfile_system_tests`  | Executable  | System integration tests (require GPU)   |
| `hipfile_tests`         | Executable  | Unit tests                               |
| `internal_tests`        | Executable  | AMD-only internal unit tests             |
| `state_mt`, `batch_mt`  | Executable  | AMD-only concurrency stress tests        |
| `aiscp`                 | Executable  | Example file copy tool                   |
| `ais-stats`             | Executable  | AMD-only stats tool                      |

### Backend Selection

Controlled by `CMAKE_HIP_PLATFORM` (default: `"amd"`):

| Aspect              | AMD                                           | NVIDIA                                 |
|----------------------|-----------------------------------------------|----------------------------------------|
| Source directory     | `src/amd_detail/` (19 source files + HIP kernel) | `src/nvidia_detail/` (3 source files) |
| Compile definition   | `__HIP_PLATFORM_AMD__`                         | `__HIP_PLATFORM_NVIDIA__`             |
| Extra dependencies  | `libmount`, `hip::host`                        | cuFile, CUDA driver/runtime            |
| Internal tests      | Built (`test/amd_detail/`)                     | Not built                              |
| Stats tool          | Built                                          | Not built                              |

### Key Dependencies

| Dependency       | Required  | Mechanism                                           |
|------------------|-----------|-----------------------------------------------------|
| HIP              | Always    | `find_package(hip CONFIG REQUIRED)`                 |
| hsa-runtime64    | Always    | `find_package(hsa-runtime64 CONFIG REQUIRED)`       |
| rocm-cmake       | Always    | FetchContent (tag `rocm-6.4.3`)                     |
| libmount         | AMD only  | `find_library(... mount)`                           |
| CUDAToolkit      | NVIDIA    | `find_package(CUDAToolkit REQUIRED)`                |
| cuFile           | NVIDIA    | `find_library(CUFILE_LIBRARY cufile ...)`           |
| GoogleTest 1.17  | Tests     | FetchContent (or system)                            |
| Boost.Program_options | Tests | `find_package(Boost COMPONENTS program_options)`   |

### Installation

The library installs into `${ROCM_PATH}` (default `/opt/rocm`) and exports
the CMake target `hip::hipfile`. Consumers use:

```cmake
find_package(hipfile REQUIRED)
target_link_libraries(myapp hip::hipfile)
```

Packages can be built with CPack (DEB, RPM, TGZ).

---

## Testing Architecture

| Suite                | Label(s)             | GPU Required | Description                                    |
|----------------------|----------------------|--------------|------------------------------------------------|
| `test/system/`       | `system;hipfile`     | Yes          | Integration tests against real library         |
| `test/amd_detail/`   | `unit;internal`      | No           | Mock-based unit tests for AMD internals        |
| `test/amd_detail/`   | `stress;internal`    | No           | Concurrency stress tests (10 threads x 10s)    |
| `test/nvidia_detail/` | `unit;hipfile`      | No           | cuFile type-conversion compatibility tests     |
| `test/legacy/`       | (not in CTest)       | Yes          | Legacy end-to-end test programs                |
| `python/tests/`      | (pytest)             | No           | Python binding tests with mocked CDLL          |

The AMD unit tests use **Google Mock** extensively. All internal singletons
(`Hip`, `Sys`, `DriverState`, `Configuration`, `LibMount`, `StatsServer`,
`AsyncMonitor`) have corresponding mock classes (e.g., `mhip.h`, `msys.h`,
`mstate.h`). The `ContextOverride<T>` pattern automatically injects the mock
and reverts on destruction.

Test commands:
```sh
ctest . -L 'unit'                    # All unit tests
ctest . -L internal -L unit          # Internal unit tests only
ctest . -L 'system|unit'             # System or unit tests
ctest . -R 'HipFileBuffer*'          # Tests matching a pattern
```

---

## Design Patterns

| Pattern                          | Where Used                        | Purpose                                              |
|----------------------------------|-----------------------------------|------------------------------------------------------|
| **Strategy**                     | `Backend` / `Fastpath` / `Fallback` | IO backend selection via scoring                    |
| **Singleton with test override** | `Context<T>` / `ContextOverride<T>` | Global state with mock injection for testing        |
| **PassKey idiom**                | `PassKey<FileMap>`, `PassKey<BufferMap>` | Restricts object construction to authorized classes |
| **Reference counting**           | `DriverState.ref_count`           | Driver lifecycle matching cuFile semantics           |
| **Interface segregation**        | `IFile`, `IBuffer`, `IStream`, `IBatchContext` | Enables mock injection for testing          |
| **RAII**                         | `FileDescriptor`, unique_ptr deleters | Resource cleanup for fds, mmap, pinned memory      |
| **Exception-to-error bridge**    | `handle_exception()` in `hipfile.cpp` | C++ exceptions -> C error codes at API boundary    |
| **Mockable wrappers**            | `Hip`, `Sys`, `LibMount`          | Virtual wrappers around external APIs for testing    |
| **Shared-memory IPC**            | `StatsServer` / `StatsClient`     | IO telemetry via memfd + Unix socket fd-passing      |
| **Deferred binding**             | `std::variant<T, T*>` in `AsyncOp` | Fixed vs live parameter values based on stream flags |

---

## Data Flow (Read Path, AMD Backend)

```
hipFileRead(fh, buffer, size, file_off, buf_off)
    |
    v
handle_exception() wrapper            C++ exceptions -> C error codes
    |
    v
DriverState.getFileAndBuffer(fh, buffer)
    |
    v
for each backend in DriverState.getBackends():
    backend.score(file, buffer, ...)   Pick highest-scoring backend
    |
    +-- Fastpath.io()
    |       hipAmdFileRead()           P2P DMA: NVMe -> GPU VRAM
    |
    +-- Fallback.io()
    |       pread() + hipMemcpy()      Bounce via CPU RAM (16 MiB chunks)
    |
    v
return bytes_transferred               (or negative error code)
```

---

## Current Limitations (AMD Backend)

The following APIs are declared but return errors on the AMD backend:
- Driver property getters/setters (`hipFileDriverGetProperties`,
  `hipFileDriverSetPollMode`, etc.)
- Batch IO status/cancel/destroy (`hipFileBatchIOGetStatus`,
  `hipFileBatchIOCancel`)
- All parameter get/set functions (`hipFileGet/SetParameter*`)
- RDMA operations

All of these are fully functional on the NVIDIA backend (delegated to cuFile).

Supported filesystems: ext4 (with `data=ordered`) and xfs on local NVMe only.
