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

1.  **Install Dependencies:** Install the required dependencies using your system's package manager (gRPC/protobuf, OpenMP, CMake ≥ 3.15).
2.  **Configure with CMake:**
    ```bash
    cmake -S . -B build
    ```
3.  **Build all binaries:**
    ```bash
    cmake --build build
    ```

## Running the Raytracer

The project produces three executables: `render`, `master`, and `worker`.

### Standalone Renderer

The `render` executable is a standalone ray tracer that can render a scene without the master and worker nodes.

```bash
./render -s <scene_file> -o <output_file>
```

### Distributed Renderer

The distributed path uses a persistent master service and a pool of stateless workers. Each worker registers once, receives the immutable scene plus render settings, and then streams task requests until no more tiles remain.

1.  **Start the master node** (from the `build/` directory):
    ```bash
    ./master \
      --scene examples/stress_test.scene \
      --output output.ppm \
      --width 1920 \
      --height 1080 \
      --tile-size 64 \
      --samples 200 \
      --depth 50 \
      --port 50051
    ```
    The master validates image/tile dimensions, splits the image into uniquely identified tiles, and listens for worker registrations on the requested port.

2.  **Start one or more worker nodes** (can run locally or remotely):
    ```bash
    ./worker \
      --address master-host:50051 \
      --name kitchen-gpu
    ```
    `--name` (default `local-worker`) helps identify logs on the master. Each worker re-registers automatically if the master restarts or forgets its lease.

### Scene File

The renderer uses a custom file format to describe 3D scenes. Examples are in the `examples/` directory, and the grammar is documented alongside the parser in `common/`. During registration the master serializes the full scene graph (including BVH and camera) using `common/proto/raytracer.proto` and pushes it to every worker so tasks only need tile metadata.
