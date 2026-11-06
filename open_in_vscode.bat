:; exec sh -c "$(sed -n '/^#__SH__/,$p' "$0" | tr -d '\r')" "$0" "$@" #
@echo off
chcp 65001 >nul
setlocal EnableExtensions
for %%I in ("%~dp0.") do set "ROOT_DIR=%%~fI"

where code >nul 2>nul
if errorlevel 1 (
    echo [ERROR] code command not found. Please install VSCode with "Add to PATH".
    exit /b 1
)

code "%ROOT_DIR%"
exit /b %errorlevel%

#__SH__
cd "$(dirname "$0")" || exit 1

if ! command -v code >/dev/null 2>&1; then
    echo "[ERROR] code command not found. Please install VSCode first." >&2
    exit 1
fi

code .
