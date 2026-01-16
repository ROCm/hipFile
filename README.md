# hipFile

> [!CAUTION] 
> This release is an *early-access* software technology preview. Running production workloads is *not* recommended.

[![MIT](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/ROCm/hipFile/develop/LICENSE.md)
[![CI](https://github.com/ROCm/hipFile/actions/workflows/root-ci.yml/badge.svg)](https://github.com/ROCm/hipFile/actions/workflows/root-ci.yml)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/11458/badge)](https://www.bestpractices.dev/projects/11458)
[![CodeQL](https://github.com/ROCm/hipFile/actions/workflows/github-code-scanning/codeql/badge.svg)](https://github.com/ROCm/hipFile/actions/workflows/github-code-scanning/codeql)
[![Coverity](https://scan.coverity.com/projects/hipFile/badge.svg)](https://scan.coverity.com/projects/hipFile)

AMD Infinity Storage library that supports IO directly to the GPU

## Building hipFile

See the INSTALL.md file in the project root for a list of supported
hardware and compilers as well as build and install instructions.

### hipify support

We've created a fork of [ROCm/HIPIFY](https://github.com/ROCm/HIPIFY) at
[derobins/HIPIFY](https://github.com/derobins/HIPIFY). Changes to support
hipFile can be found in the `hipFile` branch. Please note that this branch
is a work in progress and may not be fully functional.

A cuFile --> hipFile API map can be found [here](docs/cuFile_API_supported_by_HIP.md)

### fio support

We've created a fork of [axboe/fio](https://github.com/axboe/fio) at
[ROCm/fio](https://github.com/ROCm/fio). Changes to support a
libhipfile engine can be found in the `hipFile` branch. Please note
that this fork is a work in progress and may not be fully functional.

hello-world