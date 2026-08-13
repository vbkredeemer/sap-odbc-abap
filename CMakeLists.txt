cmake_minimum_required(VERSION 3.20)
project(sapodbcabap VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

# Build mode: CI (stub headers, no NWRFC SDK) or REAL (with NWRFC SDK)
option(CI_BUILD "CI build with stub NWRFC headers (no real SDK needed)" OFF)

if(CI_BUILD)
    # Use stub headers for CI compilation
    message(STATUS "CI build mode — using stub NWRFC headers")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ci)
else()
    # Real build — require NWRFC SDK
    if(NOT NWRFCSDK_DIR)
        message(FATAL_ERROR "NWRFCSDK_DIR must be set to the SAP NWRFC SDK directory, or use -DCI_BUILD=ON for CI compilation")
    endif()
    include_directories(${NWRFCSDK_DIR}/include)
endif()

# ODBC Driver Manager
if(WIN32)
    target_link_libraries(sapodbcabap PRIVATE odbc32 odbccp32)
endif()

# Output: sapodbcabap.dll
add_library(sapodbcabap SHARED
    src/driver.cpp
    src/connection.cpp
    src/statement.cpp
    src/resultset.cpp
    src/rfc_wrapper.cpp
    src/config.cpp
)

# Link NWRFC SDK library (only in real build)
if(NOT CI_BUILD)
    if(WIN32)
        target_link_libraries(sapodbcabap PRIVATE ${NWRFCSDK_DIR}/lib/libsapnwrfc.lib)
    else()
        target_link_libraries(sapodbcabap PRIVATE ${NWRFCSDK_DIR}/lib/libsapnwrfc.so)
    endif()
endif()

# ODBC Driver Manager (odbc32 on Windows)
if(WIN32)
    target_link_libraries(sapodbcabap PRIVATE odbc32 odbccp32)
endif()

# Include directory
target_include_directories(sapodbcabap PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${NWRFCSDK_DIR}/include
)

# Set output name and use .def for exports
set_target_properties(sapodbcabap PROPERTIES 
    OUTPUT_NAME "sapodbcabap"
    PREFIX ""
)

if(WIN32)
    set_target_properties(sapodbcabap PROPERTIES 
        LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/sapodbcabap.def"
    )
endif()