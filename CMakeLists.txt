cmake_minimum_required(VERSION 3.16)

# ########################  version (single source of truth)  ################
# 版本号唯一来源是 include/CarbonUIVersion.h：升版只改该头文件，这里解析出
# 三位数字供 project(VERSION)、目标 VERSION/SOVERSION 与 Windows DLL 版本
# 资源（cmake/version.rc.in）使用。必须在 project() 之前读出。
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/CarbonUIVersion.h" _carbonui_version_h)
string(REGEX MATCH "define[ \t]+CARBONUI_VERSION_MAJOR[ \t]+([0-9]+)" _carbonui_m "${_carbonui_version_h}")
set(CARBONUI_VERSION_MAJOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "define[ \t]+CARBONUI_VERSION_MINOR[ \t]+([0-9]+)" _carbonui_m "${_carbonui_version_h}")
set(CARBONUI_VERSION_MINOR "${CMAKE_MATCH_1}")
string(REGEX MATCH "define[ \t]+CARBONUI_VERSION_PATCH[ \t]+([0-9]+)" _carbonui_m "${_carbonui_version_h}")
set(CARBONUI_VERSION_PATCH "${CMAKE_MATCH_1}")
if("${CARBONUI_VERSION_MAJOR}" STREQUAL "" OR "${CARBONUI_VERSION_MINOR}" STREQUAL "" OR "${CARBONUI_VERSION_PATCH}" STREQUAL "")
  # 注意不能用 if(NOT VAR)：主版本号为 0 时 CMake 会把 0 当 false 常量误判
  message(FATAL_ERROR "无法从 include/CarbonUIVersion.h 解析 CARBONUI_VERSION_MAJOR/MINOR/PATCH")
endif()
set(CARBONUI_VERSION "${CARBONUI_VERSION_MAJOR}.${CARBONUI_VERSION_MINOR}.${CARBONUI_VERSION_PATCH}")
message(STATUS "CarbonUI version: ${CARBONUI_VERSION} (from include/CarbonUIVersion.h)")

project(CarbonUI VERSION ${CARBONUI_VERSION} LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt automatic tools (moc/uic/rcc) so Q_OBJECT classes get processed


# ########################  3rd party dependencies  #########################
include(FetchContent)
include(cmake/CPM.cmake)

# Try to auto-detect Qt5 via find_package (so we can derive Qt5_DIR from the discovered Qt5Core)
# This allows users to rely on find_package instead of hardcoding Qt5 paths.
find_package(Qt5Core QUIET)
if(TARGET Qt5::Core OR Qt5Core_FOUND)
  # Qt5Core_DIR is provided by the config package and points to the directory containing Qt5Config.cmake
  if(DEFINED Qt5Core_DIR)
    set(Qt5_DIR "${Qt5Core_DIR}" CACHE PATH "Detected Qt5 CMake directory from Qt5Core")
    message(STATUS "Detected Qt5_DIR from Qt5Core: ${Qt5_DIR}")
  else()
    message(STATUS "Qt5Core found but Qt5Core_DIR not defined; leaving Qt5_DIR as-is: ${Qt5_DIR}")
  endif()
else()
  message(STATUS "Qt5Core not found by initial probe; will use Qt5_DIR (if set) as a hint when locating Qt5 components")
endif()

include(cmake/qt.cmake)
qt_standard_project_setup()

# Ensure Qt5 imported targets are available for linking
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# headers go into include/, sources into src/
set(PROJECT_HEADERS
    include/carbonui_global.h
    include/CarbonUIVersion.h
    include/CarbonUIClass.h
    include/CarbonUIDialog.h
    include/groupboxcollapser.h
)

set(PROJECT_SOURCES
    src/CarbonUIClass.cpp
    src/CarbonUIClass_QRadioButton.cpp
    src/CarbonUIClass_QCheckBox.cpp
    src/CarbonUIClass_QLineEdit.cpp
    src/CarbonUIClass_QPushButton.cpp
    src/CarbonUIClass_QScrollArea.cpp
    src/CarbonUIClass_QMenuBar.cpp
    src/CarbonUIClass_QToolBar.cpp
    src/CarbonUIClass_QTabWidget.cpp
    src/CarbonUIClass_QComboBox.cpp
    src/CarbonUIClass_QStatusBar.cpp
    src/CarbonUIClass_QTreeWidget.cpp
    src/CarbonUIDialog.cpp
    src/groupboxcollapser.cpp
)

# Qt Designer UI file(s)
# (none currently in this project; add .ui files here when needed)
set(PROJECT_UI_FILES
)

# Qt resource file(s)
# 文件名以 carbonui_ 开头是有意的：RCC 按 qrc 文件名生成注册符号
# qInitResources_<name>，消费方常自带 Resource.qrc，同名会撞符号。
set(PROJECT_RESOURCES
    carbonui_rcc.qrc
)

# Manual Qt processing (no CMAKE_AUTOMOC/AUTOUIC/AUTORCC):
qt5_wrap_cpp(MOC_SOURCES ${PROJECT_HEADERS})
qt5_wrap_ui(UIC_SOURCES ${PROJECT_UI_FILES})
qt5_add_resources(RCC_SOURCES ${PROJECT_RESOURCES})

# Allow consumers to select shared vs static build. Default: OFF (build static library)
if(NOT DEFINED BUILD_CARBONUI_SHARED)
  option(BUILD_CARBONUI_SHARED "Build CarbonUI as a shared library" OFF)
else()
  message(STATUS "BUILD_CARBONUI_SHARED provided by caller: ${BUILD_CARBONUI_SHARED}")
endif()

if(BUILD_CARBONUI_SHARED)
  message(STATUS "Config: Building CarbonUI as SHARED library")
else()
  message(STATUS "Config: Building CarbonUI as STATIC library")
endif()

# Create target as SHARED or STATIC depending on option
if(BUILD_CARBONUI_SHARED)
  add_library(${PROJECT_NAME} SHARED
    ${PROJECT_SOURCES} ${PROJECT_HEADERS}
    ${MOC_SOURCES} ${UIC_SOURCES} ${RCC_SOURCES})
else()
  add_library(${PROJECT_NAME} STATIC
    ${PROJECT_SOURCES} ${PROJECT_HEADERS}
    ${MOC_SOURCES} ${UIC_SOURCES} ${RCC_SOURCES})
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)

# Windows DLL 版本属性：configure 填充 version.rc.in（FileVersion/ProductVersion），
# 编译进 dll 的资源段，资源管理器可见。仅动态库需要，静态库无此产物。
if(WIN32 AND BUILD_CARBONUI_SHARED)
  configure_file(cmake/version.rc.in "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
  target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
endif()

# 动态库文件名带版本（libCarbonUI.so.0.2.1 / libCarbonUI.1.dylib）；
# Win32 的 PE 头版本由上面的 version.rc 负责，VERSION 属性同时写入其 Unicode 元数据。
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION   ${CARBONUI_VERSION}
    SOVERSION ${CARBONUI_VERSION_MAJOR})

# Sources contain UTF-8 (Chinese) Doxygen comments; MSVC would otherwise decode
# them with the system code page and emit C4819 / break the comment blocks.
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# Use target-local include directories so consuming projects (CPM/FetchContent) find headers.
# Provide BUILD_INTERFACE for in-tree builds and INSTALL_INTERFACE for installed layout.
# Also expose the uic binary output dir so generated headers can be #included.
target_include_directories(${PROJECT_NAME}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    PRIVATE
      ${CMAKE_CURRENT_BINARY_DIR}
)

# Define CARBONUI_LIB when building the CarbonUI shared library so export/import
# macros in headers resolve correctly for the build. Consumers can check BUILD_CARBONUI_SHARED.
if(BUILD_CARBONUI_SHARED)
  target_compile_definitions(${PROJECT_NAME} PRIVATE CARBONUI_DLL_EXPORT)
else()
  target_compile_definitions(${PROJECT_NAME} PUBLIC  CARBONUI_STATIC_LIB)
endif()

# Optional: export a namespace alias for consumers that expect CarbonUI::CarbonUI
add_library(CarbonUI::CarbonUI ALIAS ${PROJECT_NAME})

# ########################  carbonui_cli (CLI-Anything)  ####################
# 本库没有 GUI 主程序，CLI 走"独立薄可执行文件"形态：只做参数解析 + 调样式
# API + 输出统一信封。独立检出的仓库默认构建（测试与本地验证都要用），
# 被 add_subdirectory/CPM 引入的消费者默认不构建，可用 -DBUILD_CARBONUI_CLI=ON 打开。
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CARBONUI_CLI_DEFAULT ON)
else()
  set(CARBONUI_CLI_DEFAULT OFF)
endif()
option(BUILD_CARBONUI_CLI "Build the carbonui_cli command line tool" ${CARBONUI_CLI_DEFAULT})

if(BUILD_CARBONUI_CLI)
  add_subdirectory(cli)
endif()

# ########################  unit tests (TDD)  #########################
# Standalone checkouts default to ON so the red/green loop is one command away;
# consumers pulling CarbonUI in via add_subdirectory/CPM default to OFF and can
# opt in with -DBUILD_CARBONUI_TESTS=ON.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(CARBONUI_TESTS_DEFAULT ON)
else()
  set(CARBONUI_TESTS_DEFAULT OFF)
endif()
option(BUILD_CARBONUI_TESTS "Build the CarbonUI GoogleTest unit test suite" ${CARBONUI_TESTS_DEFAULT})

if(BUILD_CARBONUI_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

