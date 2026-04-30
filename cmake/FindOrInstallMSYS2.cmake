# --- Automação de Dependências MSYS2 ---
if(MINGW AND EXISTS "/etc/msystem" AND "$ENV{MSYSTEM}" STREQUAL "UCRT64")
    message(STATUS "Detectado ambiente MSYS2 UCRT64. Verificando dependências de sistema...")
    
    # Verifica se o Python do UCRT64 está instalado
    find_program(MSYS_PYTHON NAMES python3 PATHS /ucrt64/bin NO_DEFAULT_PATH)
    
    if(NOT MSYS_PYTHON)
        message(STATUS "Dependências do UCRT64 faltando. Executando pacman...")
        # Executa o script bash através do interpretador do MSYS2
        execute_process(
            COMMAND bash "${CMAKE_SOURCE_DIR}/setup_ucrt64.sh"
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE _pkg_result
            # Não use caminhos que exijam interação
        )
        if(NOT _pkg_result EQUAL 0)
            message(FATAL_ERROR "Falha ao instalar dependências via pacman.")
        endif()
    endif()

    # Agora usamos o FindPython moderno que detectará o Python do UCRT64 automaticamente
    find_package(Python 3 COMPONENTS Interpreter Development REQUIRED)
    
    # Sobrescreve as variáveis do pybind11 para garantir que ele use o Python do MSYS2
    set(PYTHON_EXECUTABLE "${Python_EXECUTABLE}" CACHE INTERNAL "")
    set(PYTHON_INCLUDE_DIR "${Python_INCLUDE_DIRS}" CACHE INTERNAL "")
endif()