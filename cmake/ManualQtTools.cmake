# ManualQtTools.cmake
# Helpers for manually invoking moc, uic, and rcc instead of relying on AUTO* properties.
#
# Usage:
#   qt_manual_moc(moc_outputs HEADERS header1.h header2.h ...)
#   qt_manual_uic(uic_outputs UI_FILES form1.ui form2.ui ...)
#   qt_manual_rcc(rcc_outputs QRC_FILES resource.qrc ...)
#
# Add the output variables to your target's source list, e.g.:
#   add_library(MyLib ${SOURCES} ${MOC_SOURCES} ${UIC_SOURCES} ${RCC_SOURCES})
#
# Requires: find_package(Qt5 REQUIRED COMPONENTS Core ...) called before including this file.

cmake_minimum_required(VERSION 3.16)

# ---------------------------------------------------------------------------
# qt_manual_moc
#   Runs moc on each listed header that contains Q_OBJECT or Q_GADGET.
#   Generated files are written to ${CMAKE_CURRENT_BINARY_DIR}/moc/moc_<name>.cpp
# ---------------------------------------------------------------------------
function(qt_manual_moc out_var)
    cmake_parse_arguments(ARG "" "" "HEADERS" ${ARGN})

    get_target_property(_moc Qt5::moc IMPORTED_LOCATION)
    if(NOT _moc)
        message(FATAL_ERROR "qt_manual_moc: could not locate Qt5::moc executable. "
                            "Make sure find_package(Qt5 ...) is called before including ManualQtTools.cmake.")
    endif()

    # Collect all Qt include dirs for moc -I flags
    set(_qt_includes)
    foreach(_comp Core Gui Widgets)
        if(TARGET Qt5::${_comp})
            get_target_property(_inc Qt5::${_comp} INTERFACE_INCLUDE_DIRECTORIES)
            if(_inc)
                list(APPEND _qt_includes ${_inc})
            endif()
        endif()
    endforeach()

    set(_generated)
    foreach(_hdr IN LISTS ARG_HEADERS)
        # Resolve to absolute path
        if(NOT IS_ABSOLUTE "${_hdr}")
            set(_hdr "${CMAKE_CURRENT_SOURCE_DIR}/${_hdr}")
        endif()

        # Only generate moc output for files that actually declare Q_OBJECT/Q_GADGET
        file(READ "${_hdr}" _contents)
        if(_contents MATCHES "Q_OBJECT|Q_GADGET")
            get_filename_component(_basename "${_hdr}" NAME_WE)
            set(_out "${CMAKE_CURRENT_BINARY_DIR}/moc/moc_${_basename}.cpp")

            # Build -I flag list
            set(_moc_includes)
            foreach(_inc_dir IN LISTS _qt_includes)
                list(APPEND _moc_includes "-I${_inc_dir}")
            endforeach()
            list(APPEND _moc_includes "-I${CMAKE_CURRENT_SOURCE_DIR}/include")

            add_custom_command(
                OUTPUT  "${_out}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/moc"
                COMMAND "${_moc}"
                        ${_moc_includes}
                        -o "${_out}"
                        "${_hdr}"
                DEPENDS "${_hdr}"
                COMMENT "moc: ${_basename}.h -> moc_${_basename}.cpp"
                VERBATIM
            )
            list(APPEND _generated "${_out}")
        endif()
    endforeach()

    set(${out_var} "${_generated}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# qt_manual_uic
#   Runs uic on each listed .ui file.
#   Generated headers are written to ${CMAKE_CURRENT_BINARY_DIR}/uic/ui_<name>.h
# ---------------------------------------------------------------------------
function(qt_manual_uic out_var)
    cmake_parse_arguments(ARG "" "" "UI_FILES" ${ARGN})

    get_target_property(_uic Qt5::uic IMPORTED_LOCATION)
    if(NOT _uic)
        message(FATAL_ERROR "qt_manual_uic: could not locate Qt5::uic executable. "
                            "Make sure find_package(Qt5 ...) is called before including ManualQtTools.cmake.")
    endif()

    set(_generated)
    foreach(_ui IN LISTS ARG_UI_FILES)
        if(NOT IS_ABSOLUTE "${_ui}")
            set(_ui "${CMAKE_CURRENT_SOURCE_DIR}/${_ui}")
        endif()

        get_filename_component(_basename "${_ui}" NAME_WE)
        set(_out "${CMAKE_CURRENT_BINARY_DIR}/uic/ui_${_basename}.h")

        add_custom_command(
            OUTPUT  "${_out}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/uic"
            COMMAND "${_uic}" -o "${_out}" "${_ui}"
            DEPENDS "${_ui}"
            COMMENT "uic: ${_basename}.ui -> ui_${_basename}.h"
            VERBATIM
        )
        list(APPEND _generated "${_out}")
    endforeach()

    set(${out_var} "${_generated}" PARENT_SCOPE)
endfunction()

# ---------------------------------------------------------------------------
# qt_manual_rcc
#   Runs rcc on each listed .qrc file.
#   Generated source files are written to ${CMAKE_CURRENT_BINARY_DIR}/rcc/qrc_<name>.cpp
# ---------------------------------------------------------------------------
function(qt_manual_rcc out_var)
    cmake_parse_arguments(ARG "" "" "QRC_FILES" ${ARGN})

    get_target_property(_rcc Qt5::rcc IMPORTED_LOCATION)
    if(NOT _rcc)
        message(FATAL_ERROR "qt_manual_rcc: could not locate Qt5::rcc executable. "
                            "Make sure find_package(Qt5 ...) is called before including ManualQtTools.cmake.")
    endif()

    set(_generated)
    foreach(_qrc IN LISTS ARG_QRC_FILES)
        if(NOT IS_ABSOLUTE "${_qrc}")
            set(_qrc "${CMAKE_CURRENT_SOURCE_DIR}/${_qrc}")
        endif()

        get_filename_component(_basename "${_qrc}" NAME_WE)
        set(_out "${CMAKE_CURRENT_BINARY_DIR}/rcc/qrc_${_basename}.cpp")

        add_custom_command(
            OUTPUT  "${_out}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/rcc"
            COMMAND "${_rcc}" --name "${_basename}" -o "${_out}" "${_qrc}"
            DEPENDS "${_qrc}"
            COMMENT "rcc: ${_basename}.qrc -> qrc_${_basename}.cpp"
            VERBATIM
        )
        list(APPEND _generated "${_out}")
    endforeach()

    set(${out_var} "${_generated}" PARENT_SCOPE)
endfunction()
