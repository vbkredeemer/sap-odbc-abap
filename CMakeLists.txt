cmake_minimum_required(VERSION 3.20)
project(sapodbcabap VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

# NWRFC SDK path (pass with -DNWRFCSDK_DIR=...)
if(NOT NWRFCSDK_DIR)
    message(FATAL_ERROR "NWRFCSDK_DIR must be set to the SAP NWRFC SDK directory")
endif()

# Include NWRFC SDK headers
include_directories(${NWRFCSDK_DIR}/include)

# Output: sapodbcabap.dll
add_library(sapodbcabap SHARED
    src/driver.cpp
    src/connection.cpp
    src/statement.cpp
    src/resultset.cpp
    src/rfc_wrapper.cpp
    src/config.cpp
)

# Link NWRFC SDK library
if(WIN32)
    target_link_libraries(sapodbcabap PRIVATE ${NWRFCSDK_DIR}/lib/libsapnwrfc.lib)
else()
    target_link_libraries(sapodbcabap PRIVATE ${NWRFCSDK_DIR}/lib/libsapnwrfc.so)
endif()

# ODBC Driver Manager (odbc32 / odbc32.lib on Windows)
if(WIN32)
    target_link_libraries(sapodbcabap PRIVATE odbc32 odbccp32)
endif()

# Include directory
target_include_directories(sapodbcabap PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${NWRFCSDK_DIR}/include
)

# Set output name
set_target_properties(sapodbcabap PROPERTIES 
    OUTPUT_NAME "sapodbcabap"
    PREFIX ""
)