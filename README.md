# Distributed Raytracer

Distributed ray tracing engine written in C++. It uses a master-worker architecture to distribute the rendering of a scene across multiple nodes.

## Features

*   **Distributed Rendering:** The rendering of a single image is distributed across multiple worker nodes.
*   **Cross-Platform:** The project uses CMake for building and should compile on any platform with the required dependencies.
*   **Ray Tracing Features:**
    *   Spheres and Cylinders
    *   Lambertian, Metal, and Dielectric materials
    *   Bounding Volume Hierarchy (BVH) for acceleration
*   **gRPC for Communication:** The master and worker nodes communicate using gRPC.
*   **Scene File Format:** The scene is described in a simple text-based format.

## Project Structure

*   `common/`: Contains common code for serialization and scene parsing.
    *   `proto/`: The protobuf and gRPC definitions for communication.
*   `master/`: The master node, which distributes the rendering work.
*   `worker/`: The worker nodes, which perform the actual rendering.
*   `render/`: The core ray tracing engine.
*   `examples/`: Example scenes and images (ppm)

## Dependencies

*   **CMake:** A C++ build tool.
*   **gRPC and Protocol Buffers:** For communication between the master and worker nodes.
*   **OpenMP:** For parallel processing in the rendering engine.
*   **cxxopts:** For parsing command-line arguments.

## Building the Project

1.  **Install Dependencies:** Install the required dependencies using your system's package manager.
2.  **Create a build directory:**
    ```bash
    mkdir build && cd build
    ```
3.  **Run CMake:**
    ```bash
    cmake ..
    ```
4.  **Build the project:**
    ```bash
    make
    ```

## Running the Raytracer

The project produces three executables: `render`, `master`, and `worker`.

### Standalone Renderer

The `render` executable is a standalone ray tracer that can render a scene without the master and worker nodes.

```bash
./render -s <scene_file> -o <output_file>
```

### Distributed Renderer

To run the distributed renderer, you need to start the master node and one or more worker nodes.

1.  **Start the master node:**
    ```bash
    ./master -s <scene_file> -o <output_file>
    ```
2.  **Start one or more worker nodes:**
    ```bash
    ./worker -a <master_address>
    ```
    For example:
    ```bash
    ./worker -a localhost:50051
    ```

### Scene File

The renderer uses a custom file format to describe 3D scences. Examples are in the `examples/` directory
