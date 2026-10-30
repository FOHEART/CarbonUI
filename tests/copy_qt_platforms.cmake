# Copies the Qt QPA plugins next to a test executable so the "offscreen"
# platform resolves without a system-wide Qt installation on PATH.
#
# Required variables:
#   QT_BIN  - directory holding Qt5Core.dll (i.e. <Qt>/bin)
#   DST_DIR - directory of the test executable
#
# Both may arrive as a quoted `-DVAR="C:/Qt/bin"` argument from a POST_BUILD
# step; CMake keeps those quote characters, so strip them before use.

if(NOT DEFINED QT_BIN OR NOT DEFINED DST_DIR)
    message(FATAL_ERROR "copy_qt_platforms.cmake: QT_BIN and DST_DIR are required")
endif()

string(REGEX REPLACE "^\"(.*)\"$" "\\1" _qt_bin "${QT_BIN}")
string(REGEX REPLACE "^\"(.*)\"$" "\\1" _dst_dir "${DST_DIR}")

set(_platforms_src "${_qt_bin}/../plugins/platforms")
if(EXISTS "${_platforms_src}")
    file(COPY "${_platforms_src}/" DESTINATION "${_dst_dir}/platforms")
else()
    message(WARNING "copy_qt_platforms.cmake: no QPA plugins under ${_platforms_src}")
endif()
