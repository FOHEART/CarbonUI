##############################################################################
# carbonui_cli -- 独立薄 CLI（CLI-Anything 宿主形态之二）
#
# 库工程没有 GUI 主程序可供 `--cli` 分流，所以 CLI 是一个独立可执行文件：
# 只做参数解析 + 调 CarbonUI 的样式 API + 输出统一信封，不含业务逻辑。
# 铁律：不 show() 任何窗口，QPA 缺省 offscreen，抓图靠 QWidget::grab()。
##############################################################################

add_executable(carbonui_cli
    main.cpp
    cli_parser.h
    cli_parser.cpp
    widget_factory.h
    widget_factory.cpp
    commands/info_cmd.h
    commands/info_cmd.cpp
    commands/render_cmd.h
    commands/render_cmd.cpp
    commands/qss_cmd.h
    commands/qss_cmd.cpp
)

target_include_directories(carbonui_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(carbonui_cli PRIVATE
    CarbonUI::CarbonUI
    Qt5::Core Qt5::Gui Qt5::Widgets)

# 源码含 UTF-8 中文注释，MSVC 需要 /utf-8 才不会按系统代码页解码。
if(MSVC)
    target_compile_options(carbonui_cli PRIVATE /utf-8)
endif()

# 渲染用内置字体（与 tests/visual 的截图基线同一份）：字形度量跨机器一致，
# snapshot/gallery 的输出才可复现。文件不存在（如裁剪过的源码树）时跳过，
# CLI 运行期会静默回退系统字体。
set(CARBONUI_BUNDLED_FONT "${CMAKE_SOURCE_DIR}/src/Resources/font/HarmonyOS_Sans_SC_Regular.ttf")
if(EXISTS "${CARBONUI_BUNDLED_FONT}")
    target_compile_definitions(carbonui_cli PRIVATE
        CARBONUI_CLI_BUNDLED_FONT="${CARBONUI_BUNDLED_FONT}")
endif()

# Windows 上 Qt DLL 与平台插件不在默认搜索路径：拷到 exe 同级，
# 让 CLI 脱离 QTDIR/PATH 也能直接跑（与 tests 的做法一致）。
if(WIN32)
    add_custom_command(TARGET carbonui_cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:carbonui_cli>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt5::Core> $<TARGET_FILE:Qt5::Gui> $<TARGET_FILE:Qt5::Widgets>
                "$<TARGET_FILE_DIR:carbonui_cli>"
        COMMAND ${CMAKE_COMMAND}
                -DQT_BIN="$<TARGET_FILE_DIR:Qt5::Core>"
                -DDST_DIR="$<TARGET_FILE_DIR:carbonui_cli>"
                -P "${CMAKE_SOURCE_DIR}/tests/copy_qt_platforms.cmake"
        VERBATIM)
endif()
