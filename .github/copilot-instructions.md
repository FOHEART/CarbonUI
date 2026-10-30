# CarbonUI — Copilot Instructions

Lightweight **Qt 5 Widgets** styling/helper library (C++, CMake). It ships no application but **does ship a GoogleTest suite** (`tests/`); it is consumed as a library (`add_subdirectory` / CPM) by Qt apps — this checkout lives at `third_party/CarbonUI` of a larger Qt application, so **public helper signatures are an API contract** for that app.

## Build

The build was validated with the Visual Studio 2022 generator against `D:/Qt5.15.2/5.15.2/msvc2019_64`.

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
- README drift: it claims C++17 (project sets `CMAKE_CXX_STANDARD 11`) and a `lib/Debug`/`lib/Release` artifact layout (the project sets no output dirs — artifacts land in the build tree). Trust `CMakeLists.txt`.
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

## CI

`.github/workflows/cmake-multi-platform.yml` builds Release on `ubuntu-latest` (Qt 5.14.2) and `windows-latest` (Qt 5.15.2, MSVC) with `BUILD_CARBONUI_SHARED` ON/OFF, then runs `ctest`, which now executes the real GoogleTest suite (fetched with `FetchContent`, so CI needs network access to GitHub). Note the configure step passes `-DBUILDCARBONUI_SHARED=...` (missing underscore), which is not the real option name — the CI matrix therefore always builds **static**. Verify shared builds locally; CI won't catch shared-only failures.
