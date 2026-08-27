# --- Conan auto-install logic ---

# Conan 2 gera arquivos dentro de subpastas (build/<config>/generators)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Tenta mapear o caminho padrão do Conan 2 primeiro
set(CONAN_TOOLCHAIN_PATH "${CMAKE_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/generators/conan_toolchain.cmake")

# Caso o Conan mude a estrutura ou use o layout antigo, verifica se existe. 
# Se não existir, define o fallback para a checagem inicial.
if(NOT EXISTS "${CONAN_TOOLCHAIN_PATH}")
    if(EXISTS "${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
        set(CONAN_TOOLCHAIN_PATH "${CMAKE_BINARY_DIR}/generators/conan_toolchain.cmake")
    else()
        set(CONAN_TOOLCHAIN_PATH "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    endif()
endif()

# Mantém o resto do IF original que checa o NOT EXISTS
if(NOT EXISTS "${CONAN_TOOLCHAIN_PATH}")
    message(STATUS "Conan toolchain not found. Running Conan to install dependencies...")

    # Resolve .venv Python platform-independently
    if(WIN32)
        set(_venv_python "${CMAKE_SOURCE_DIR}/pythonlib/.venv/Scripts/python.exe")
    else()
        set(_venv_python "${CMAKE_SOURCE_DIR}/pythonlib/.venv/bin/python")
    endif()

    if(EXISTS "${_venv_python}")
        set(_conan_python "${_venv_python}")
        message(STATUS "Using .venv Python for Conan: ${_conan_python}")
    else()
        set(_conan_python "${Python_EXECUTABLE}")
        message(STATUS "Using system Python for Conan: ${_conan_python}")
    endif()

    # Find the conan executable inside the specified .venv folder
    get_filename_component(_venv_dir "${_conan_python}" DIRECTORY)
    find_program(CONAN_EXECUTABLE NAMES conan HINTS
        "${_venv_dir}"
        NO_DEFAULT_PATH
    )
    if(NOT CONAN_EXECUTABLE)
        find_program(CONAN_EXECUTABLE NAMES conan)
    endif()

    if(CONAN_EXECUTABLE)
        set(_conan_cmd "${CONAN_EXECUTABLE}")
        set(_conan_args profile detect --force)
        message(STATUS "Found Conan executable: ${CONAN_EXECUTABLE}")
    else()
        # Fallback to python package runner (requires conan installed as module or wrapper)
        set(_conan_cmd "${_conan_python}")
        set(_conan_args -m conan profile detect --force)
    endif()

    # Ensure Conan has a default profile before install.
    execute_process(
        COMMAND ${_conan_cmd} ${_conan_args}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _conan_profile_result
    )
    if(NOT _conan_profile_result EQUAL 0)
        message(WARNING "Conan profile detect failed; continuing and letting Conan install report details.")
    endif()

    # Get Python include path
    execute_process(
        COMMAND "${_conan_python}" -c "import sysconfig; print(sysconfig.get_path('include'))"
        OUTPUT_VARIABLE PYTHON_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(ENV{PYTHON_INCLUDE_DIR} "${PYTHON_INCLUDE_DIR}")
    message(STATUS "PYTHON_INCLUDE_DIR set to: $ENV{PYTHON_INCLUDE_DIR}")

    if(CONAN_EXECUTABLE)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env
                "PYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIR}"
                "${CONAN_EXECUTABLE}" install ${CMAKE_SOURCE_DIR}
                    -pr:b=default
                    -pr:h=default
                    -s build_type=Debug
                    --output-folder=${CMAKE_BINARY_DIR}
                    --build=missing
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE _conan_result
        )
    else()
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env
                "PYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIR}"
                "${_conan_python}" -m conan install ${CMAKE_SOURCE_DIR}
                    -pr:b=default
                    -pr:h=default
                    -s build_type=Debug
                    --output-folder=${CMAKE_BINARY_DIR}
                    --build=missing
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE _conan_result
        )
    endif()
    if(NOT _conan_result EQUAL 0)
        message(FATAL_ERROR "Conan install failed. Please check your Conan/.venv setup.")
    endif()
endif()

# Conan/pybind11 integration
include(${CONAN_TOOLCHAIN_PATH} OPTIONAL)
