# CarbonUI

Lightweight Qt Widgets UI helpers library

## Overview

`CarbonUI` provides a small collection of UI helper widgets and styling utilities for Qt Widgets-based applications. The project is written in C++17 and built with CMake (Ninja generator supported).

## Features

- Helper styles for radio buttons, line edits, checkboxes and buttons
- `QGroupBox` collapse/expand helper with animation
- Clean include/src layout for easy integration

## Prerequisites

- C++17 compatible compiler
- CMake >= 3.16
- Ninja (recommended generator)
- Qt5 (Widgets, Core, Gui)

Install Qt5 on your platform (examples):

- Ubuntu: `sudo apt install qtbase5-dev qttools5-dev`
- macOS (Homebrew): `brew install qt@5`
- Windows: use the Qt Online Installer or MSYS/Chocolatey packages

## Build

From the project root:

```sh
mkdir -p build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

To build a Debug configuration:

```sh
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```

Notes:
- The library target defines `CARBONUI_LIB` during the build so headers export symbols correctly.
- Build outputs (DLL/.lib/.so) are placed under `lib/Debug` or `lib/Release` depending on the configuration.

## Layout

- `include/` — public headers
- `src/` — source files
- `tests/` — `unit/` state suites, `visual/` render-evidence suite, `support/` shared helpers
- `docs/` — generated Doxygen HTML (not committed; created by `doxygen Doxyfile`)
- `lib/` — configured output directory for built artifacts (`lib/Debug`, `lib/Release`)

## Usage

Link against the `CarbonUI` library from your CMake project:

```cmake
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
add_subdirectory(path/to/CarbonUI)
# or link to the built library target
target_link_libraries(MyApp PRIVATE CarbonUI Qt5::Widgets Qt5::Core)
```

Include headers with:

```cpp
#include <CarbonUI.h>
#include <CarbonUIVersion.h>
#include <modifyui.h>
#include <groupboxcollapser.h>
```

## Tests

Two GoogleTest executables, answering two different questions:

| Target | ctest prefix | Platform | What it proves |
| --- | --- | --- | --- |
| `carbonui_tests` | `Unit.*` | `offscreen` | the helper **applied the intended configuration** — `styleSheet()` text, geometry, child widgets, fonts/cursors, visibility after the 180 ms animation |
| `carbonui_visual_tests` | `Visual.*` | native (`windows` on Win32) | it **actually looks right when painted** — pixel colours, clipped corners, checkbox indicator, selection band, and that captions are really drawn |

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt5       # tests ON by default in a standalone checkout
cmake --build build --config Release --target carbonui_tests carbonui_visual_tests
ctest --test-dir build -C Release --output-on-failure

ctest --test-dir build -C Release -R "Unit.RadioStyle"     # one state suite
ctest --test-dir build -C Release -R "^Visual\."           # render evidence only
build/tests/Release/carbonui_visual_tests --gtest_filter=RenderEvidence.Segment* --gtest_brief=1
```

Notes:

- **Nothing pops up.** The visual suite never calls `show()`; it renders with `QWidget::grab()` (QPainter → raster) into a `QImage`. Each render is saved as PNG in `<test exe dir>/ui-artifacts/`, so you can open the real picture instead of trusting an assertion:
  `build/tests/Release/ui-artifacts/01-button-round-all.png` … `13-groupbox-collapsed.png`.
- **Why the visual suite needs the native backend**: with `QT_QPA_PLATFORM=offscreen` on Windows the font database is empty (`QFontDatabase().families()` == 0), so no glyph is drawn at all and every "text" assertion would be meaningless. The visual `main()` therefore defaults to `windows`; text-dependent cases `GTEST_SKIP()` when fonts are unavailable, so the suite still runs headless on other platforms.
- **Rounded corners must be sampled on a parent**: outside a widget's `border-radius` is transparent, so only the parent's backdrop colour proves the corner was clipped. The suite paints the host widget magenta `#ff00ff` for exactly this reason — grabbing a parentless widget makes all four corners look "painted".
- GoogleTest is fetched with `FetchContent` (`GTEST_TAG`, default `v1.15.2`). For an offline build point at a local checkout: `-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=D:/deps/googletest`.
- **Screenshot baselines are CI-rendered**: `tests/visual/baseline/<platform>/*.png` are produced on the GitHub runners (`windows-latest`, `ubuntu-24.04`). The visual test entry point loads the bundled `Resources/fonts/HarmonyOS_Sans_SC_Regular.ttf` as the default font (same TTF on every machine, so glyph metrics match) and pins DPI/scale, keeping pixels reproducible. Locally the `Screenshot.*` suite is skipped unless you opt in (`CARBONUI_SCREENSHOT_LOCAL=1`) or point `CARBONUI_SCREENSHOT_BASELINE` at your own recorded baselines. Re-record after an intentional UI change: run the suite with `CARBONUI_SCREENSHOT_RECORD=force`, then let CI regenerate the committed baselines from its uploaded `ui-artifacts`.
- Consumers that pull CarbonUI in via `add_subdirectory` get `BUILD_CARBONUI_TESTS=OFF` by default, so no test dependency leaks into the host app.
- Adding a case: create/extend a suite in `tests/unit/` (state) or `tests/visual/` (pixels), then add the file to `tests/CMakeLists.txt`. Name cases `TEST(被测单元, 行为_条件)` and assert observable results instead of implementation details.
- Test-authoring traps worth remembering: sibling `QRadioButton`s are **auto-exclusive** by default (a segmented bar needs `setAutoExclusive(false)`); QSS `min-width` **grows a widget past `setGeometry()`**, so derive sample points from `minimumSizeHint()`; glyph antialiasing tints centre samples, so sample in padding rather than on the caption.

## Versioning & Release

The **single source of truth** for the version is `include/CarbonUIVersion.h` (`CARBONUI_VERSION_MAJOR/MINOR/PATCH`, currently `0.2.1`). Bump the version by editing that header only — everything else follows automatically:

| Consumer | Mechanism |
| --- | --- |
| DLL file properties (FileVersion / ProductVersion) | CMake parses the header and fills `cmake/version.rc.in` (Windows shared builds) |
| `libCarbonUI.so.X.Y.Z` file name | CMake target `VERSION` / `SOVERSION` |
| Doxygen `PROJECT_NUMBER` | `scripts/gen_docs.bat` resolves the header and injects `$(CARBONUI_VERSION)` |
| Source package name `CarbonUI_v<ver>.zip` | `tools/package_source.ps1` reads the header first (Doxyfile only as legacy fallback) |
| git tag / GitHub Release | `scripts/release.bat` tags `v<ver>` and pushes; `.github/workflows/release.yml` hard-fails unless the tag equals the header version, then publishes assets named `CarbonUI_v<ver>*.zip` |

Per-file Doxygen `@version` tags intentionally do not carry numbers — they point at `include/CarbonUIVersion.h`, so a release bump touches the header alone.

## Documentation

Every header/source file carries Doxygen comments (file header, `@brief`/`@param`/`@return` for each function, per-value enum docs). Generate the HTML docs with:

```bat
scripts\gen_docs.bat
:: (recommended: it injects CARBONUI_VERSION from include/CarbonUIVersion.h into the Doxyfile)
:: direct invocation also works: D:\autotools\doxygen-1.11.0\doxygen.exe Doxyfile
:: output: docs\html\index.html   warnings: doxygen_warnings.log (must stay empty)
```

`EXTRACT_ALL=NO` + `WARN_IF_UNDOCUMENTED/WARN_NO_PARAMDOC=YES` means an undocumented function produces a warning, so the log is the self-check for the comment rules.

## Local toolchain

Tools available on this machine (invoke them by absolute path):

| Tool | Purpose | Location |
| --- | --- | --- |
| Doxygen 1.11.0 | generate `docs/html` from `Doxyfile` | `D:\autotools\doxygen-1.11.0` |
| Python | one-off scripts (batch comment edits, encoding checks) | `C:\python` |
| pandoc | Markdown ↔ Word/HTML/PDF conversion | `D:\autotools\Pandoc` |
| 7za | archive/unarchive from the command line | `D:\autotools\7z` |
| ffmpeg | audio/video transcoding | `D:\autotools\ffmpeg\bin` |
| wkhtmltopdf | HTML → PDF / image | `D:\autotools\wkhtmltopdf\bin` |
| MuPDF (`mutool`) | PDF inspection/processing | `D:\autotools\mupdf` |
| Poppler (`pdftotext`/`pdftoppm`/…) | PDF conversion | `D:\autotools\pdftohtml` |
| aria2c | multi-protocol downloads | `D:\autotools\aria2-1.37.0-win-64bit-build1` |
| wget | simple HTTP downloads | `D:\autotools\wget` |
| iperf3 | network throughput testing | `D:\autotools\iperf` |
| Gost | port forwarding / tunnelling | `D:\autotools\gost` |
| gallery-dl | image-set downloader | `D:\autotools\gallery_dl` |
| Crawl4AI | web crawling / extraction skill pack | `D:\autotools\crawl4ai` |
| claude-skills | pdf / pptx / video-downloader skill packs | `D:\autotools\claude-skills` |
| RhinoAI | Rhino 8 MCP + modelling skill pack | `D:\autotools\rhinoai` |

The full list lives in `D:\autotools\autotools.md`.

## License

This project is licensed under the Mozilla Public License 2.0 (MPL-2.0). See `LICENSE` for details.

## Contributing

- Follow the project's formatting and style conventions (see `.clang-format` and repository guidelines).
- Open issues and pull requests on GitHub. Keep changes focused and add tests where appropriate.

