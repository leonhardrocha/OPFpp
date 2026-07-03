#!/bin/bash
set -e
echo "==== OPFpp PyPI Deployment Script (Linux) ===="
# 1. Determine script and project paths
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_ROOT"
# 2. Check and activate virtual environment
VENV_PATH="pythonlib/.venv"
if [ ! -d "$VENV_PATH" ]; then
    echo "Error: Virtual environment '$VENV_PATH' not found."
    echo "Please create it first: python3.12 -m venv $VENV_PATH"
    exit 1
fi
echo "Activating virtual environment..."
source "$VENV_PATH/bin/activate"
# 3. Verify Python version
PYTHON_VERSION=$(python -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
echo "Using Python: $(which python) (v$PYTHON_VERSION)"
if [ "$PYTHON_VERSION" != "3.12" ]; then
    echo "Warning: Expected Python 3.12, but found $PYTHON_VERSION."
fi
# 4. Ensure packaging tools are installed
echo "Ensuring build and twine are installed..."
python -m pip install --upgrade pip
python -m pip install --upgrade build twine
# 5. Clean old build artifacts
echo "Cleaning old build artifacts..."
rm -rf pythonlib/build pythonlib/dist pythonlib/opfppy.egg-info
# 6. Build the wheel
echo "Building the wheel..."
python -m build --wheel pythonlib/
# 6b. Retag wheel: PyPI rejects raw linux_x86_64 tags (PEP 600)
echo "Retagging wheel for manylinux compatibility..."
python tools/retag_wheel.py pythonlib/dist/
# 7. Upload to PyPI
echo "Preparing to upload to PyPI..."
if [ -z "$PYPI_API_TOKEN" ]; then
    read -sp "Enter PyPI API Token (starts with pypi-): " PYPI_API_TOKEN
    echo ""
fi
if [ -z "$PYPI_API_TOKEN" ]; then
    echo "Error: PyPI API Token is required."
    exit 1
fi
export TWINE_USERNAME="__token__"
export TWINE_PASSWORD="$PYPI_API_TOKEN"
python -m twine upload pythonlib/dist/*
echo "Deployment completed successfully!"
