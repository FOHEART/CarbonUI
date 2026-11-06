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

echo "[2/3] build: carbonui_tests carbonui_visual_tests carbonui_cli [$CONFIG]"
cmake --build "$BUILD" --target carbonui_tests carbonui_visual_tests carbonui_cli -j"$JOBS" || { echo "[ERROR] build failed"; exit 1; }

echo "[3/3] ctest [$CONFIG]"
cd "$BUILD" || exit 1
if [ -n "$FILTER" ]; then
    # ctest -R 没匹配到任何用例时退出码是 0，直接跑会把"一个用例都没跑到"误报成通过
    # （新用例没编进去、用例名写错都会走到这里）。先用 -N 断言确实匹配到了用例。
    if ! ctest -N -R "$FILTER" 2>/dev/null | grep -qE 'Total Tests: [1-9]'; then
        echo "[ERROR] 过滤器 \"$FILTER\" 没有匹配到任何用例（构建失败或用例名写错）"
        exit 1
    fi
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
