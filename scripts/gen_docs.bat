:; exec sh -c "$(sed -n '/^#__SH__/,$p' "$0" | tr -d '\r')" "$0" "$@" #
@echo off
chcp 65001 >nul
REM  chcp 必须在任何多字节内容之前。
REM  gen_docs.bat -- Doxygen 文档生成（有告警即失败）
REM  doxygen 路径：优先环境变量 DOXYGEN，默认本机安装路径；Linux 用 PATH
setlocal
if not defined DOXYGEN set "DOXYGEN=D:\autotools\doxygen-1.11.0\doxygen.exe"
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "LOGFILE=%ROOT%\out\doxygen\doxygen_warnings.log"

if not exist "%DOXYGEN%" (
    echo [ERROR] doxygen not found: %DOXYGEN%
    echo         可用 set DOXYGEN=路径 指定，或安装 doxygen 后重试。
    exit /b 2
)

pushd "%ROOT%"
REM 告警日志按运行 doxygen 时的当前目录解析（WARN_LOGFILE 相对路径），必须先到工程根
if not exist "out\doxygen" md "out\doxygen" 2>nul
"%DOXYGEN%" Doxyfile
set "DOX_RC=%errorlevel%"
if exist "doxygen_warnings.log" move /y "doxygen_warnings.log" "out\doxygen\doxygen_warnings.log" >nul 2>nul
if not "%DOX_RC%"=="0" (
    echo.
    echo [FAILED] doxygen failed, exit code: %DOX_RC%
    popd
    exit /b 1
)
REM WARN_AS_ERROR 未在 Doxyfile 打开时，非空告警日志同样判为失败
if exist "%LOGFILE%" for %%S in ("%LOGFILE%") do if not %%~zS EQU 0 (
    echo.
    echo [FAILED] doxygen reported warnings, see %LOGFILE%
    type "%LOGFILE%"
    popd
    exit /b 1
)

echo.
echo [OK] html generated: %ROOT%\docs\html\index.html
start "" "%ROOT%\docs\html\index.html"
popd
exit /b 0

#__SH__
# POSIX 段
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
cd "$ROOT" || exit 1

DOXYGEN_BIN=${DOXYGEN:-doxygen}
if ! command -v "$DOXYGEN_BIN" >/dev/null 2>&1; then
    echo "[ERROR] doxygen not found: $DOXYGEN_BIN" >&2
    echo "        请安装 doxygen（如 sudo apt install doxygen），或用 DOXYGEN=/path/to/doxygen 指定。" >&2
    exit 2
fi

mkdir -p out/doxygen
if ! "$DOXYGEN_BIN" Doxyfile; then
    [ -f doxygen_warnings.log ] && mv -f doxygen_warnings.log out/doxygen/doxygen_warnings.log
    echo ""
    echo "[FAILED] doxygen failed, see out/doxygen/doxygen_warnings.log"
    [ -f out/doxygen/doxygen_warnings.log ] && cat out/doxygen/doxygen_warnings.log
    exit 1
fi
[ -f doxygen_warnings.log ] && mv -f doxygen_warnings.log out/doxygen/doxygen_warnings.log
if [ -s out/doxygen/doxygen_warnings.log ]; then
    echo ""
    echo "[FAILED] doxygen reported warnings, see out/doxygen/doxygen_warnings.log"
    cat out/doxygen/doxygen_warnings.log
    exit 1
fi

echo ""
echo "[OK] html generated: $ROOT/docs/html/index.html"
