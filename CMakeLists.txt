cmake_minimum_required(VERSION 3.10)
project(DistributedRaytracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(render/include common/include)

# --- Library Definitions ---

add_library(common COMMON common/src/scene_parser.cpp)

add_executable(render
    render/src/main.cpp
    render/src/camera.cpp
    render/src/renderer.cpp
    render/src/vec3.cpp
    render/src/material.cpp
    render/src/sphere.cpp
    render/src/cylinder.cpp
    render/src/hittable_list.cpp
    render/src/color.cpp
)

add_executable(master master/main.cpp)
add_executable(worker worker/main.cpp)

# --- Linkage ---

# Link executables against the common library
target_link_libraries(master PRIVATE common)
target_link_libraries(worker PRIVATE common)

# --- OpenMP Support for 'render' executable (Platform-Aware) ---

if(APPLE)
    # Explicit configuration for macOS with Homebrew's libomp
    message(STATUS "Apple system detected. Applying macOS-specific OpenMP configuration.")
    target_include_directories(render PRIVATE /opt/homebrew/opt/libomp/include)
    target_link_directories(render PRIVATE /opt/homebrew/opt/libomp/lib)
    target_compile_options(render PRIVATE -Xpreprocessor -fopenmp)
    # Link both common and omp library to the render executable
    target_link_libraries(render PRIVATE common omp)
else()
    # Standard configuration for other systems (Linux, Windows)
    find_package(OpenMP)
    if(OpenMP_FOUND)
        message(STATUS "OpenMP found, enabling parallel build for 'render' target.")
        target_link_libraries(render PRIVATE common OpenMP::OpenMP_CXX)
    else()
        message(WARNING "OpenMP not found. 'render' will be single-threaded.")
        target_link_libraries(render PRIVATE common)
    endif()
endif()