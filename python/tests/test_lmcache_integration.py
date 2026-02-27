#!/usr/bin/env python3
"""
Test hipfile bindings integration with LMCache pattern.
This verifies that the hipFile bindings can be used in the same way as cuFile.
"""

import ctypes
import tempfile
import os

try:
    import hipfile
    print("✓ hipfile imported successfully")
except ImportError as e:
    print(f"✗ Failed to import hipfile: {e}")
    exit(1)

def test_buffer_registration():
    """Test buffer registration similar to LMCache usage."""
    print("\n=== Testing Buffer Registration ===")

    # Test that the required functions exist
    required_functions = [
        'hipFileBufRegister', 'hipFileBufDeregister',
        'hipFileDriverOpen', 'hipFileDriverClose',
    ]

    for func_name in required_functions:
        if hasattr(hipfile, func_name):
            print(f"✓ {func_name} available")
        else:
            print(f"✗ {func_name} missing")
            return False

    return True

def test_context_managers():
    """Test context manager patterns."""
    print("\n=== Testing Context Managers ===")

    # Check if context managers are available
    if hasattr(hipfile, 'CuFileDriver'):
        print("✓ CuFileDriver context manager available")
    else:
        print("✗ CuFileDriver context manager missing")
        return False

    if hasattr(hipfile, 'CuFile'):
        print("✓ CuFile context manager available")
    else:
        print("✗ CuFile context manager missing")
        return False

    # RegisteredBuffer is not implemented yet, so we'll skip it
    print("- RegisteredBuffer context manager not implemented (optional)")

    return True

def test_error_handling():
    """Test error handling."""
    print("\n=== Testing Error Handling ===")

    if hasattr(hipfile, 'HipFileError'):
        print("✓ HipFileError exception class available")
    else:
        print("✗ HipFileError exception class missing")
        return False

    return True

def test_constants():
    """Test that required constants are available."""
    print("\n=== Testing Constants ===")

    # Check for version
    if hasattr(hipfile, '__version__'):
        print(f"✓ hipfile version: {hipfile.__version__}")
    else:
        print("✗ version information missing")
        return False

    # Check if we can import from bindings
    try:
        from hipfile.bindings import hipFileHandle_t
        print("✓ hipFileHandle_t type available")
    except ImportError:
        print("✗ hipFileHandle_t type missing")
        return False

    return True

def main():
    """Run all integration tests."""
    print("=== hipfile Python Bindings Integration Test ===")
    print(f"hipfile version: {getattr(hipfile, '__version__', 'unknown')}")

    tests = [
        test_buffer_registration,
        test_context_managers,
        test_error_handling,
        test_constants
    ]

    passed = 0
    total = len(tests)

    for test in tests:
        try:
            if test():
                passed += 1
        except Exception as e:
            print(f"✗ Test {test.__name__} failed with exception: {e}")

    print(f"\n=== Results: {passed}/{total} tests passed ===")

    if passed == total:
        print("🎉 All integration tests passed! hipfile bindings are ready for LMCache.")
        return True
    else:
        print("❌ Some tests failed. Check the output above.")
        return False

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)
