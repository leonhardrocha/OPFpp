import os
import sys

def add_windows_runtime_dirs():
    """
    On Windows, add MSYS2/UCRT64 runtime DLL directories to the DLL search path.
    Uses UCRT64_RUNTIME_FOLDER and UCRT64_RUNTIME_LIB_FOLDER from the environment,
    or defaults to standard MSYS2 locations.
    """
    if sys.platform == "win32":
        _runtime_dirs = [
            os.environ.get("UCRT64_RUNTIME_FOLDER", r"D:\msys64\ucrt64\bin"),
            os.environ.get("UCRT64_RUNTIME_LIB_FOLDER", r"D:\msys64\ucrt64\lib"),
        ]
        for _runtime_dir in _runtime_dirs:
            if _runtime_dir and os.path.isdir(_runtime_dir):
                os.add_dll_directory(_runtime_dir)
