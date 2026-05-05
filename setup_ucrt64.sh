#!/bin/bash
echo "==== Setup UCRT64 ===="

# O --noconfirm impede que o script pare à espera de um "Y"
pacman -Sy --needed --noconfirm \
    mingw-w64-ucrt-x86_64-gcc \
    mingw-w64-ucrt-x86_64-cmake \
    mingw-w64-ucrt-x86_64-ninja \
    mingw-w64-ucrt-x86_64-python \
    mingw-w64-ucrt-x86_64-uv


# Para o pip no MSYS2/UCRT64 (usando a flag de bypass do ambiente gerenciado)
# mkdir pythonlib
# cd pythonlib
# uv venv .venv
# source .venv/Scripts/activate
# uv pip install conan pybind11 --break-system-packages --quiet
# /ucrt64/bin/python -m pip install conan pybind11 --break-system-packages --quiet