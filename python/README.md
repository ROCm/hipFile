# hipfile – Python bindings for AMD hipFile

Python `ctypes`-based bindings for [AMD hipFile](https://github.com/ROCm/hipFile),
the ROCm equivalent of NVIDIA's cuFile, enabling **GPU-direct storage** – data
movement directly between NVMe/filesystem storage and GPU memory, bypassing CPU
staging buffers.

> **Status:** Early-stage community bindings, tracking
> [ROCm/hipFile#201](https://github.com/ROCm/hipFile/issues/201).

---

## Requirements

- Linux (x86_64 or aarch64)
- ROCm installed (tested with ROCm 6.x)
- hipFile library built and installed from [ROCm/hipFile](https://github.com/ROCm/hipFile)
- Python 3.8+

---

## Installation

```bash
# From source
git clone https://github.com/ROCm/hipFile.git
cd hipFile/python
pip install -e .
```

Make sure `libhipfile.so` is on your `LD_LIBRARY_PATH`:

```bash
export LD_LIBRARY_PATH=/opt/rocm/lib:$LD_LIBRARY_PATH
```

---

## Quick start

```python
import os
import hipfile
import ctypes

# --- Open the driver ---
with hipfile.CuFileDriver():

    # Open a file with O_DIRECT for best performance
    fd = os.open("data.bin", os.O_RDONLY | os.O_DIRECT)
    try:
        byte_size = 4 * 1024 * 1024  # 4 MB

        # Allocate GPU memory (example using PyTorch)
        import torch
        tensor = torch.empty(1024 * 1024, dtype=torch.float32, device="cuda")
        gpu_ptr = tensor.data_ptr()

        # Register the GPU buffer, then do the I/O
        from hipfile.bindings import hipFileBufRegister, hipFileBufDeregister

        hipFileBufRegister(ctypes.c_void_p(gpu_ptr), byte_size, 0)
        try:
            with hipfile.CuFile("data.bin", "r") as f:
                n = f.read(ctypes.c_void_p(gpu_ptr), byte_size, file_offset=0)
                print(f"Read {n} bytes directly into GPU memory")
        finally:
            hipFileBufDeregister(ctypes.c_void_p(gpu_ptr))

    finally:
        os.close(fd)
```

---

## API overview

### Driver lifecycle

```python
hipfile.hipFileDriverOpen()            # initialise the hipFile driver
hipfile.hipFileDriverClose()           # tear down

props = hipfile.hipFileDriverGetProperties()  # returns hipFileDriverProps_t
print(props.major_version, props.minor_version)

hipfile.hipFileDriverSetMaxDirectIOSize(128)   # KB
hipfile.hipFileDriverSetMaxCacheSize(512)       # KB
hipfile.hipFileDriverSetMaxPinnedMemSize(256)  # KB
```

### Context managers

```python
from hipfile.bindings import hipFileBufRegister, hipFileBufDeregister

with hipfile.CuFileDriver():                    # open / close driver
    # Register GPU buffer
    hipFileBufRegister(ctypes.c_void_p(ptr), size, 0)
    try:
        with hipfile.CuFile("data.bin", "r+") as f:  # open / close file
            f.read(ptr, count=size, file_offset=0)
            f.write(ptr, count=size, file_offset=0)
    finally:
        hipFileBufDeregister(ctypes.c_void_p(ptr))
```

### Buffer registration

```python
from hipfile.bindings import hipFileBufRegister, hipFileBufDeregister

hipFileBufRegister(ctypes.c_void_p(gpu_ptr), size, 0)
hipFileBufDeregister(ctypes.c_void_p(gpu_ptr))
```

### Error handling

```python
try:
    hipfile.hipFileDriverOpen()
except hipfile.HipFileError as e:
    print(f"HipFile error occurred")
```

---

## Running the tests

The test suite uses mocks and runs without real hardware:

```bash
pip install pytest
pytest tests/ -v
```

For LMCache integration testing:

```bash
python test_lmcache_integration.py
```

---

## PyTorch example

```bash
python examples/pytorch_example.py --create --count 1048576
```

---

## LMCache Integration

hipFile Python bindings are designed to be a drop-in replacement for NVIDIA's cuFile in applications like LMCache:

```python
# Works with both cuFile and hipFile
try:
    import cufile as gds_lib
except ImportError:
    import hipfile as gds_lib

# Same API for both
with gds_lib.CuFileDriver():
    from gds_lib.bindings import hipFileBufRegister, hipFileBufDeregister
    hipFileBufRegister(ctypes.c_void_p(tensor.data_ptr()), tensor.nbytes, 0)
    # ... perform GDS operations ...
```

---

## Building & Publishing

```bash
# Build sdist and wheel
uv build

# Validate the built artifacts
uvx twine check dist/*

# Publish to PyPI
uv publish
```

---

## How it works

hipFile provides a C API for GPU-direct I/O on AMD ROCm hardware. These Python
bindings use `ctypes` to call `libhipfile.so` directly, with no C compilation
needed. The binding layer:

1. Loads `libhipfile.so` at import time (lazy, configurable via `HIPFILE_LIB_PATH`).
2. Declares `argtypes` / `restype` for each API function.
3. Wraps the C types in Pythonic classes with context-manager support.
4. Translates error status codes to `HipFileError` exceptions.
5. Provides cuFile-compatible API for easy migration.

---

## Known Limitations

- `RegisteredBuffer` context manager is not yet implemented
- Some advanced cuFile features may not yet have hipFile equivalents
- Error reporting is less detailed than NVIDIA's cuFile

---

## Contributing

PRs welcome! The main tracking issue for official bindings is
[ROCm/hipFile#201](https://github.com/ROCm/hipFile/issues/201).
