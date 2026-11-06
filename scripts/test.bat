:; exec sh -c "$(sed -n '/^#__SH__/,$p' "$0" | tr -d '\r')" "$0" "$@" #
@echo off
chcp 65001 >nul
REM  chcp 必须在任何多字节内容之前。
REM  test.bat -- 构建并运行单元测试（独立构建树 out\build\tdd-tests）
REM  用法：scripts\test.bat [Debug|Release] [ctest -R 正则]
setlocal
for %%I in ("%~dp0..") do set "ROOT=%%~fI"
call "%ROOT%\tools\build_and_test.bat" %*
exit /b %errorlevel%

#__SH__
# POSIX 段
ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd) || exit 1
cd "$ROOT" || exit 1

CONFIG=${1:-Debug}
FILTER=${2:-}
BUILD="out/build/tdd-tests"
JOBS=$( (command -v nproc >/dev/null 2>&1 && nproc) || echo 4 )

echo "[1/3] configure: $BUILD"
cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE="$CONFIG" \
      ${QTDIR:+-DCMAKE_PREFIX_PATH="$QTDIR"} || { echo "[ERROR] cmake configure failed"; exit 1; }

echo "[2/3] build: carbonui_tests carbonui_visual_tests [$CONFIG]"
cmake --build "$BUILD" --target carbonui_tests carbonui_visual_tests -j"$JOBS" || { echo "[ERROR] build failed"; exit 1; }

echo "[3/3] ctest [$CONFIG]"
cd "$BUILD" || exit 1
if [ -n "$FILTER" ]; then
    ctest --output-on-failure -R "$FILTER"
else
    ctest --output-on-failure
fi
rc=$?
if [ "$rc" -ne 0 ]; then
    echo "[FAILED] ctest exit code = $rc"
    exit "$rc"
fi
echo "[OK] all tests passed"
