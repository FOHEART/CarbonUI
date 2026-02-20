cmake_minimum_required(VERSION 3.16)
project(CarbonUI LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt automatic tools (moc/uic/rcc) so Q_OBJECT classes get processed


# ########################  3rd party dependencies  #########################
include(FetchContent)

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

# Prefer user/CI-provided hints. Only set a fallback when Qt5_DIR is not already defined.
if(NOT DEFINED Qt5_DIR OR Qt5_DIR STREQUAL "")
  if(WIN32)
    # set(Qt5_DIR "D:/Qt5/5.15.2/msvc2019_64/lib/cmake/Qt5" CACHE PATH "Path to Qt5Config.cmake")
  else()
    # set(Qt5_DIR "/opt/Qt/5.14.2/gcc_64/lib/cmake/Qt5" CACHE PATH "Path to Qt5Config.cmake")
  endif()
endif()
if(DEFINED Qt5_DIR AND NOT Qt5_DIR STREQUAL "")
  message(STATUS "Using Qt5 from: ${Qt5_DIR}")
else()
  message(STATUS "Using Qt5 from: (not set)")
endif()

include(cmake/qt.cmake)
qt_standard_project_setup()

# Ensure Qt5 imported targets are available for linking
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# headers go into include/, sources into src/
set(PROJECT_HEADERS
    include/carbonui_global.h
    include/CarbonUIClass.h
    include/groupboxcollapser.h
)

set(PROJECT_SOURCES
    src/CarbonUIClass.cpp
    src/CarbonUIClass_QRadioButton.cpp
    src/groupboxcollapser.cpp
)

# Qt resource file(s)
set(PROJECT_RESOURCES
    Resource.qrc
)

# Allow consumers to select shared vs static build. Default: OFF (build static library)
option(BUILD_CARBONUI_SHARED "Build CarbonUI as a shared library" OFF)
if(BUILD_CARBONUI_SHARED)
  message(STATUS "Config: Building CarbonUI as SHARED library")
else()
  message(STATUS "Config: Building CarbonUI as STATIC library")
endif()

# Create target as SHARED or STATIC depending on option
if(BUILD_CARBONUI_SHARED)
  add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${PROJECT_RESOURCES})
else()
  add_library(${PROJECT_NAME} STATIC ${PROJECT_SOURCES} ${PROJECT_HEADERS} ${PROJECT_RESOURCES})
endif()

# Use target-local include directories so consuming projects (CPM/FetchContent) find headers.
# Provide BUILD_INTERFACE for in-tree builds and INSTALL_INTERFACE for installed layout.
target_include_directories(${PROJECT_NAME}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt5::Core
         Qt5::Gui
        Qt5::Widgets
)

# Define CARBONUI_LIB when building the CarbonUI shared library so export/import
# macros in headers resolve correctly for the build. Consumers can check BUILD_CARBONUI_SHARED.
if(BUILD_CARBONUI_SHARED)
  target_compile_definitions(${PROJECT_NAME}       PRIVATE         CARBONUI_DLL_EXPORT  )
else()
  # For static builds, define a symbol if callers need to detect static linking
  target_compile_definitions(${PROJECT_NAME}      PUBLIC          CARBONUI_STATIC_LIB  )
endif()

# Place generated runtime (DLL), archive (lib) and library outputs into
# the project "lib/<Config>" directory (e.g. lib/Debug, lib/Release).
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/$<CONFIG>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/$<CONFIG>"
)

# Optional: export a namespace alias for consumers that expect CarbonUI::CarbonUI
add_library(CarbonUI::CarbonUI ALIAS ${PROJECT_NAME})

