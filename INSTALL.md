# Install guide for hipFile / rocFile

## Building hipFile and rocFile

> [!NOTE]
> hipFile is alpha software that has undergone testing on limited hardware. It may not work on your system at this time.

Supported hardware:
* hipFile should work with the [GPUs that ROCm supports](https://rocm.docs.amd.com/projects/install-on-linux/en/latest/reference/system-requirements.html)
* Storage is currently limited to local NVMe drives
* hipFile has only been tested on AMD CPUs/systems

Supported compilers: amdclang++ (preferred), clang++, g++

If CMake can't find hipcc/nvcc, you can set `-DCMAKE_HIP_COMPILER=<path>`

Supported platforms: Linux (Windows may be supported in a future release)

Supported filesystems: Only ext4 is supported at this time

Targeting NVIDIA requires cuFile to be installed

### Prerequisites

> [!NOTE]
> hipFile relies on the unreleased [ROCm 7.2](https://github.com/ROCm/TheRock) libraries and associated [amdgpu](https://github.com/ROCm/amdgpu) drivers. We will update the install instructions when these are released.

### Configure

You do not need to set the `HIP_PLATFORM` environment variable, as
that will be set by CMake.

`cmake -DCMAKE_HIP_PLATFORM=(amd|nvidia) <options> <path/to/source>`

Options
|Option|Default|Purpose|
|------|-------|-------|
|AIS\_USE\_CLANG\_TIDY|OFF|Run the `clang-tidy` tool|
|AIS\_USE\_IWYU|OFF|Run the `include-what-you-use` tool|
|BUILD\_AIS\_DOCS|OFF|Build API documentation (requires Doxygen)|
|BUILD\_AISCP|ON|Build `aiscp` example program (OFF if no hipFile)|
|BUILD\_CODE\_COVERAGE|OFF|Generate code coverage information when tests are run|
|BUILD\_HIPFILE|ON|Build the hipFile library|
|BUILD\_ROCFILE|ON|Build the rocFile library (AMD only, OFF for NVIDIA)|
|BUILD\_TESTING|ON|Build the test suite|

### Build
`cmake --build .`

### Run tests

`ctest .` will run all tests.

You can filter on test labels using the `-L` option. We currently have `hipfile` and `rocfile` labels for each library and `unit`, `system`, and `stress` labels for the different test types. Multiple `-L` options form an *and*. A `|` can be used for an *or*.

`ctest . -L rocfile -L unit` runs all unit tests in the rocFile library.
`ctest . -L 'system|unit'` runs all system or unit tests.

You can filter on test names using the `-R` option. This option supports wildcards.

`ctest . -R RocFileBuffer.get_buffer_makes_temporary_buffer` runs the specified test.
`ctest . -R 'RocFileBuffer*'` runs all tests that start with RocFileBuffer.

### Install and package
The hipFile and rocFile libraries can be installed with CMake. The default
install prefix is still the CMake default of `/usr/local`.

`cmake --install .`

The libraries can also be packaged with CPack. .rpm, .deb. and .tar.gz
are all supported.

`cpack -G RPM`
`cpack -G DEB`
`cpack -G TGZ`

### Code coverage
The same version of Clang and LLVM must be used in order to generate
coverage results. `rocm-llvm-dev` can be installed to get the LLVM
version matching amdclang++.

PATH may need to be adjusted depending on the system configuration. The
following will add the default ROCm version of the tools to the path.
```
export PATH=/opt/rocm/bin:/opt/rocm/llvm/bin:"$PATH"
```

To generate code coverage information, the `BUILD_CODE_COVERAGE`
flag must be enabled when configuring. Run the build and test steps,
then run the following to generate the summaries.
```
<path/to/repo>/util/llvm-coverage.sh
```

The results will be wrote to `<path/to/repo>/build`, in the
`coverage-report.txt` and `coverage-lines.txt` files.

### Documentation
The API documentation is built using Doxygen. To build it, use the
BUILD\_AIS\_DOCS option. This will build the documentation for any
libraries that have been configured. As a special case, configuring
the documentation without any library will build the documentation
for BOTH libraries, allowing for a docs-only build.

The documentation will be built with the libraries and appear in
`docs/(hip|roc)file`. We build HTML, XML, and LaTeX docs. If you
want a pdf, run `make pdf` in the `latex` directory, which will
create a file named refman.pdf that you can rename.

If you want to build the docs without compiling the libraries,
you can just build the `doc` target:

    `cmake --build . --target doc`
