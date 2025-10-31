NVIDIA cuFile Compatibility
===========================

hipfile.h/cufile.h Differences
------------------------------

In general, hipfile.h attempts to keep a 1:1 correspondence with the contents
of cufile.h so that hipifying CUDA code is more straightforward. There are
some differences, however.

* hipfile.h does not typedef ``struct sockaddr`` to ``sockaddr_t`` as cufile.h does.

* cufile.h has ``#define cuFileDriverClose cuFileDriverClose_v2``. hipFile has no
  equivalent for this (libcufile does contain both symbols) so both API calls
  will map to ``hipFileDriverClose()``.

* hipfile.h avoids POSIX types that have Windows portability problems:

  - ``long`` is 32 bits on Windows (LLP64) and 64 bits on most POSIX systems
    (LP64). In its place, hipfile.h uses ``int64_t``.
  - hipfile.h uses a platform-independent ``hoff_t`` type in place of ``off_t``
    to avoid problems on Windows, where ``off_t`` is always 32-bit. ``hoff_t``
    is set to ``__int64`` on Windows to match the Windows POSIX layer.
