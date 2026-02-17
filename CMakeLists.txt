cmake_minimum_required(VERSION 3.16)
project(CarbonUI LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

if (WIN32)
  # Optional hardcoded fallback for Windows development machines can remain commented out.
  # set(Qt5_DIR "D:/Qt5.15.2/5.15.2/msvc2019_64/lib/cmake/Qt5" CACHE PATH "Path to Qt5Config.cmake")
else()
 set(Qt5_DIR "/opt/Qt5.14.2/5.14.2/gcc_64/lib/cmake/Qt5")
endif()
message(STATUS "Using Qt5 from: ${Qt5_DIR}")

include(cmake/qt.cmake)

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
    src/groupboxcollapser.cpp
)

add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES} ${PROJECT_HEADERS})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Qt5::Core
         Qt5::Gui
        Qt5::Widgets
)

# Define CARBONUI_LIB when building the CarbonUI library so export/import
# macros in headers resolve correctly for the build.
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        CARBONUI_LIB
)

# Place generated runtime (DLL), archive (lib) and library outputs into
# the project "lib/<Config>" directory (e.g. lib/Debug, lib/Release).
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/$<CONFIG>"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/$<CONFIG>"
)

