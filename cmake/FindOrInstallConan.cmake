# --- Conan auto-install logic ---
# Uses the .venv Python (pythonlib/.venv) for Conan, since the UCRT64 system
# Python is externally managed (PEP 668) and cannot install packages via pip.
set(CONAN_TOOLCHAIN_PATH "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
if(NOT EXISTS "${CONAN_TOOLCHAIN_PATH}")
    message(STATUS "Conan toolchain not found. Running Conan to install dependencies...")

    # Resolve .venv Python: prefer it over UCRT64 system Python for Conan
    set(_venv_python "${CMAKE_SOURCE_DIR}/pythonlib/.venv/Scripts/python.exe")
    if(EXISTS "${_venv_python}")
        set(_conan_python "${_venv_python}")
        message(STATUS "Using .venv Python for Conan: ${_conan_python}")
    else()
        set(_conan_python "${Python_EXECUTABLE}")
        message(STATUS "Using system Python for Conan: ${_conan_python}")
    endif()

    # Get Python include path
    execute_process(
        COMMAND "${_conan_python}" -c "import sysconfig; print(sysconfig.get_path('include'))"
        OUTPUT_VARIABLE PYTHON_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(ENV{PYTHON_INCLUDE_DIR} "${PYTHON_INCLUDE_DIR}")
    message(STATUS "PYTHON_INCLUDE_DIR set to: $ENV{PYTHON_INCLUDE_DIR}")

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env
            "PYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIR}"
            "${_conan_python}" -m conan install "${CMAKE_SOURCE_DIR}"
                -pr:b=default
                -pr:h=default
                -s build_type=Debug
                --output-folder="${CMAKE_BINARY_DIR}"
                --build=missing
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _conan_result
    )
    if(NOT _conan_result EQUAL 0)
        message(FATAL_ERROR "Conan install failed. Please check your Conan/.venv setup.")
    endif()
endif()

# Conan/pybind11 integration
include(${CONAN_TOOLCHAIN_PATH} OPTIONAL)
