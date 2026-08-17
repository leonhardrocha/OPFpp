# tools/build_opfpy.ps1
# Builds the opfpy Python extension (C++/pybind11) using Conan and CMake.

$ErrorActionPreference = "Stop"

# Determina os caminhos base do projeto
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectDir = (Get-Item (Join-Path $ScriptDir "..")).FullName
Set-Location $ProjectDir

# Definição das variáveis de ambiente e caminhos
$MsysBinDir = "D:/msys64/ucrt64/bin"
$CMake = Join-Path $MsysBinDir "cmake.exe"
$BuildDir = Join-Path $ProjectDir "build/gcc"
$BinDir = Join-Path $ProjectDir "pythonlib/bin"
$VenvPython = Join-Path $ProjectDir "pythonlib/.venv/Scripts/python.exe"

# Resolve o caminho direto para o executável do Conan instalado via pip/uv no .venv
$VenvScripts = Split-Path -Parent $VenvPython
$ConanExe = Join-Path $VenvScripts "conan.exe"

# Injeta a pasta bin do MSYS2 UCRT64 no PATH do Windows temporariamente
$env:PATH = "$MsysBinDir;$env:PATH"

# Validação do CMake do MSYS2
if (-not (Test-Path $CMake)) {
    Write-Error "ERROR: UCRT64 cmake not found at $CMake"
    Exit 1
}

# Validação do Conan do .venv
if (-not (Test-Path $ConanExe)) {
    Write-Error "ERROR: Conan executable not found at $ConanExe"
    Write-Error "       Ensure it is installed in the .venv: uv pip install conan"
    Exit 1
}

# Coleta e exibe as versões das ferramentas
$CMakeVersion = (& $CMake --version | Select-Object -First 1)
$ConanVersion = (& $ConanExe --version | Select-Object -First 1)

Write-Host "==> Using CMake: $CMakeVersion" -ForegroundColor Cyan
Write-Host "==> Using Conan from .venv: $ConanVersion" -ForegroundColor Cyan

# Garante que o diretório de build exista fisicamente
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
}

# --- PASSO CRUCIAL CORRIGIDO: Executa o conan.exe do .venv diretamente ---
Write-Host "==> Running Conan install to generate build profiles in ./build/gcc..." -ForegroundColor Yellow
& $ConanExe install "$ProjectDir" --output-folder="$BuildDir" --build=missing -s build_type=Debug

Write-Host "==> Configuring CMake (preset: gcc)..." -ForegroundColor Green

# Configura o CMake apontando para os arquivos que o Conan acabou de gerar
& $CMake --preset gcc -S "$ProjectDir" -B "$BuildDir"

Write-Host "==> Building opfpy target..." -ForegroundColor Green
# Compila o projeto em modo verboso para capturar falhas no código C++
& $CMake --build "$BuildDir" --target opfpy --verbose

Write-Host "==> Writing Cython dependency manifest..." -ForegroundColor Green
$PydFile = Get-ChildItem -Path $BinDir -Filter "opfpy*.pyd" | Select-Object -First 1

if ($null -ne $PydFile) {
    $ManifestPath = Join-Path $BinDir "opfpy_cython_deps.txt"
    $PydFile.FullName | Out-File -FilePath $ManifestPath -Encoding utf8
    Write-Host "    Manifest: $ManifestPath" -ForegroundColor Gray
}

Write-Host "==> Build complete. Extension output:" -ForegroundColor Green
$PydOutputs = Get-ChildItem -Path $BinDir -Filter "opfpy*.pyd" -ErrorAction SilentlyContinue

if ($null -eq $PydOutputs) {
    Write-Host "    WARNING: No .pyd found in $BinDir" -ForegroundColor Yellow
} else {
    foreach ($file in $PydOutputs) {
        $sizeKB = [math]::Round($file.Length / 1KB, 2)
        Write-Host "    $($sizeKB)KB `t $($file.Name)" -ForegroundColor Gray
    }
}
