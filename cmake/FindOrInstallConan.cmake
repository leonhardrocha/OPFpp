# --- Conan auto-install logic ---
set(CONAN_TOOLCHAIN_PATH "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
if(NOT EXISTS "${CONAN_TOOLCHAIN_PATH}")
    message(STATUS "Conan toolchain not found. Running Conan to install dependencies...")
    # Get Python include path using sysconfig
    execute_process(
        COMMAND ${Python_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('include'))"
        OUTPUT_VARIABLE PYTHON_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(ENV{PYTHON_INCLUDE_DIR} "${PYTHON_INCLUDE_DIR}")
    message(STATUS "PYTHON_INCLUDE_DIR set to: $ENV{PYTHON_INCLUDE_DIR}")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env
            "PYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIR}"
            conan install ${CMAKE_SOURCE_DIR} 
                -pr:b=default 
                -pr:h=default 
                -s build_type=Debug 
                --output-folder=${CMAKE_BINARY_DIR} 
                --build=missing     
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _conan_result
    )
    if(NOT _conan_result EQUAL 0)
        message(FATAL_ERROR "Conan install failed. Please check your Conan setup.")
    endif()
endif()

# Conan/pybind11 integration
include(${CONAN_TOOLCHAIN_PATH} OPTIONAL)
