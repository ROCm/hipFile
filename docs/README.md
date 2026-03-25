# hipFile Documentation

hipFile is documented using Doxygen markup in the public header
files and Sphinx .rst files.

## Building the Documentation

### Requirements

* CMake >= 3.21
* Doxygen (we use 1.9.8, other versions are untested)
* Python 3.12 (earlier versions might work but are untested)
* The following Python packages:
    * breathe
    * rocm-docs-core
    * sphinx
    * sphinx-rtd-theme

### Generation

Building the documentation is done via CMake by turning on the docs
option (which is off by default).

    `cmake -DAIS_BUILD_DOCS=ON <options> <path/to/source>`

You can then build the `doc` target.

    `cmake --build . --target doc`

The documentation will be in the `docs` subdirectory of the build
directory.

## Adding to the Documentation

### API Documentation (Doxygen)

* Make your markup look like the rest of the file
* Doxygen macros can be found in `docs/doxygen/Doxyfile.in`
* All public API types, functions, etc. MUST have Doxygen markup

### Other Documentation (Sphinx)

All other documentation goes in Sphinx .rst documents. API-specific documentation
belongs in `docs/API` and other documentation goes in `docs`. Be sure to update
`sphinx/_toc.yml.in` if you add any new files.
