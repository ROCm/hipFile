Introduction
============

hipFile is an AMD equivalent to NVIDIA's cuFile API. It is intendended
to be a drop-in, easily HIPify-able replacement for cuFile. Like other
HIP libraries, it transparently maps hipFile API calls to AMD's rocFile
or NVIDIA's cuFile based on whether `__HIP_PLATFORM_AMD__` or `__HIP_PLATFORM_NVIDIA__`
were set when building the library, respectively.

The documentation for hipFile is somewhat sparse as it's a very thin layer.
For API details, see AMD's rocFile or NVIDIA's cuFile API documentation.
