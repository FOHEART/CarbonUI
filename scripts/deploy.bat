:; exec sh -c "$(sed -n '/^#__SH__/,$p' "$0" | tr -d '\r')" "$0" "$@" #
@echo off
chcp 65001 >nul
REM  chcp 必须在任何多字节内容之前。
REM  deploy.bat -- 安装 + 产物汇总（CarbonUI 是库工程：汇总头文件与库文件）
REM  用法（Windows）：scripts\deploy.bat [Debug|Release] [构建树目录]
REM  用法（Linux）  ：sh scripts/deploy.bat [Debug|Release] [构建树目录]
setlocal EnableDelayedExpansion
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
cd /d "%ROOT%"

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=all"
if /i "%CONFIG%"=="all" (
    call "%~f0" Debug
    if errorlevel 1 exit /b 1
    call "%~f0" Release
    exit /b !errorlevel!
)

set "BDIR=%~2"
if "%BDIR%"=="" set "BDIR=out\build\x64-%CONFIG%"
if not exist "%BDIR%\CMakeCache.txt" (
    echo [ERROR] 未找到构建树 %BDIR%，请先运行 scripts\build.bat。
    exit /b 1
)

set "INSTALL=%ROOT%\out\install\x64-%CONFIG%"

echo   -^> 安装到 %INSTALL%
cmake --install "%BDIR%" --config %CONFIG% --prefix "%INSTALL%"
if errorlevel 1 (
    echo [ERROR] install failed, exit code: !errorlevel!
    exit /b 1
)

echo   -^> 汇总产物到 %INSTALL%
REM 只清空汇总子目录（幂等重跑），保留 cmake --install 的其它安装产物
if exist "%INSTALL%\include" rd /s /q "%INSTALL%\include"
if exist "%INSTALL%\lib" rd /s /q "%INSTALL%\lib"
md "%INSTALL%\include" 2>nul
md "%INSTALL%\lib" 2>nul

REM 库工程：头文件 + 编译产物（多配置生成器在 <Config>\ 子目录，单配置在构建树根）
copy /y "%ROOT%\include\*.h" "%INSTALL%\include\" >nul 2>nul
set "LIBSRC=%BDIR%\%CONFIG%"
if not exist "%LIBSRC%\*.lib" if not exist "%LIBSRC%\*.dll" set "LIBSRC=%BDIR%"
copy /y "%LIBSRC%\*.lib" "%INSTALL%\lib\" >nul 2>nul
copy /y "%LIBSRC%\*.dll" "%INSTALL%\lib\" >nul 2>nul

set "HDR_COUNT=0"
set "LIB_COUNT=0"
for %%F in ("%INSTALL%\include\*.h") do set /a HDR_COUNT+=1
for %%F in ("%INSTALL%\lib\*.lib") do set /a LIB_COUNT+=1
for %%F in ("%INSTALL%\lib\*.dll") do set /a LIB_COUNT+=1
echo      %INSTALL%：!HDR_COUNT! 个头文件，!LIB_COUNT! 个库文件
if "!HDR_COUNT!"=="0" (
    echo [WARN] 未汇总到头文件，请检查 include\ 目录。
    exit /b 1
)

REM CLI 与 agent-harness：与库产物同目录，harness 靠相对位置自动定位 exe
call "%~dp0sync_agent_harness.bat" %CONFIG% "%INSTALL%"
if errorlevel 1 exit /b 1
exit /b 0

#__SH__
# POSIX 段
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
cd "$ROOT" || exit 1

CONFIG=${1:-all}
if [ "$CONFIG" = "all" ]; then
    sh "$(dirname "$0")/deploy.bat" Debug || exit 1
    sh "$(dirname "$0")/deploy.bat" Release || exit 1
    exit 0
fi

BDIR=${2:-out/build/x64-$CONFIG}
if [ ! -f "$BDIR/CMakeCache.txt" ]; then
    echo "[ERROR] 未找到构建树 $BDIR，请先运行 sh scripts/build.bat。"
    exit 1
fi

INSTALL="$ROOT/out/install/x64-$CONFIG"

echo "  -> 安装到 $INSTALL"
cmake --install "$BDIR" --prefix "$INSTALL" || { echo "[ERROR] install failed"; exit 1; }

echo "  -> 汇总产物到 $INSTALL"
# 只清空汇总子目录（幂等重跑），保留 cmake --install 的其它安装产物
rm -rf "$INSTALL/include" "$INSTALL/lib"
mkdir -p "$INSTALL/include" "$INSTALL/lib"

# 库工程：头文件 + 编译产物（静态 .a / 动态 .so，单配置生成器在构建树根目录）
cp "$ROOT"/include/*.h "$INSTALL/include/" 2>/dev/null
count=0
for f in "$BDIR"/libCarbonUI*.a "$BDIR"/*.so "$BDIR"/*.so.*; do
    [ -f "$f" ] || continue
    cp "$f" "$INSTALL/lib/" || exit 1
    count=$((count + 1))
done

hdr=$(find "$INSTALL/include" -name '*.h' | wc -l | tr -d ' ')
echo "     $INSTALL：$hdr 个头文件，$count 个库文件"
if [ "$hdr" -eq 0 ]; then
    echo "[WARN] 未汇总到头文件，请检查 include/ 目录。"
    exit 1
fi
if [ "$count" -gt 0 ]; then
    echo "     运行链接了 CarbonUI 动态库的程序前可执行：export LD_LIBRARY_PATH=\"$INSTALL/lib:\$LD_LIBRARY_PATH\""
fi

# CLI 与 agent-harness：与库产物同目录，harness 靠相对位置自动定位 exe
sh "$(dirname "$0")/sync_agent_harness.bat" "$CONFIG" "$INSTALL" || exit 1
exit 0
