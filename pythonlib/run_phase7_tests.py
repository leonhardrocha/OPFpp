#!/usr/bin/env python
"""Run Phase 7 tests with proper DLL search path setup."""
import os
import sys

# Add MSYS2/UCRT64 runtime DLL directories on Windows
from windows_runtime_helper import add_windows_runtime_dirs
add_windows_runtime_dirs()

# Add build output to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "bin"))

# Now import and run tests
import unittest
loader = unittest.TestLoader()
suite = loader.discover(os.path.dirname(__file__), pattern="test_phase7_multitypes.py")
runner = unittest.TextTestRunner(verbosity=2)
result = runner.run(suite)
sys.exit(0 if result.wasSuccessful() else 1)
