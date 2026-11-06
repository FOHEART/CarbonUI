##############################################################################
# CarbonUI tests (GoogleTest)
#
# Two executables, because they answer two different questions:
#
#   carbonui_tests         "did the helper apply the intended configuration?"
#                          (offscreen QPA, no fonts needed, fast, CI-friendly)
#   carbonui_visual_tests  "does it actually look right when painted?"
#                          (native QPA so fonts exist, renders via QWidget::grab()
#                           and asserts pixel colours; never shows a window)
#
# Both start a QApplication from their own custom main() and share the helpers in
# support/qt_test_support.h.
##############################################################################

include(FetchContent)
include(GoogleTest)

# Overridable so air-gapped builds can point at a local checkout or a mirror:
#   cmake -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=D:/deps/googletest
#   cmake -DGTEST_REPO=https://git.internal/googletest.git
set(GTEST_REPO "https://github.com/google/googletest.git" CACHE STRING "Git repository used to fetch GoogleTest")
set(GTEST_TAG "v1.15.2" CACHE STRING "GoogleTest tag/branch/SHA to build")

# MSVC: GoogleTest defaults to /MT while Qt uses /MD; mixing the two fails with
# LNK4098 and cross-heap frees.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(googletest
    GIT_REPOSITORY "${GTEST_REPO}"
    GIT_TAG        "${GTEST_TAG}"
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE)
FetchContent_MakeAvailable(googletest)

# Applies the settings every test executable needs.
#   target  - test executable
#   prefix  - ctest name prefix ("Unit." / "Visual.")
#   env     - QT_QPA_PLATFORM value handed to ctest
function(carbonui_configure_test_target target prefix env)
    # gtest 1.14+ requires C++14; the library itself stays on C++11.
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 14
        CXX_STANDARD_REQUIRED ON)

    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/support)

    target_link_libraries(${target} PRIVATE
        CarbonUI
        GTest::gtest
        Qt5::Core Qt5::Gui Qt5::Widgets)

    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8)
    endif()

    # Qt DLLs/plugins live outside the default runtime search path, so tests are
    # unrunnable straight after a build on a machine without QTDIR on PATH.
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:Qt5::Core> $<TARGET_FILE:Qt5::Gui> $<TARGET_FILE:Qt5::Widgets>
                    "$<TARGET_FILE:CarbonUI>"
                    "$<TARGET_FILE_DIR:${target}>"
            COMMAND ${CMAKE_COMMAND}
                    -DQT_BIN="$<TARGET_FILE_DIR:Qt5::Core>"
                    -DDST_DIR="$<TARGET_FILE_DIR:${target}>"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/copy_qt_platforms.cmake"
            VERBATIM)
    endif()

    # DISCOVERY_MODE PRE_TEST asks the executable for its case list from ctest, not
    # from the build step, so a plain `cmake --build` never has to load Qt.
    gtest_discover_tests(${target}
        TEST_PREFIX "${prefix}"
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=${env}" TIMEOUT 120
        DISCOVERY_MODE PRE_TEST)
endfunction()

# ---------------------------------------------------------------- state tests
add_executable(carbonui_tests
    unit/main.cpp
    unit/carbonui_lineedit_test.cpp
    unit/carbonui_checkbox_test.cpp
    unit/carbonui_button_test.cpp
    unit/carbonui_container_test.cpp
    unit/carbonui_tree_test.cpp
    unit/carbonui_radio_test.cpp
    unit/groupboxcollapser_test.cpp
)
carbonui_configure_test_target(carbonui_tests "Unit." offscreen)

# ------------------------------------------------------------- render evidence
add_executable(carbonui_visual_tests
    visual/main.cpp
    visual/carbonui_render_test.cpp
    visual/screenshot_test.cpp
)
# "windows" on Win32 so the font engine is available; the suite skips text
# assertions on platforms whose font database is empty.
if(WIN32)
    set(CARBONUI_VISUAL_QPA windows)
else()
    set(CARBONUI_VISUAL_QPA offscreen)
endif()
carbonui_configure_test_target(carbonui_visual_tests "Visual." ${CARBONUI_VISUAL_QPA})

# 截图基线：基线必须按平台分目录。字体引擎不同（DirectWrite / FreeType），
# 同一控件文字的亚像素分布本来就不同，跨平台共用一份基线只会永久红灯。
if(WIN32)
    set(CARBONUI_SCREENSHOT_PLATFORM "windows")
elseif(APPLE)
    set(CARBONUI_SCREENSHOT_PLATFORM "macos")
else()
    set(CARBONUI_SCREENSHOT_PLATFORM "linux")
endif()
target_compile_definitions(carbonui_visual_tests PRIVATE
    CARBONUI_SCREENSHOT_BASELINE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/visual/baseline"
    CARBONUI_SCREENSHOT_PLATFORM="${CARBONUI_SCREENSHOT_PLATFORM}"
    # 视觉测试统一使用仓库内置字体：本地与 CI 加载同一份 TTF，字形度量完全一致，
    # 截图基线才跨机器可复现（不再依赖系统 UI 字体的偶然差异）。
    CARBONUI_TEST_BUNDLED_FONT="${CMAKE_SOURCE_DIR}/src/Resources/font/HarmonyOS_Sans_SC_Regular.ttf")
