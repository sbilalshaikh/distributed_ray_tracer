cmake_minimum_required(VERSION 3.15)
project(DistributedRaytracer LANGUAGES CXX)

# -----------------------
# Global configuration
# -----------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# -----------------------
# Dependencies (portable)
# -----------------------
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# -----------------------
# Proto generation
# -----------------------
set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/common/proto/raytracer.proto)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated-proto)
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(PROTO_SRCS ${GENERATED_DIR}/raytracer.pb.cc)
set(PROTO_HDRS ${GENERATED_DIR}/raytracer.pb.h)
set(GRPC_SRCS ${GENERATED_DIR}/raytracer.grpc.pb.cc)
set(GRPC_HDRS ${GENERATED_DIR}/raytracer.grpc.pb.h)

add_custom_command(
    OUTPUT
        ${PROTO_SRCS}
        ${PROTO_HDRS}
        ${GRPC_SRCS}
        ${GRPC_HDRS}
    COMMAND protobuf::protoc
        --grpc_out=${GENERATED_DIR}
        --cpp_out=${GENERATED_DIR}
        -I ${CMAKE_CURRENT_SOURCE_DIR}/common/proto
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
)

# -----------------------
# Common library
# -----------------------
add_library(common STATIC
    common/src/scene_parser.cpp
    common/src/serialization.cpp
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

target_include_directories(common PUBLIC
    common/include
    ${GENERATED_DIR}
    render/include
)

target_link_libraries(common PUBLIC
    protobuf::libprotobuf
    gRPC::grpc++
)

# -----------------------
# Render core library
# -----------------------
add_library(render_core STATIC
    render/src/camera.cpp
    render/src/renderer.cpp
    render/src/material.cpp
    render/src/sphere.cpp
    render/src/cylinder.cpp
    render/src/hittable_list.cpp
    render/src/bvh.cpp
    render/src/color.cpp
)

target_include_directories(render_core PUBLIC
    render/include
)

# -----------------------
# OpenMP (portable)
# -----------------------
if(APPLE)
    find_package(OpenMP QUIET)

    if(OpenMP_FOUND)
        target_link_libraries(render_core PUBLIC OpenMP::OpenMP_CXX)
    else()
        set(LIBOMP_ROOT "/opt/homebrew/opt/libomp")
        set(LIBOMP_LIB "${LIBOMP_ROOT}/lib/libomp.dylib")

        if(EXISTS "${LIBOMP_LIB}")
            message(STATUS "OpenMP not found by CMake, configuring manually for Homebrew libomp")
            target_compile_options(render_core PUBLIC "-Xpreprocessor" "-fopenmp")
            target_include_directories(render_core PUBLIC "${LIBOMP_ROOT}/include")
            target_link_libraries(render_core PUBLIC "${LIBOMP_LIB}")
        else()
            message(WARNING "OpenMP not found; building without OpenMP support")
        endif()
    endif()
else()
    find_package(OpenMP REQUIRED)
    target_link_libraries(render_core PUBLIC OpenMP::OpenMP_CXX)
endif()

# -----------------------
# CLI parsing (cxxopts)
# -----------------------
include(FetchContent)
FetchContent_Declare(
    cxxopts
    GIT_REPOSITORY https://github.com/jarro2783/cxxopts.git
    GIT_TAG v3.0.0
)
FetchContent_MakeAvailable(cxxopts)

# -----------------------
# Executables
# -----------------------
add_executable(render
    render/src/main.cpp
)

add_executable(master
    master/main.cpp
    master/master.cpp
)

add_executable(worker
    worker/main.cpp
    worker/worker.cpp
)

# -----------------------
# Final linkage
# -----------------------
target_link_libraries(render PRIVATE
    render_core
    common
    cxxopts::cxxopts
)

target_link_libraries(master PRIVATE
    render_core
    common
    gRPC::grpc++
    protobuf::libprotobuf
    cxxopts::cxxopts
)

target_link_libraries(worker PRIVATE
    render_core
    common
    gRPC::grpc++
    protobuf::libprotobuf
    cxxopts::cxxopts
)
