rocFile hipify README
---------------------

This is a source patch that updates hipify so it can convert the cuFile
API to hipFile. This is a diff against the `rocm-7.0.1` tag, but can
probably be applied against other recent versions of hipify since it's
mainly appending things to the ends of lists of API calls, types, etc.

To apply:

1. Get the hipify source from `https://github.com/ROCm/HIPIFY` and check out the `rocm-7.0.1` tag.
2. Copy the `hipfile_hipify.diff` file to the hipfiy source root
3. Run `patch -p1 < hipfile_hipify.diff` to add the hipFile changes
4. Configure and build normally

The produced executable will be able to convert cuFile code to hipFile.
You can get some sample programs to convert from NVIDIA's MagnumIO
GitHub repository:

`https://github.com/NVIDIA/MagnumIO`

The cuFile examples are in the `gds/samples` directory.
