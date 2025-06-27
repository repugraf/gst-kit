cmake_minimum_required(VERSION 3.10)
project(gst_kit)

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
        ${CMAKE_CURRENT_SOURCE_DIR}/../node_modules/node-addon-api
    NO_DEFAULT_PATH
)

if(NOT NODE_ADDON_API_DIR)
    message(FATAL_ERROR "node-addon-api not found. Please ensure dependencies are installed:\n"
                        "  npm install\n"
                        "Searched in:\n"
                        "  - ${CMAKE_JS_INC}\n"
                        "  - ${CMAKE_CURRENT_SOURCE_DIR}/node_modules/node-addon-api\n"
                        "  - ${CMAKE_CURRENT_SOURCE_DIR}/../node_modules/node-addon-api")
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

# Add library directories for linking
if(WIN32)
    # Detect Windows architecture for GStreamer path
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
            set(GSTREAMER_ARCH "arm64")
            set(GSTREAMER_ENV_VAR "GSTREAMER_1_0_ROOT_MSVC_ARM64")
        else()
            set(GSTREAMER_ARCH "x86_64")
            set(GSTREAMER_ENV_VAR "GSTREAMER_1_0_ROOT_MSVC_X86_64")
        endif()
    else()
        # 32-bit x86
        set(GSTREAMER_ARCH "x86")
        set(GSTREAMER_ENV_VAR "GSTREAMER_1_0_ROOT_MSVC_X86")
    endif()
    
    message(STATUS "Detected Windows architecture: ${GSTREAMER_ARCH}")
    
    # Windows-specific library directory setup
    if(DEFINED ENV{${GSTREAMER_ENV_VAR}})
        link_directories("$ENV{${GSTREAMER_ENV_VAR}}/lib")
        message(STATUS "Added Windows GStreamer lib directory: $ENV{${GSTREAMER_ENV_VAR}}/lib")
    endif()
    
    # Also add from pkg-config library directories
    if(GSTREAMER_LIBRARY_DIRS)
        link_directories(${GSTREAMER_LIBRARY_DIRS})
        message(STATUS "Added pkg-config library directories: ${GSTREAMER_LIBRARY_DIRS}")
    endif()
    if(GSTREAMER_APP_LIBRARY_DIRS)
        link_directories(${GSTREAMER_APP_LIBRARY_DIRS})
    endif()
    if(GSTREAMER_RTP_LIBRARY_DIRS)
        link_directories(${GSTREAMER_RTP_LIBRARY_DIRS})
    endif()
    if(GLIB_LIBRARY_DIRS)
        link_directories(${GLIB_LIBRARY_DIRS})
    endif()
    if(GOBJECT_LIBRARY_DIRS)
        link_directories(${GOBJECT_LIBRARY_DIRS})
    endif()
else()
    # Unix-like systems: use library directories from pkg-config
    link_directories(
        ${GSTREAMER_LIBRARY_DIRS}
        ${GSTREAMER_APP_LIBRARY_DIRS}
        ${GSTREAMER_RTP_LIBRARY_DIRS}
        ${GLIB_LIBRARY_DIRS}
        ${GOBJECT_LIBRARY_DIRS}
    )
endif()

# Set output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Release")

# Add GStreamer Kit native addon library
add_library(gst_kit_addon SHARED
    src/cpp/addon.cpp
    src/cpp/pipeline.cpp
    src/cpp/element.cpp
    src/cpp/type-conversion.cpp
    src/cpp/async-workers.cpp
)

# Set output name and properties (keep .node extension for Node.js compatibility)
set_target_properties(gst_kit_addon PROPERTIES
    PREFIX ""
    SUFFIX ".node"
    OUTPUT_NAME "gst_kit"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
)

# Link GStreamer and Node.js libraries
target_link_libraries(gst_kit_addon
    ${CMAKE_JS_LIB}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_APP_LIBRARIES}
    ${GSTREAMER_RTP_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GOBJECT_LIBRARIES}
)

# Windows-specific compiler flag fixes
if(WIN32)
    # Remove Unix-specific flags that cause warnings on Windows
    string(REPLACE "-fPIC" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "-O3" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    
    # Add Windows-specific optimizations
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
    
    message(STATUS "Windows build detected - adjusted compiler flags")
endif()

# Set build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Set compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

# Add compile definitions for Node.js addon
target_compile_definitions(gst_kit_addon PRIVATE
    NAPI_CPP_EXCEPTIONS
    BUILDING_NODE_EXTENSION
)
