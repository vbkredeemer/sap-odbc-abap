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
)

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
endif()

# Link libraries
if(WIN32)
    target_link_libraries(sapodbcabap PRIVATE odbc32 odbccp32)
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
    set_target_properties(sapodbcabap PROPERTIES
        LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/sapodbcabap.def"
    )
endif()