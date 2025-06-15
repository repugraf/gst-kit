cmake_minimum_required(VERSION 3.10)
project(native_addon)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GSTREAMER_RTP REQUIRED gstreamer-rtp-1.0)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GOBJECT REQUIRED gobject-2.0)

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

# Include directories
include_directories(
    ${CMAKE_JS_INC}
    ${NODE_ADDON_API_DIR}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_APP_INCLUDE_DIRS}
    ${GSTREAMER_RTP_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${GOBJECT_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp
)

# Set output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release")

# Add library target
add_library(native_addon SHARED
    src/cpp/addon.cpp
    src/cpp/pipeline.cpp
    src/cpp/element.cpp
)

# Set output name and properties
set_target_properties(native_addon PROPERTIES
    PREFIX ""
    SUFFIX ".node"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
)

# Link libraries
target_link_libraries(native_addon
    ${CMAKE_JS_LIB}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    ${GSTREAMER_RTP_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GOBJECT_LIBRARIES}
)

# Set build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Add compile definitions
target_compile_definitions(native_addon PRIVATE
    NAPI_CPP_EXCEPTIONS
    BUILDING_NODE_EXTENSION
)
 