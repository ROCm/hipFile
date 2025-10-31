Introduction
============

hipFile is an AMD equivalent to NVIDIA's cuFile API. It is intendended
to be a drop-in, easily HIPify-able replacement for cuFile. Like other
HIP libraries, it transparently maps hipFile API calls to AMD's rocFile
or NVIDIA's cuFile based on whether `__HIP_PLATFORM_AMD__` or `__HIP_PLATFORM_NVIDIA__`
were set when building the library, respectively.

The documentation for hipFile is somewhat sparse as it's a very thin layer.
For API details, see AMD's rocFile or NVIDIA's cuFile API documentation.

Modifying CUDA code for hipFile
-------------------------------
Until the hipify tool has been updated to handle hipFile, the distribution
will contain a source patch to hipify that can be used to transform cuFile
code to the hipFile API. Please see the `README.md` included with the patch
for instructions.
