@echo off
chcp 65001 >nul
REM  chcp 必须在任何中文之前！本文件是"深层被 call 的子脚本"，CRLF + chcp 顺序错了
REM  不会影响顶层入口，但自己会解析错位——症状是碎片命令报错（'xxx' is not recognized）。
REM ============================================================================
REM  build_and_test.bat -- 单元测试：配置 + 构建 + ctest
REM  用法：tools\build_and_test.bat [Debug|Release] [ctest -R 正则]
REM  测试构建树固定 out\build\tdd-tests，与主构建树 out\build\x64-* 隔离。
REM ============================================================================
setlocal enabledelayedexpansion

set "ROOT=%~dp0.."
pushd "%ROOT%"
set "ROOT=%CD%"
popd

set "CONFIG=%~1"
if "%CONFIG%"=="" set "CONFIG=Debug"
set "FILTER=%~2"
set "BUILD=%ROOT%\out\build\tdd-tests"

REM -- Visual Studio 环境与生成器：vswhere 探测最新带 C++ 工具链的实例 --------
REM 生成器名从 installationVersion 主版本号推导（该属性必然存在）；
REM catalog_productLineVersion 在 Insiders/预览实例上可能为空，不能用。
REM 否则会出现 VSROOT 指向 VS2026、生成器却写死 17 2022 的自相矛盾
REM （症状：cmake 报 could not find any instance of Visual Studio）。
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSROOT="
set "VS_VER="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -prerelease -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do if not defined VSROOT set "VSROOT=%%I"
    for /f "usebackq delims=" %%V in (`"%VSWHERE%" -prerelease -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) do if not defined VS_VER set "VS_VER=%%V"
)
set "VS_MAJOR="
if defined VS_VER for /f "tokens=1 delims=." %%M in ("%VS_VER%") do set "VS_MAJOR=%%M"
if "%VS_MAJOR%"=="18" (
    set "VS_YEAR=2026"
    set "GENERATOR=Visual Studio 18 2026"
) else if "%VS_MAJOR%"=="16" (
    set "VS_YEAR=2019"
    set "GENERATOR=Visual Studio 16 2019"
) else (
    set "VS_YEAR=2022"
    set "GENERATOR=Visual Studio 17 2022"
)

set "VCVARS=%VSROOT%\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" set "VCVARS=C:\Program Files\Microsoft Visual Studio\%VS_YEAR%\Community\VC\Auxiliary\Build\vcvars64.bat"
if not exist "%VCVARS%" (
    echo [ERROR] vcvars64.bat not found: %VCVARS%
    exit /b 2
)
echo [INFO] VSROOT=%VSROOT%
echo [INFO] installationVersion=%VS_VER% -^> %GENERATOR%
call "%VCVARS%" >nul
if errorlevel 1 (
    echo [ERROR] failed to load Visual Studio environment
    exit /b 2
)

REM -- Qt 前缀：优先 QTDIR 环境变量，否则用本机默认安装路径 -------------------
set "QT_ARGS="
if defined QTDIR set "QT_ARGS=-DCMAKE_PREFIX_PATH=%QTDIR%"

echo [1/3] configure: %BUILD% （%GENERATOR%）
cmake -S "%ROOT%" -B "%BUILD%" -G "%GENERATOR%" -A x64 ^
    -DCMAKE_CONFIGURATION_TYPES="Debug;Release" %QT_ARGS%
if errorlevel 1 (
    echo [ERROR] cmake configure failed
    exit /b 1
)

echo [2/3] build: carbonui_tests carbonui_visual_tests [%CONFIG%]
cmake --build "%BUILD%" --config %CONFIG% --target carbonui_tests carbonui_visual_tests
if errorlevel 1 (
    echo [ERROR] build failed
    exit /b 1
)

echo [3/3] ctest [%CONFIG%]
cd /d "%BUILD%"
if "%FILTER%"=="" (
    ctest --output-on-failure -C %CONFIG%
) else (
    ctest --output-on-failure -C %CONFIG% -R "%FILTER%"
)
if errorlevel 1 (
    echo [FAILED] ctest failed
    exit /b 1
)
echo [OK] all tests passed
exit /b 0
