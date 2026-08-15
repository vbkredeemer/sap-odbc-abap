cmake_minimum_required(VERSION 3.20)
project(sapodbcabap VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

# Build mode: CI (stub headers, no NWRFC SDK) or REAL (with NWRFC SDK)
option(CI_BUILD "CI build with stub NWRFC headers (no real SDK needed)" OFF)

if(CI_BUILD)
    message(STATUS "CI build mode — using stub NWRFC headers")
else()
    if(NOT NWRFCSDK_DIR)
        message(FATAL_ERROR "NWRFCSDK_DIR must be set to the SAP NWRFC SDK directory, or use -DCI_BUILD=ON for CI compilation")
    endif()
endif()

# Output: sapodbcabap.dll
add_library(sapodbcabap SHARED
    src/driver.cpp
    src/rfc_wrapper.cpp
    src/config.cpp
    src/metadata.cpp
    src/chunked_read.cpp
)

# Compile .rc resource into .res object (Windows/MinGW only)
if(WIN32 AND MINGW)
    find_program(WINDRES x86_64-w64-mingw32-windres)
    if(NOT WINDRES)
        message(FATAL_ERROR "windres not found — needed for dialog resource compilation")
    endif()
    set(CONFIG_DLG_RES ${CMAKE_BINARY_DIR}/config_dlg.res)
    add_custom_command(
        OUTPUT ${CONFIG_DLG_RES}
        COMMAND ${WINDRES} -O coff -I ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${CMAKE_CURRENT_SOURCE_DIR}/src/config_dlg.rc
                -o ${CONFIG_DLG_RES}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/config_dlg.rc
                ${CMAKE_CURRENT_SOURCE_DIR}/src/resource.h
        COMMENT "Compiling dialog resource config_dlg.rc"
    )
    # Add .res directly to the linker command — target_sources alone
    # does not reliably pass .res through to the link step under MinGW.
    target_sources(sapodbcabap PRIVATE ${CONFIG_DLG_RES})
endif()

# Include directories
if(CI_BUILD)
    target_include_directories(sapodbcabap PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/ci
    )
else()
    target_include_directories(sapodbcabap PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${NWRFCSDK_DIR}/include
    )
    # SAP NWRFC SDK uses Unicode mode on Windows
    target_compile_definitions(sapodbcabap PRIVATE SAPwithUNICODE)
endif()

# Link libraries
if(WIN32)
    target_link_libraries(sapodbcabap PRIVATE odbc32 odbccp32)
    if(MINGW)
        # Statisch linken gegen MinGW-Runtime, damit libgcc_s_seh-1.dll
        # und libstdc++-6.dll nicht als externe Abhaengigkeit entstehen
        target_link_options(sapodbcabap PRIVATE -static-libgcc -static-libstdc++)
    endif()
endif()

if(NOT CI_BUILD)
    if(WIN32)
        target_link_libraries(sapodbcabap PRIVATE ${NWRFCSDK_DIR}/lib/libsapnwrfc.lib)
    else()
        target_link_libraries(sapodbcabap PRIVATE ${NWRFCSDK_DIR}/lib/libsapnwrfc.so)
    endif()
endif()

# Output name and .def for exports
set_target_properties(sapodbcabap PROPERTIES
    OUTPUT_NAME "sapodbcabap"
    PREFIX ""
)

if(WIN32)
    if(MINGW)
        # MinGW: use linker --def option via module definition + link .res resource
        set_target_properties(sapodbcabap PROPERTIES
            LINK_FLAGS "-Wl,--kill-at -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/sapodbcabap.exports --def=${CMAKE_CURRENT_SOURCE_DIR}/src/sapodbcabap.def ${CMAKE_BINARY_DIR}/config_dlg.res"
        )
    else()
        # MSVC: use /DEF linker flag
        set_target_properties(sapodbcabap PROPERTIES
            LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/sapodbcabap.def"
        )
    endif()
endif()