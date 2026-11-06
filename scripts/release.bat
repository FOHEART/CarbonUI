::; exec sh -c "$(sed -n '/^#__SH__/,$p' "$0" | tr -d '\r')" "$0" "$@" #
@echo off
chcp 65001 >nul
REM  chcp 必须在任何多字节内容之前。
REM  release.bat -- 按版本号唯一来源（include\CarbonUIVersion.h）打 tag 并推送
REM  用法：scripts\release.bat [--force]
REM  行为：
REM    1) 从 include\CarbonUIVersion.h 解析 CARBONUI_VERSION（如 0.2.1）
REM    2) 校验工作树干净（有未提交改动则中止，--force 跳过）
REM    3) 校验本地/远程均不存在同名 tag，创建 annotated tag v<版本> 并推送
REM    4) 推送后 .github/workflows/release.yml 校验 tag 与头文件一致并发布
REM       GitHub Release（资产命名 CarbonUI_v<版本>*.zip）
setlocal EnableDelayedExpansion
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
cd /d "%ROOT%"

set "FORCE="
if /i "%~1"=="--force" set "FORCE=1"

set "VERSION_H=%ROOT%\include\CarbonUIVersion.h"
if not exist "%VERSION_H%" (
    echo [ERROR] not found: %VERSION_H%
    exit /b 1
)

REM 解析锚点行：#define CARBONUI_VERSION "x.y.z"（tokens=3 取带引号串，%%~V 去引号）
set "VER="
for /f "tokens=3" %%V in ('findstr /b /c:"#define CARBONUI_VERSION " "%VERSION_H%"') do set "VER=%%~V"
if not defined VER (
    echo [ERROR] cannot parse CARBONUI_VERSION from include\CarbonUIVersion.h
    exit /b 1
)
set "TAG=v%VER%"

git diff --quiet --exit-code -- >nul 2>nul && git diff --cached --quiet --exit-code -- >nul 2>nul
if errorlevel 1 (
    if not defined FORCE (
        echo [ERROR] working tree not clean. Commit or stash first, or rerun with --force.
        exit /b 1
    )
    echo [WARN] --force: continuing with dirty working tree.
)

git show-ref --tags --verify --quiet "refs/tags/%TAG%" 2>nul
if not errorlevel 1 (
    echo [ERROR] tag %TAG% already exists locally.
    exit /b 1
)
git ls-remote --exit-code --tags origin "refs/tags/%TAG%" >nul 2>nul
if not errorlevel 1 (
    echo [ERROR] tag %TAG% already exists on origin.
    exit /b 1
)

echo [1/2] create tag %TAG% (from include\CarbonUIVersion.h) ...
git tag -a "%TAG%" -m "CarbonUI %TAG%"
if errorlevel 1 exit /b 1

echo [2/2] push tag %TAG% ...
git push origin "%TAG%"
if errorlevel 1 (
    echo [ERROR] push failed, deleting local tag %TAG%
    git tag -d "%TAG%" >nul 2>nul
    exit /b 1
)

echo.
echo [OK] tag %TAG% pushed. GitHub Actions release.yml will build and publish
echo      Release assets named CarbonUI_v%VER%*.zip.
exit /b 0

#__SH__
# POSIX 段
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
cd "$ROOT" || exit 1

FORCE=0
[ "$1" = "--force" ] && FORCE=1

VERSION_H="$ROOT/include/CarbonUIVersion.h"
[ -f "$VERSION_H" ] || { echo "[ERROR] not found: $VERSION_H"; exit 1; }

# 解析锚点行：#define CARBONUI_VERSION "x.y.z"
VER=$(sed -n 's/^#define CARBONUI_VERSION "\(.*\)"/\1/p' "$VERSION_H" | head -n 1)
[ -n "$VER" ] || { echo "[ERROR] cannot parse CARBONUI_VERSION from include/CarbonUIVersion.h"; exit 1; }
TAG="v$VER"

if [ "$FORCE" -ne 1 ]; then
    if ! git diff --quiet --exit-code -- || ! git diff --cached --quiet --exit-code --; then
        echo "[ERROR] working tree not clean. Commit or stash first, or rerun with --force."
        exit 1
    fi
else
    echo "[WARN] --force: continuing with dirty working tree."
fi

if git show-ref --tags --verify --quiet "refs/tags/$TAG" 2>/dev/null; then
    echo "[ERROR] tag $TAG already exists locally."
    exit 1
fi
if git ls-remote --tags origin "refs/tags/$TAG" | grep -q .; then
    echo "[ERROR] tag $TAG already exists on origin."
    exit 1
fi

echo "[1/2] create tag $TAG (from include/CarbonUIVersion.h) ..."
git tag -a "$TAG" -m "CarbonUI $TAG" || exit 1

echo "[2/2] push tag $TAG ..."
if ! git push origin "$TAG"; then
    echo "[ERROR] push failed, deleting local tag $TAG"
    git tag -d "$TAG" >/dev/null 2>&1
    exit 1
fi

echo ""
echo "[OK] tag $TAG pushed. GitHub Actions release.yml will build and publish"
echo "     Release assets named CarbonUI_v$VER*.zip."
exit 0
