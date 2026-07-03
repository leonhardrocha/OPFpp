Write-Host "==== OPFpp PyPI Deployment Script (Windows) ====" -ForegroundColor Cyan
# 1. Determine paths
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
cd $ProjectRoot
# 2. Add MSYS2 UCRT64 paths to PATH
$MsysPath = "C:\msys64"
$UcrtBin = "$MsysPath\ucrt64\bin"
$UsrBin = "$MsysPath\usr\bin"
if (Test-Path $UcrtBin) {
    Write-Host "MSYS2 UCRT64 detected. Adding to PATH..." -ForegroundColor Green
    $env:PATH = "$UcrtBin;$UsrBin;" + $env:PATH
} else {
    Write-Host "Warning: MSYS2 UCRT64 not found at $UcrtBin. Ensure gcc and ninja are in your PATH." -ForegroundColor Yellow
}
# 3. Check and activate virtual environment
$VenvPath = "pythonlib\.venv"
if (-not (Test-Path $VenvPath)) {
    Write-Error "Virtual environment '$VenvPath' not found. Please create it first: python3.12 -m venv $VenvPath"
    exit 1
}
Write-Host "Activating virtual environment..." -ForegroundColor Green
. "$VenvPath\Scripts\Activate.ps1"
# 4. Verify Python version
$PythonVersion = python -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")'
Write-Host "Using Python: $(Get-Command python | Select-Object -ExpandProperty Source) (v$PythonVersion)"
if ($PythonVersion -ne "3.12") {
    Write-Host "Warning: Expected Python 3.12, but found $PythonVersion." -ForegroundColor Yellow
}
# 5. Ensure packaging tools are installed
Write-Host "Ensuring build and twine are installed..." -ForegroundColor Green
python -m pip install --upgrade pip
python -m pip install --upgrade build twine
# 6. Set compiler environment variables for MSYS2
Write-Host "Enforcing GCC and Ninja toolchain..." -ForegroundColor Green
$env:CC = "gcc"
$env:CXX = "g++"
# 7. Clean old build artifacts
Write-Host "Cleaning old build artifacts..." -ForegroundColor Green
if (Test-Path "pythonlib\build") { Remove-Item -Recurse -Force "pythonlib\build" }
if (Test-Path "pythonlib\dist") { Remove-Item -Recurse -Force "pythonlib\dist" }
if (Test-Path "pythonlib\opfppy.egg-info") { Remove-Item -Recurse -Force "pythonlib\opfppy.egg-info" }
# 8. Build the wheel with Ninja generator
Write-Host "Building the wheel..." -ForegroundColor Green
python -m build --wheel pythonlib/ -Ccmake.args="-GNinja"
# 9. Upload to PyPI
Write-Host "Preparing to upload to PyPI..." -ForegroundColor Green
if (-not $env:PYPI_API_TOKEN) {
    $PasswordInput = Read-Host "Enter PyPI API Token (starts with pypi-) " -AsSecureString
    $BSTR = [System.Runtime.InteropServices.Marshal]::SecureStringToBSTR($PasswordInput)
    $env:PYPI_API_TOKEN = [System.Runtime.InteropServices.Marshal]::PtrToStringAuto($BSTR)
}
if (-not $env:PYPI_API_TOKEN) {
    Write-Error "PyPI API Token is required."
    exit 1
}
$env:TWINE_USERNAME = "__token__"
$env:TWINE_PASSWORD = $env:PYPI_API_TOKEN
python -m twine upload pythonlib\dist\*
Write-Host "Deployment completed successfully!" -ForegroundColor Green
