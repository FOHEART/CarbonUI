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

- `include/` ¡ª public headers
- `src/` ¡ª source files
- `lib/` ¡ª configured output directory for built artifacts (`lib/Debug`, `lib/Release`)

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
#include <modifyui.h>
#include <groupboxcollapser.h>
```

## License

This project is licensed under the Mozilla Public License 2.0 (MPL-2.0). See `LICENSE` for details.

## Contributing

- Follow the project's formatting and style conventions (see `.clang-format` and repository guidelines).
- Open issues and pull requests on GitHub. Keep changes focused and add tests where appropriate.

