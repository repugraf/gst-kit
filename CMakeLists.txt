cmake_minimum_required(VERSION 3.10)
project(native_addon)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find node-addon-api
find_path(NODE_ADDON_API_DIR
    NAMES napi.h
    PATHS 
        ${CMAKE_JS_INC}
        ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/node-addon-api
    NO_DEFAULT_PATH
)

if(NOT NODE_ADDON_API_DIR)
    message(FATAL_ERROR "node-addon-api not found. Please run 'npm install' first.")
endif()

# Add the source files
set(SOURCES
    src/cpp/addon.cpp
)

# Set output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release")

# Create the shared library
add_library(native_addon SHARED ${SOURCES})

# Include directories
target_include_directories(native_addon PRIVATE
    ${CMAKE_JS_INC}
    ${NODE_ADDON_API_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp
)

# Set the output directory and properties
set_target_properties(native_addon PROPERTIES
    PREFIX ""
    SUFFIX ".node"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
)

# Link with node-addon-api
target_link_libraries(native_addon ${CMAKE_JS_LIB})

# Add compile definitions
target_compile_definitions(native_addon PRIVATE
    NAPI_CPP_EXCEPTIONS
    BUILDING_NODE_EXTENSION
)
