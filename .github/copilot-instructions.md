# CarbonUI — Copilot Instructions

Lightweight **Qt 5 Widgets** styling/helper library (C++, CMake). It ships no application but **does ship a GoogleTest suite** (`tests/`); it is consumed as a library (`add_subdirectory` / CPM) by Qt apps — this checkout lives at `third_party/CarbonUI` of a larger Qt application, so **public helper signatures are an API contract** for that app.

## Build

The build was validated with the Visual Studio 2022 generator against `D:/Qt5.15.2/5.15.2/msvc2019_64`.

> Windows 日常开发可直接用 `scripts\build.bat` / `scripts\test.bat`（详见下文「## scripts/ — 一键工作流脚本」一节），不必手敲 cmake。

```powershell
# configure (static by default; add -DBUILD_CARBONUI_SHARED=ON for a DLL)
cmake -S . -B build -DCMAKE_PREFIX_PATH="D:/Qt5.15.2/5.15.2/msvc2019_64" -DBUILD_CARBONUI_SHARED=ON

# build everything
cmake --build build --config Release

# build only the library target (the closest thing to a "single unit" build)
cmake --build build --config Release --target CarbonUI

# build + run the unit tests (standalone checkouts default BUILD_CARBONUI_TESTS=ON)
cmake --build build --config Release --target carbonui_tests carbonui_visual_tests
ctest --test-dir build -C Release --output-on-failure
ctest --test-dir build -C Release -R "Unit.RadioStyle"     # one state suite
ctest --test-dir build -C Release -R "^Visual\."           # render evidence only
```

- `.vscode`-driven preset workflow: `CMakePresets.json` (hidden `Qt` preset feeding `CMAKE_PREFIX_PATH` from `$env{QTDIR}`) + the committed `CMakeUserPresets.json` (`Debug-x64` / `Release-x64` → `out/build/{debug,release}`, Ninja, hardcoded `QTDIR`). Presets are checked in, so preset edits affect everyone; don't "fix" the hardcoded path casually.
- Qt resolution: `CMakeLists.txt` probes `find_package(Qt5Core QUIET)` and derives `Qt5_DIR` from `Qt5Core_DIR`, so `CMAKE_PREFIX_PATH` or a `Qt5_DIR` cache entry both work.
- **Tests are opt-in for consumers**: `BUILD_CARBONUI_TESTS` defaults to `ON` only when CarbonUI is the top-level project, so `add_subdirectory(CarbonUI)` in the host app never pulls GoogleTest.

### Tests (TDD loop)

- `tests/unit/*.cpp` — **state suites**, one GoogleTest file per widget family. They assert what the helper *configures* (`styleSheet()` text, geometry, child widgets, cursors/fonts, post-animation visibility). Run headless on `offscreen`; fast; no fonts needed.
- `tests/visual/carbonui_render_test.cpp` — **render-evidence suite**. It paints the styled widgets through the real pipeline (`QWidget::grab()` → QPainter → raster) and asserts **pixels**: `#009ee8` fill, corners clipped by `border-radius` (a magenta backdrop proves the corner is transparent), selection band `#0078d7`, checkbox indicator colour per state, and that the caption is actually drawn. PNGs land in `<test exe dir>/ui-artifacts/` so a human can eyeball the result.
- Never call `show()` in tests: `grab()` renders offscreen, so nothing pops up. The visual suite picks the native QPA backend (`windows` on Win32) because **`offscreen` has an empty font database on Windows** (`QFontDatabase().families()` == 0) and would paint no text at all; the text assertion `GTEST_SKIP()`s when fonts are missing.
- Both suites share `tests/support/qt_test_support.h`: `EXPECT_QSS_CONTAINS`, `spinUntil()`, `renderToArtifact()`, `pixelName()`, `distinctColourCount()`, `fontsAvailable()`.
- Each exe owns its `main` (`unit/main.cpp`, `visual/main.cpp`): `InitGoogleTest` first, then `QApplication`, then `RUN_ALL_TESTS()`; link `GTest::gtest` (never `gtest_main`).
- Headless by default: `QT_QPA_PLATFORM` is set in both mains and again via `gtest_discover_tests(... PROPERTIES ENVIRONMENT ...)`. On Windows the POST_BUILD step copies the Qt DLLs and `platforms/*.dll` next to each test exe, so `ctest` works without `QTDIR` on `PATH`.
- GoogleTest is fetched with `FetchContent` (`GTEST_TAG`, default `v1.15.2`; override with `-DGTEST_REPO=...` or `-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=...` for offline builds). Tests compile as **C++14** (gtest requirement) while the library stays C++11.
- **Never sleep in a test**: wait for state with `carbonui_test::spinUntil(predicate)`.
- Test-authoring traps hit in practice: `QRadioButton` siblings in one parent are **auto-exclusive** (call `setAutoExclusive(false)` for segmented bars); QSS `min-width` **grows the widget past `setGeometry()`**, so derive sample points from `minimumSizeHint()`/`geometry()`; glyph antialiasing leaks colour into centre samples, so sample in padding, not on the caption.

### Doxygen

- `Doxyfile` at the repo root drives the docs: `D:\autotools\doxygen-1.11.0\doxygen.exe Doxyfile`, output in `docs/html`, warnings in `doxygen_warnings.log` (repo root).
- Acceptance bar is **zero warnings**. `TEST()` / `TEST_F()` are listed in `EXCLUDE_SYMBOLS` because doxygen would otherwise parse every gtest case as an undocumented function; do not turn on `MACRO_EXPANSION` (it makes statement-level `EXPECT_*` calls look like declarations).
- Comment rules (from `D:\autotools\skills\代码注释规范_Doxygen.md`): every file needs an `@file` block, every function `@brief` + `@param`/`@return` (no `@return` on `void`), every enum value a `///<`, `#` in colour literals escaped as `\#`, and a space after `@ref`/`@p` before Chinese text.

### Verification limits (important)

- `ctest` now really runs the suite; "tests pass" is a valid claim **only** after `ctest` output shows `100% tests passed`.
- There is **no formatter or linter config in the repo** (README mentions `.clang-format` — it does not exist).
- README 已于 2026-11-06 与代码对齐（C++11、真实头文件清单、`out/build/x64-*` 构建布局）；改 CMake 选项或输出目录时记得同步 README，不要重新引入 `lib/Debug` 这类已废弃的说法。
- Write C++11-compatible code. Include a real `#include` for everything used; headers include concrete Qt modules (`<QtWidgets/QPushButton>`, `<QtCore/QEvent>`) rather than `<QtWidgets>`.

## 本机可用工具（D:\autotools）

这些工具已装在本机，可按需直接调用（**当前电脑无加密软件，新建文件可直接写入，不需要临时文件中转**）：

| 工具 | 用途 | 位置 |
| --- | --- | --- |
| pandoc | 文档格式转换（Markdown ↔ Word/HTML/PDF） | `D:\autotools\Pandoc` |
| wkhtmltopdf | HTML 转 PDF / 图片 | `D:\autotools\wkhtmltopdf\bin` |
| ffmpeg | 音视频转码、剪辑与流处理 | `D:\autotools\ffmpeg\bin` |
| python | Python 运行环境 | `C:\python` |
| Gost | 端口转发 / 代理隧道 | `D:\autotools\gost` |
| wget | 命令行文件下载 | `D:\autotools\wget` |
| aria2 (aria2c) | 多协议多线程下载（HTTP/HTTPS/FTP/BT/磁力） | `D:\autotools\aria2-1.37.0-win-64bit-build1` |
| MuPDF (mutool) | PDF 查看与处理 | `D:\autotools\mupdf` |
| iperf3 | 网络带宽与性能测试 | `D:\autotools\iperf` |
| pdftohtml (Poppler) | PDF 转换（pdftotext / pdftoppm / pdftocairo / pdfimages） | `D:\autotools\pdftohtml` |
| claude-skills | 技能包（pdf / pptx / video-downloader） | `D:\autotools\claude-skills` |
| 7-Zip Extra (7za) | 命令行压缩 / 解压 | `D:\autotools\7z` |
| Doxygen (doxygen / doxywizard) | 源码文档生成（本项目 `Doxyfile` 用它跑） | `D:\autotools\doxygen-1.11.0` |
| RhinoAI 配置包 | Rhino 8 建模的 MCP 连接 + 技能包 | `D:\autotools\rhinoai` |
| gallery-dl | 命令行图集下载 | `D:\autotools\gallery_dl` |
| Crawl4AI | 网页爬取与结构化提取技能包 | `D:\autotools\crawl4ai` |

与开发流程直接相关的两个：**Doxygen**（生成 `docs/html`，见上一节）和 **python**（跑一次性脚本，例如批量改写注释/校验编码）。

## Architecture

Two independently usable pieces, no shared state, no namespaces (all globals in the `CarbonUI` target):

1. **`CarbonUIClass` — a stateless static facade** (`include/CarbonUIClass.h`). Every `applyXStyle` / `setupXStyle` helper takes a widget pointer, calls `setStyleSheet()` with an inline QSS string, and may also mutate geometry/fonts/cursors/validators. The class has no data members. Implementations are **split by widget family** in `src/`: `CarbonUIClass.cpp` (everything generic) and `CarbonUIClass_QRadioButton.cpp` (radio-specific because it needs child-label plumbing). Add new widget styling as `src/CarbonUIClass_<Widget>.cpp`, declared on the same class.
2. **`QGroupBoxCollapser`** (`include/groupboxcollapser.h`) — a `QObject` *controller*, not a widget. `installIntoGroupBox()` takes an already-populated `QGroupBox`, moves its layout into a `QGroupBoxBody` content widget, replaces the title with a checkable `QToolButton` parked in the layout's **menu-bar slot**, and animates `maximumHeight` (180 ms, `InOutCubic`) on toggle. It owns the box (parented `QObject`) and returns a heap object the caller keeps alive. An event filter on the group box re-syncs the button text from `groupBox->title()` and clears the group box title again, so callers can keep calling `setTitle()` after installation.

**Symbol export**: `include/carbonui_global.h` defines `CARBONUI_DLL_API` from `CARBONUI_DLL_EXPORT` (set `PRIVATE` when shared) / `CARBONUI_STATIC_LIB` (set `PUBLIC` when static). Every new public class needs `CARBONUI_DLL_API`, otherwise static-vs-shared consumers break in different ways.

**Qt codegen is manual — this is the most common way to break a build.** `cmake/qt.cmake` shims `qt_standard_project_setup()` to force `CMAKE_AUTOMOC/AUTOUIC/AUTORCC` **OFF**, and `CMakeLists.txt` runs `qt5_wrap_cpp` / `qt5_wrap_ui` / `qt5_add_resources` over explicit lists. Therefore:

- a new header with `Q_OBJECT` must be added to `PROJECT_HEADERS` or `moc` never runs (link errors on `staticMetaObject`/`vtable`);
- new sources → `PROJECT_SOURCES`, `.ui` → `PROJECT_UI_FILES` (currently empty), `.qrc` → `PROJECT_RESOURCES`;
- headers without `Q_OBJECT` produce harmless `No relevant classes found` moc notes — that is expected output, not a warning to fix.

## Conventions

- **Styling is per-widget QSS, not a theme system.** Each helper owns its own complete style string; `applyTooltipStyle()` is the only global one (it appends to `qApp->styleSheet()`). There is no central palette header — colors are literals in each QSS string. Match the existing dark palette: accent `#009ee8` (hover `#00b0ff`, pressed `#007bb3`), surfaces `#272727` / `#2e2e2e` / `#2f2f2f` / `#3b3b3b`, text `#cdcdcd` / `#ffffff`, muted text `#676767`, borders `#5f5f5f` / `#636363`, selected row `#0078d7`.
- **Native indicators are disabled, not styled**: helpers set `::indicator { width: 0px; height: 0px; }` (and hide drop-downs/arrows) and re-create the visual with child widgets, because QSS cannot express the checked/hover visuals the design needs.
- **`applyRadioStyle()` / `applyCheckStyle()` move the widget text out of the widget.** The radio text goes into a child `QLabel` named `<objectName>_label`, the checkbox indicator into `<objectName>_indicator`, and the widget's own `text()` is cleared. A hidden event filter (`StyledRadioTextSyncFilter`) re-copies `QRadioButton::text()` into that label on paint/resize/polish. Consequences: reading `rb->text()` after styling returns an empty string, and any code that needs to *change* the caption must `setText()` on the button (the filter syncs it) rather than on the label. The `_carbonui_radio_sync_installed` dynamic property guards against double-installing the filter — keep that idiom when adding idempotent styling.
- **Null-guard every public helper** with an early `if (!w) return;`; the API is called from app code with widgets that may be absent.
- **Hover/state logic lives in local `QObject` event filters** defined in the `.cpp` (anonymous namespace or function-local classes) that reparent themselves to the widget. Follow that pattern instead of subclassing Qt widgets.
- **Resources**: `Resource.qrc` uses prefix `/img`, so URLs are `:/img/Resources/img/arrow_down.png` (as in `setupComboBoxStyle`). `applyTreeViewStyle()` uses `:/images/...`, which does not resolve — don't copy that path.
- **Every source file starts with the MPL-2.0 comment header.** Indentation is inconsistent between files (4 spaces in `CarbonUIClass.cpp`, tabs in `CarbonUIClass_QRadioButton.cpp` and the group box code) — match the file you are editing instead of reformatting.
- `include/` is the only public include root (`PUBLIC $<BUILD_INTERFACE:...>/include`); internal helpers stay in `.cpp` files, and there are no install rules or package config — the library is meant to be consumed in-tree.

## scripts/ — 一键工作流脚本

所有 `.bat` 都是**双解释器单文件**（`::; exec sh …` 首行 + `#__SH__` 分界）：Windows 走 cmd 段，Linux 用 `sh scripts/xxx.bat` 跑 POSIX 段。文件为 UTF-8，cmd 段开头 `chcp 65001`——**中文注释必须出现在 chcp 之后**。

| 脚本 | 功能 |
| --- | --- |
| `scripts\build.bat [2022\|2026] [Debug\|Release]` | 配置 + 编译（缺省两配置都编）。VS 多配置生成器，构建树 `out\build\x64-<cfg>`；Qt 前缀取 `QTDIR` 环境变量，否则回落本机默认 `D:\Qt5.15.2\5.15.2\msvc2019_64` |
| `scripts\test.bat [Debug\|Release] [ctest -R 正则]` | TDD 一键循环，用**独立测试构建树** `out\build\tdd-tests`：configure → build `carbonui_tests carbonui_visual_tests carbonui_cli` → `ctest`。带过滤器时先 `ctest -N` 断言确实匹配到用例，防止「空匹配误报通过」（新用例没编进去/用例名写错会在这里拦住） |
| `scripts\deploy.bat [Debug\|Release] [构建树]` | `cmake --install` 到 `out\install\x64-<cfg>` 并汇总头文件/库文件（只清空汇总子目录，幂等重跑），随后调用 `sync_agent_harness.bat` |
| `scripts\sync_agent_harness.bat [Debug\|Release] [安装目录]` | 把 `carbonui_cli.exe` + `agent-harness/` + `SKILL.md` 同步到安装目录（deploy 内部调用；也可单独跑给已装目录补 harness） |
| `scripts\gen_docs.bat` | Doxygen 文档生成：从 `include\CarbonUIVersion.h` 解析版本注入 `$(CARBONUI_VERSION)`；doxygen 路径取 `DOXYGEN` 环境变量，默认 `D:\autotools\doxygen-1.11.0`；告警日志在 `out\doxygen\doxygen_warnings.log`，**有告警即失败**（非 0 退出码） |
| `scripts\package.bat [版本号]` | 打可分发源码 ZIP 到 `out\pack`（实际逻辑唯一在 `tools\package_source.ps1`）；版本缺省解析自 `CarbonUIVersion.h` |
| `scripts\release.bat [--force]` | 发布：校验工作树干净（`--force` 跳过）→ 校验本地/远程无同名 tag → 创建并推送 annotated tag `v<版本>`（版本解析自 `CarbonUIVersion.h`）→ `release.yml` 校验 tag 与头文件一致后发布 GitHub Release（资产 `CarbonUI_v<版本>*.zip`） |
| `scripts\git_pull_latest.bat` | `git fetch` + `git pull origin main`，结尾打印最近 5 条提交 |

### scripts\ci\ — GitHub Actions 查询与运维（PowerShell 实现 + 同名 .bat 入口）

公共设施在 `ci_common.ps1`（dot-source 引入）：`Resolve-GhCli`（`GH_CLI` 环境变量 > `D:\autotools\gh_windows_amd64\bin\gh.exe` > PATH）、`Set-GhToken`（`GH_TOKEN`/`GITHUB_TOKEN` > `git credential fill` 取 Windows 凭据管理器令牌）、`Resolve-Repo`（参数 > `GH_REPO_DEFAULT` > origin URL）。**该目录所有文件必须纯 ASCII**——PowerShell 5.1/cmd 把无 BOM 文件按 ANSI 读，中文会破坏解析；脚本退出码必须正确传播（供 CI 链式调用）。

- `check_ci.bat` — 查询/等待 workflow runs：不带参数列出最近 run；`-Watch` 等最新（或 `-RunId` 指定）run 结束；`-RunId <id>` 打印该 run 的失败日志摘要
- `pull_artifacts.bat` — 下载 run 产物（`ui-artifacts` 截图等）到 `out\ci-artifacts`（每次清空重下，`gh run download` 不覆盖已存在文件）；`-Name ui-artifacts-*` 按名过滤
- `refresh_baselines.bat` — **重录截图基线的标准流程**：拉取 CI run 的 ui-artifacts，把 windows 产物覆盖到 `tests\visual\baseline\windows`、ubuntu 产物覆盖到 `baseline\linux`（跳过 `*-diff.png`）；`-DryRun` 只列不拷。覆盖后 `git diff --stat` 审查 → 提交 → 让 CI 复验
- `rerun_failed.bat` — 只重跑指定 run（缺省最新）的失败 jobs；仅对终态 run 有效，需 workflow/repo 权限；`-DryRun` 预览

### scripts\wsl\ — WSL 预验证

`wsl.bat [sync|build|test|docs|verify]`：把仓库同步到 WSL 家目录副本（默认 `~/CarbonUI`）后跑同一套脚本的 POSIX 分支，本地预验证、不必等 CI 红一轮。`verify`（缺省动作）依次 build → test → docs，任一失败即停。环境变量：`WSL_DISTRO`（默认 `Ubuntu-24.04`）、`WSL_DEST`（副本目录）、`WSL_QTDIR`（WSL 内 Qt 前缀，缺省交给 `find_package`）。其余 `wsl/*.sh`（`lib/sync/build/test/docs.sh`）是它的实现细节，不直接调用。踩坑记录：`wsl.exe -d` 参数**不吃引号**（写 `-d "%NAME%"` 会报 `WSL_E_DISTRO_NOT_FOUND`）。

## CI

`.github/workflows/cmake-multi-platform.yml` builds Release on `ubuntu-latest` (Qt 5.14.2) and `windows-latest` (Qt 5.15.2, MSVC) with `BUILD_CARBONUI_SHARED` ON/OFF, then runs `ctest`, which now executes the real GoogleTest suite (fetched with `FetchContent`, so CI needs network access to GitHub). Note the configure step passes `-DBUILDCARBONUI_SHARED=...` (missing underscore), which is not the real option name — the CI matrix therefore always builds **static**. Verify shared builds locally; CI won't catch shared-only failures.
