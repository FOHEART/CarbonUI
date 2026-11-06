:; exec sh -c "$(sed -n '/^#__SH__/,$p' "$0" | tr -d '\r')" "$0" "$@" #
@echo off
chcp 65001 >nul
REM  chcp 必须在任何多字节内容之前。
REM  sync_agent_harness.bat -- 把 CLI 可执行文件 + agent-harness + SKILL.md 同步到安装目录
REM  用法（Windows）：scripts\sync_agent_harness.bat [Debug|Release] [安装目录]
REM  用法（Linux）  ：sh scripts/sync_agent_harness.bat [Debug|Release] [安装目录]
REM  被 deploy.bat 调用；也可单独跑（给已安装的目录补 harness）。
REM  注意：echo 里不要写裸 "->"（cmd 会当重定向，见 skill 文档 §8）。
setlocal EnableDelayedExpansion
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
cd /d "%ROOT%"

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Release"
set "INSTALL=%~2"
if "%INSTALL%"=="" set "INSTALL=%ROOT%\out\install\x64-%CONFIG%"

echo   汇总 CLI 与 agent-harness 到 %INSTALL%
if not exist "%INSTALL%" md "%INSTALL%" 2>nul

REM CLI 可执行文件：优先主构建树，其次测试构建树（多配置生成器在 <Config> 子目录）
set "CLI_EXE="
for %%F in (
    "%ROOT%\out\build\x64-%CONFIG%\cli\%CONFIG%\carbonui_cli.exe"
    "%ROOT%\out\build\x64-%CONFIG%\cli\carbonui_cli.exe"
    "%ROOT%\out\build\tdd-tests\cli\%CONFIG%\carbonui_cli.exe"
) do (
    if exist %%F if not defined CLI_EXE set "CLI_EXE=%%~fF"
)
if not defined CLI_EXE (
    echo [WARN] 未找到 carbonui_cli.exe，请先 scripts\build.bat / scripts\test.bat。
    exit /b 1
)
copy /y "%CLI_EXE%" "%INSTALL%\" >nul || exit /b 1

REM Qt 运行时与库 DLL 必须跟 exe 同目录，否则装出来的 CLI 起不来（stdout 为空的假死）
for %%I in ("%CLI_EXE%") do set "CLI_DIR=%%~dpI"
for %%F in ("%CLI_DIR%\Qt5*.dll" "%CLI_DIR%\CarbonUI*.dll" "%CLI_DIR%\libCarbonUI*.dll") do (
    if exist "%%F" copy /y "%%F" "%INSTALL%\" >nul
)
if exist "%CLI_DIR%\platforms" (
    robocopy "%CLI_DIR%\platforms" "%INSTALL%\platforms" /E /NFL /NDL /NJH /NJS >nul
    if errorlevel 8 exit /b 1
)

REM agent-harness 目录：排除 Python 缓存；robocopy 退出码小于 8 都算成功
robocopy "%ROOT%\agent-harness" "%INSTALL%\agent-harness" /E /NFL /NDL /NJH /NJS ^
    /XD __pycache__ .pytest_cache /XF "*.pyc" "*.egg-info" >nul
if errorlevel 8 (
    echo [ERROR] robocopy 同步 agent-harness 失败
    exit /b 1
)
copy /y "%ROOT%\SKILL.md" "%INSTALL%\" >nul 2>nul

echo      已同步 carbonui_cli.exe、agent-harness、SKILL.md
exit /b 0

#__SH__
# POSIX 段
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
cd "$ROOT" || exit 1

CONFIG=${1:-Release}
INSTALL=${2:-$ROOT/out/install/x64-$CONFIG}
mkdir -p "$INSTALL"

# CLI 可执行文件：优先主构建树，其次测试构建树（单配置生成器直接在 cli/ 下）
CLI_EXE=""
for f in \
    "$ROOT/out/build/x64-$CONFIG/cli/carbonui_cli" \
    "$ROOT/out/build/tdd-tests/cli/carbonui_cli" \
    "$ROOT/out/build/x64-$CONFIG/cli/Release/carbonui_cli.exe" \
    "$ROOT/out/build/tdd-tests/cli/Release/carbonui_cli.exe"; do
    if [ -z "$CLI_EXE" ] && [ -f "$f" ]; then CLI_EXE="$f"; fi
done
if [ -z "$CLI_EXE" ]; then
    echo "[WARN] 未找到 carbonui_cli，请先 sh scripts/build.bat / scripts/test.bat。"
    exit 1
fi
cp "$CLI_EXE" "$INSTALL/" || exit 1

# 静态构建下 Linux 无需带 Qt 运行时（系统包提供）；动态构建带上库 SO
for d in "$(dirname "$CLI_EXE")"; do
    for so in "$d"/libCarbonUI*.so*; do
        [ -f "$so" ] && cp "$so" "$INSTALL/" || true
    done
done

# agent-harness 目录：复制后清掉 Python 缓存（避免把垃圾带进部署目录）
rm -rf "$INSTALL/agent-harness"
cp -R "$ROOT/agent-harness" "$INSTALL/agent-harness" || exit 1
find "$INSTALL/agent-harness" -type d \( -name __pycache__ -o -name .pytest_cache \) -exec rm -rf {} + 2>/dev/null
find "$INSTALL/agent-harness" -type f \( -name '*.pyc' -o -name '*.egg-info' \) -delete 2>/dev/null
cp "$ROOT/SKILL.md" "$INSTALL/" || exit 1

echo "     已同步 carbonui_cli、agent-harness、SKILL.md"
exit 0
