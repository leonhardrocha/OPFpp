#!/bin/bash
# Execute usando: source env_local.sh

PROJECT_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Configuração e injeção do MSYS2/UCRT64 no Windows se o usuário estiver usando um terminal comum (como Git Bash)
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OS" == "Windows_NT" ]]; then
    MSYS_BIN_PATH="/c/msys64/ucrt64/bin"
    MSYS_USR_PATH="/c/msys64/usr/bin"
    
    # Adiciona os caminhos do MSYS2 no PATH caso eles ainda não estejam lá
    if [[ ! "$PATH" == *"$MSYS_BIN_PATH"* ]]; then
        export PATH="$MSYS_BIN_PATH:$MSYS_USR_PATH:$PATH"
    fi

    # Ativação do venv no Windows
    if [ -z "$CONDA_DEFAULT_ENV" ] && [ -z "$VIRTUAL_ENV" ]; then
        if [ -d "$PROJECT_ROOT/.venv" ]; then
            source "$PROJECT_ROOT/.venv/Scripts/activate"
            echo "Ambiente virtual (.venv) ativo!"
        fi
    fi
    
    # Vincula as DLLs compiladas do projeto ao PATH
    export PATH="$PROJECT_ROOT/build_local/lib:$PROJECT_ROOT/build_local/bin:$PATH"
    echo "PATH configurado para Windows (MSYS2/UCRT64)."
else
    # Configuração padrão para Linux Nativo
    if [ -z "$CONDA_DEFAULT_ENV" ] && [ -z "$VIRTUAL_ENV" ]; then
        if [ -d "$PROJECT_ROOT/.venv" ]; then
            source "$PROJECT_ROOT/.venv/bin/activate"
            echo "Ambiente virtual (.venv) ativo!"
        fi
    fi
    export LD_LIBRARY_PATH="$PROJECT_ROOT/build_local/lib:$PROJECT_ROOT/build_local/bin:${LD_LIBRARY_PATH}"
    echo "LD_LIBRARY_PATH configurado para Linux."
fi

export PYTHONPATH="$PROJECT_ROOT:$PROJECT_ROOT/OPFpp/pythonlib:${PYTHONPATH}"
echo "Variáveis de ambiente do OPFpp prontas para execução!"