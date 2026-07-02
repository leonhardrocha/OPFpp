# --- Conan & Dependencies Auto-Install Logic ---
# Inteligente: Funciona perfeitamente COM e SEM Docker (Multiplataforma)

# 1. Fallback de configuracao de Build se nao especificado via CLI
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    message(STATUS "CMAKE_BUILD_TYPE nao especificado. Definindo padrao para: ${CMAKE_BUILD_TYPE}")
endif()

# 2. Resolver caminhos do .venv local
if(WIN32)
    set(_venv_python "${CMAKE_SOURCE_DIR}/pythonlib/.venv/Scripts/python.exe")
    set(_venv_bin "${CMAKE_SOURCE_DIR}/pythonlib/.venv/Scripts")
else()
    set(_venv_python "${CMAKE_SOURCE_DIR}/pythonlib/.venv/bin/python")
    set(_venv_bin "${CMAKE_SOURCE_DIR}/pythonlib/.venv/bin")
endif()

# =====================================================================
# VALIDAÇÃO DO VENV: Evita capturar binarios fantasmas/quebrados copiados no Docker
# =====================================================================
set(_venv_valid FALSE)
if(EXISTS "${_venv_python}")
    # O venv existe e o interpretador eh valido
    set(_conan_python "${_venv_python}")
    set(_venv_valid TRUE)
    message(STATUS "Ambiente .venv valido detectado. Usando Python: ${_conan_python}")
else()
    # O venv nao existe ou esta quebrado (ex: copiado de outra máquina/host no Docker)
    if(NOT Python_EXECUTABLE)
        find_program(Python_EXECUTABLE NAMES python3 python python.exe)
    endif()
    
    if(Python_EXECUTABLE)
        set(_conan_python "${Python_EXECUTABLE}")
    else()
        set(_conan_python "python3")
    endif()
    message(STATUS "Usando o Python do sistema (ignorando .venv isolado): ${_conan_python}")
endif()

# Força todo o restante do projeto a usar o mesmo Python alinhado
set(Python_EXECUTABLE "${_conan_python}" CACHE PATH "Forced Python executable" FORCE)
set(Python3_EXECUTABLE "${_conan_python}" CACHE PATH "Forced Python3 executable" FORCE)

# Se o venv for invalido, limpamos os HINTS para o CMake nao ler lixo local
if(_venv_valid)
    set(_search_hints "${_venv_bin}")
else()
    set(_search_hints "")
endif()

# =====================================================================
# GARANTIA DO CYTHON
# =====================================================================
find_program(CYTHON_EXECUTABLE NAMES cython cython.exe HINTS ${_search_hints})
if(NOT CYTHON_EXECUTABLE)
    message(STATUS "Cython nao encontrado. Instalando automaticamente via pip...")
    if(WIN32)
        execute_process(COMMAND ${_conan_python} -m pip install cython --user)
    else()
        execute_process(COMMAND ${_conan_python} -m pip install cython)
    endif()
    
    unset(CYTHON_EXECUTABLE CACHE)
    find_program(CYTHON_EXECUTABLE NAMES cython cython.exe HINTS ${_search_hints} "/usr/local/bin" "/usr/bin" "$ENV{HOME}/.local/bin")
endif()
if(CYTHON_EXECUTABLE)
    message(STATUS "Cython detectado em: ${CYTHON_EXECUTABLE}")
endif()

# =====================================================================
# GARANTIA DO CONAN
# =====================================================================
set(CONAN_TOOLCHAIN_PATH "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")

if(NOT EXISTS "${CONAN_TOOLCHAIN_PATH}")
    message(STATUS "Conan toolchain nao encontrado. Verificando dependencias...")

    find_program(CONAN_EXECUTABLE NAMES conan HINTS ${_search_hints} "$ENV{VIRTUAL_ENV}/bin" "$ENV{VIRTUAL_ENV}/Scripts")

    if(NOT CONAN_EXECUTABLE)
        message(STATUS "Conan nao encontrado. Instalando automaticamente via pip...")
        if(WIN32)
            execute_process(COMMAND ${_conan_python} -m pip install conan --user)
        else()
            execute_process(COMMAND ${_conan_python} -m pip install conan)
        endif()

        unset(CONAN_EXECUTABLE CACHE)

        if(WIN32)
            find_program(CONAN_EXECUTABLE NAMES conan HINTS 
                ${_search_hints}
                "$ENV{APPDATA}/Python/Python311/Scripts"
                "$ENV{USERPROFILE}/AppData/Roaming/Python/Python311/Scripts"
            )
        else()
            find_program(CONAN_EXECUTABLE NAMES conan HINTS 
                ${_search_hints} "/usr/local/bin" "/usr/bin" "$ENV{HOME}/.local/bin"
            )
        endif()

        if(NOT CONAN_EXECUTABLE)
            find_program(CONAN_EXECUTABLE NAMES conan)
        endif()
        
        if(NOT CONAN_EXECUTABLE)
            message(FATAL_ERROR "Falha critica: Conan foi instalado, mas o CMake nao encontrou o executavel valido.")
        endif()
    endif()

    message(STATUS "Conan detectado e validado em: ${CONAN_EXECUTABLE}")

    # Garante a criacao do perfil do Conan
    execute_process(
        COMMAND ${CONAN_EXECUTABLE} profile detect --force
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _conan_profile_result
    )

    # Captura os diretórios de include do Python correto
    execute_process(
        COMMAND "${_conan_python}" -c "import sysconfig; print(sysconfig.get_path('include'))"
        OUTPUT_VARIABLE PYTHON_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(ENV{PYTHON_INCLUDE_DIR} "${PYTHON_INCLUDE_DIR}")

    # Executa a instalacao das dependencias C++
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env
            "PYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIR}"
            "${CONAN_EXECUTABLE}" install ${CMAKE_SOURCE_DIR}
                -pr:b=default
                -pr:h=default
                -s build_type=${CMAKE_BUILD_TYPE}
                -s compiler.cppstd=20
                --output-folder=${CMAKE_BINARY_DIR}
                --build=missing
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE _conan_result
    )
    
    if(NOT _conan_result EQUAL 0)
        message(FATAL_ERROR "O 'conan install' falhou. Verifique as configuracoes do seu ambiente.")
    endif()
endif()

include(${CONAN_TOOLCHAIN_PATH} OPTIONAL)