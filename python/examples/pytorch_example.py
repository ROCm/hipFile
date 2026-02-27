"""
examples/pytorch_example.py
----------------------------
Demonstrates loading a tensor from disk directly into GPU memory using
hipFile + PyTorch on an AMD ROCm system.

Requirements:
    - AMD GPU with ROCm installed
    - hipFile library (see https://github.com/ROCm/hipFile)
    - PyTorch with ROCm support (pip install torch --index-url https://download.pytorch.org/whl/rocm6.1)
    - This package: pip install hipfile  (or python -m pip install -e .)

Usage::

    python pytorch_example.py --file /path/to/float32_tensor.bin --count 1048576
"""

import argparse
import ctypes
import os
import struct
import time

import hipfile


def create_test_file(path: str, n_floats: int = 1024 * 1024) -> None:
    """Write n_floats random float32 values to a file for testing."""
    import random
    data = struct.pack(f"{n_floats}f", *[random.random() for _ in range(n_floats)])
    with open(path, "wb") as f:
        f.write(data)
    print(f"Created test file: {path} ({len(data)} bytes)")


def load_tensor_gpu_direct(filepath: str, n_floats: int):
    """Load float32 data from file directly into a GPU tensor via hipFile."""
    try:
        import torch
    except ImportError:
        print("PyTorch not available – skipping GPU demo.")
        return

    if not torch.cuda.is_available():
        print("No GPU available (torch.cuda.is_available() = False).")
        return

    dtype     = torch.float32
    byte_size = n_floats * dtype.itemsize

    # 1. Allocate device tensor
    tensor = torch.empty(n_floats, dtype=dtype, device="cuda")
    dev_ptr = tensor.data_ptr()  # raw integer GPU pointer

    # 2. Initialise hipFile driver and register the GPU buffer
    from hipfile.bindings import hipFileBufRegister, hipFileBufDeregister

    hipfile.CuFileDriver()
    hipFileBufRegister(ctypes.c_void_p(dev_ptr), byte_size, 0)
    try:
        with hipfile.CuFile(filepath, "r", use_direct_io=True) as hf:
            t0 = time.perf_counter()
            n_read = hf.read(dev_ptr, byte_size, file_offset=0)
            elapsed = time.perf_counter() - t0
    finally:
        hipFileBufDeregister(ctypes.c_void_p(dev_ptr))

    bw_gb = (n_read / elapsed) / 1e9
    print(f"GPU-direct read: {n_read} bytes in {elapsed*1000:.2f} ms  "
          f"({bw_gb:.2f} GB/s)")
    print(f"Tensor first 5 values: {tensor[:5].tolist()}")


def main():
    parser = argparse.ArgumentParser(description="hipFile + PyTorch example")
    parser.add_argument("--file",  default="/tmp/test_hipfile.bin",
                        help="Path to float32 binary file")
    parser.add_argument("--count", type=int, default=1024 * 1024,
                        help="Number of float32 values")
    parser.add_argument("--create", action="store_true",
                        help="Create a test file before reading")
    args = parser.parse_args()

    if args.create or not os.path.exists(args.file):
        create_test_file(args.file, args.count)

    load_tensor_gpu_direct(args.file, args.count)


if __name__ == "__main__":
    main()
