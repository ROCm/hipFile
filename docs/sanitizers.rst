sanitizers
==========

hipFile has CMake support for running various sanitizers. There are
two CMake options that control this:

`AIS_USE_SANITIZERS`

This adds `-fsanitize=<sanitizers>` where <sanitizers> includes:

* address
* float-divide-by-zero
* integer
* leak
* local-bounds
* nullability
* undefined
* vptr

`AIS_USE_THREAD_SANITIZER`

This adds `-fsanitize=thread`, which is not compatible with
`AIS_USE_SANITIZERS`.

NOTE: We currently only support clang's sanitizers. We'll add GNU in
      the future.

We also add the following options to the test environment when running
CTest:

`ASAN_OPTIONS=print_stacktrace=1:symbolize=1:fast_unwind_on_malloc=0:malloc_context_size=30`

`UBSAN_OPTIONS=print_stacktrace=1:symbolize=1`

This should give you stack traces on failures, though you will need
sanitizer-built ROCm libraries to track anything that was allocated
in the HIP layer. We fetch and build GoogleTest with the sanitizer
options set instead of using the system GoogleTest when the sanitizers
are enabled.
