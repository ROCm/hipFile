"""
conftest.py – Patch ctypes.CDLL so hipfile can be imported without libhipfile.so.

This mirrors the situation in cufile-python where libcufile.so is loaded at
module level.  On machines without the real library (CI, dev laptops) we
intercept the CDLL call and return a MagicMock.
"""

import ctypes
import sys
from unittest.mock import MagicMock

_real_CDLL = ctypes.CDLL


def _mock_CDLL(name, *args, **kwargs):
    if name == "libhipfile.so":
        return MagicMock()
    return _real_CDLL(name, *args, **kwargs)


# Patch before any test (or fixture) imports hipfile
ctypes.CDLL = _mock_CDLL

# Remove hipfile modules so they reimport with the patched CDLL
for mod in list(sys.modules):
    if mod.startswith("hipfile"):
        del sys.modules[mod]
