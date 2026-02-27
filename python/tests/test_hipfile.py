"""
Tests for hipfile Python bindings.
Run without real AMD hardware using mocks.
"""

import ctypes
import os
import tempfile
import unittest
from unittest.mock import MagicMock, patch, call


# ---------------------------------------------------------------------------
# Mock library factory
# ---------------------------------------------------------------------------

def _make_mock_lib():
    lib = MagicMock()
    from hipfile.bindings import hipFileStatus

    ok = hipFileStatus(); ok.err = 0; ok.cu_err = 0

    lib.hipFileDriverOpen.return_value  = ok
    lib.hipFileDriverClose.return_value = ok
    lib.hipFileHandleRegister.return_value = ok
    lib.hipFileHandleDeregister.return_value = None
    lib.hipFileBufRegister.return_value   = ok
    lib.hipFileBufDeregister.return_value = ok
    lib.hipFileRead.return_value  = 1024
    lib.hipFileWrite.return_value = 1024
    return lib


def _patch_lib(mock_lib):
    """Patch bindings.libhipfile so all convenience functions use the mock."""
    return patch("hipfile.bindings.libhipfile", mock_lib)


# ---------------------------------------------------------------------------
# Low-level bindings tests
# ---------------------------------------------------------------------------

class TestBindings(unittest.TestCase):

    def setUp(self):
        self.lib = _make_mock_lib()
        self.p   = _patch_lib(self.lib)
        self.p.start()

    def tearDown(self):
        self.p.stop()

    def test_driver_open(self):
        import hipfile
        hipfile.hipFileDriverOpen()
        self.lib.hipFileDriverOpen.assert_called_once()

    def test_driver_close(self):
        import hipfile
        hipfile.hipFileDriverClose()
        self.lib.hipFileDriverClose.assert_called_once()

    def test_driver_open_error_raises(self):
        from hipfile.bindings import hipFileStatus, HipFileError, _ck
        bad = hipFileStatus(); bad.err = 7; bad.cu_err = 0
        with self.assertRaises(HipFileError) as cm:
            _ck(bad, "hipFileDriverOpen")
        self.assertIn("hipFileDriverOpen", str(cm.exception))
        self.assertIn("7", str(cm.exception))

    def test_handle_register(self):
        import hipfile
        handle = hipfile.hipFileHandleRegister(3)
        self.lib.hipFileHandleRegister.assert_called_once()

    def test_handle_deregister(self):
        import hipfile
        hipfile.hipFileHandleDeregister(ctypes.c_void_p(0))
        self.lib.hipFileHandleDeregister.assert_called_once()

    def test_buf_register(self):
        import hipfile
        hipfile.hipFileBufRegister(ctypes.c_void_p(0xDEAD), 4096, 0)
        self.lib.hipFileBufRegister.assert_called_once()

    def test_buf_deregister(self):
        import hipfile
        hipfile.hipFileBufDeregister(ctypes.c_void_p(0xDEAD))
        self.lib.hipFileBufDeregister.assert_called_once()

    def test_read(self):
        import hipfile
        n = hipfile.hipFileRead(ctypes.c_void_p(0), ctypes.c_void_p(0x1), 1024, 0, 0)
        self.assertEqual(n, 1024)

    def test_write(self):
        import hipfile
        n = hipfile.hipFileWrite(ctypes.c_void_p(0), ctypes.c_void_p(0x1), 1024, 0, 0)
        self.assertEqual(n, 1024)


# ---------------------------------------------------------------------------
# CuFileDriver singleton
# ---------------------------------------------------------------------------

def _reset_singleton():
    """Clear the _singleton closure's _instances dict for CuFileDriver."""
    from hipfile import hipfile as _mod
    # _singleton wraps CuFileDriver; the _instances dict is in the closure
    _mod.CuFileDriver.__closure__[0].cell_contents.clear()


class TestCuFileDriver(unittest.TestCase):

    def setUp(self):
        self.lib = _make_mock_lib()
        self.p   = _patch_lib(self.lib)
        self.p.start()
        _reset_singleton()

    def tearDown(self):
        _reset_singleton()
        self.p.stop()

    def test_driver_opens_on_init(self):
        import hipfile
        hipfile.CuFileDriver()
        self.lib.hipFileDriverOpen.assert_called_once()

    def test_singleton_returns_same_instance(self):
        import hipfile
        a = hipfile.CuFileDriver()
        b = hipfile.CuFileDriver()
        self.assertIs(a, b)  # same object — that's the singleton contract


# ---------------------------------------------------------------------------
# CuFile
# ---------------------------------------------------------------------------

class TestCuFile(unittest.TestCase):

    def setUp(self):
        self.lib = _make_mock_lib()
        self.p   = _patch_lib(self.lib)
        self.p.start()
        # Suppress CuFileDriver singleton state between tests
        self.p2 = patch("hipfile.hipfile.CuFileDriver", return_value=MagicMock())
        self.p2.start()

        self.tmp = tempfile.NamedTemporaryFile(delete=False)
        self.tmp.write(b"\x00" * 4096)
        self.tmp.close()

    def tearDown(self):
        self.p.stop()
        self.p2.stop()
        os.unlink(self.tmp.name)

    def test_lazy_open(self):
        """__init__ must NOT open the file."""
        import hipfile
        f = hipfile.CuFile(self.tmp.name, "r")
        self.assertFalse(f.is_open)

    def test_open_and_close(self):
        import hipfile
        f = hipfile.CuFile(self.tmp.name, "r")
        f.open()
        self.assertTrue(f.is_open)
        self.lib.hipFileHandleRegister.assert_called_once()
        f.close()
        self.assertFalse(f.is_open)
        self.lib.hipFileHandleDeregister.assert_called_once()

    def test_context_manager(self):
        import hipfile
        with hipfile.CuFile(self.tmp.name, "r+") as f:
            self.assertTrue(f.is_open)
        self.assertFalse(f.is_open)

    def test_double_open_is_noop(self):
        import hipfile
        with hipfile.CuFile(self.tmp.name, "r") as f:
            f.open()
            self.lib.hipFileHandleRegister.assert_called_once()

    def test_double_close_is_noop(self):
        import hipfile
        with hipfile.CuFile(self.tmp.name, "r") as f:
            pass
        f.close()
        self.lib.hipFileHandleDeregister.assert_called_once()

    def test_read_requires_open(self):
        import hipfile
        f = hipfile.CuFile(self.tmp.name, "r")
        with self.assertRaises(IOError):
            f.read(ctypes.c_void_p(0xABCD), 1024)

    def test_write_requires_open(self):
        import hipfile
        f = hipfile.CuFile(self.tmp.name, "r+")
        with self.assertRaises(IOError):
            f.write(ctypes.c_void_p(0xABCD), 1024)

    def test_read_returns_int(self):
        import hipfile
        with hipfile.CuFile(self.tmp.name, "r") as f:
            n = f.read(ctypes.c_void_p(0xABCD), 1024, file_offset=0)
        self.assertEqual(n, 1024)

    def test_write_returns_int(self):
        import hipfile
        with hipfile.CuFile(self.tmp.name, "r+") as f:
            n = f.write(ctypes.c_void_p(0xABCD), 1024, file_offset=0)
        self.assertEqual(n, 1024)

    def test_dev_offset_passed_through(self):
        import hipfile
        with hipfile.CuFile(self.tmp.name, "r") as f:
            f.read(ctypes.c_void_p(0x1), 512, file_offset=128, dev_offset=64)
        # third positional arg to hipFileRead is size=512, then file_offset=128, dev_offset=64
        args = self.lib.hipFileRead.call_args[0]
        self.assertEqual(args[2], 512)   # size
        self.assertEqual(args[3], 128)   # file_offset
        self.assertEqual(args[4], 64)    # dev_offset

    def test_get_handle_when_open(self):
        import hipfile
        with hipfile.CuFile(self.tmp.name, "r") as f:
            self.assertIsInstance(f.get_handle(), int)

    def test_get_handle_when_closed(self):
        import hipfile
        f = hipfile.CuFile(self.tmp.name, "r")
        self.assertIsNone(f.get_handle())

    def test_use_direct_io(self):
        import hipfile
        f = hipfile.CuFile(self.tmp.name, "r", use_direct_io=True)
        f.open()
        f.close()

    def test_bad_mode_raises(self):
        import hipfile
        with self.assertRaises(ValueError):
            hipfile.CuFile(self.tmp.name, "z")

    def test_repr(self):
        import hipfile
        f = hipfile.CuFile(self.tmp.name, "r")
        r = repr(f)
        self.assertIn("CuFile", r)
        self.assertIn("open=False", r)


# ---------------------------------------------------------------------------
# Public API surface
# ---------------------------------------------------------------------------

class TestPublicAPI(unittest.TestCase):

    def test_all_exports_present(self):
        import hipfile
        for name in hipfile.__all__:
            self.assertTrue(hasattr(hipfile, name), f"Missing export: {name}")


if __name__ == "__main__":
    unittest.main()
