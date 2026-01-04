# Renderer

This is a ray tracer that renders scenes described in a custom scene description language. It supports basic geometric primitives, materials, lighting, and uses a Bounding Volume Hierarchy (BVH) for accelerated rendering.

## Features

*   **Geometric Primitives:**
    *   Spheres
    *   Cylinders
*   **Materials:**
    *   Lambertian (diffuse)
    *   Metal (reflective)
    *   Dielectric (refractive)
    *   Diffuse Light (emissive)
*   **Acceleration Structure:**
    *   Bounding Volume Hierarchy (BVH) for efficient ray intersection testing.
*   **Camera:**
    *   Configurable position, look-at point, up vector, and vertical field of view (vfov) via scene file.
    *   Automatic scene framing to focus on the main objects using the `--frame-scene` flag.
*   **Command-line Controls:**
    *   Control image width, samples per pixel, and max ray depth.
*   **Performance Optimizations:**
    *   Inlined `vec3` operations for reduced overhead.
    *   Optimized BVH construction and traversal.
    *   Efficient per-thread random number generation.

## Code Structure

The core rendering logic is encapsulated in the `render_core` static library, which is linked by the `render` executable and will be used by `master` and `worker` executables in a future distributed setup.

| File                      | Description                                                                      |
| ------------------------- | -------------------------------------------------------------------------------- |
| `render/src/main.cpp`       | The main entry point of the standalone `render` executable. Parses command-line arguments, loads a scene, and initiates rendering using `render_core`. |
| `render/include/aabb.hpp`     | The header file for the `aabb` (Axis-Aligned Bounding Box) utility class, used internally by the BVH. |
| `render/include/bvh.hpp`      | The header file for the `bvh_node` class, which implements the Bounding Volume Hierarchy acceleration structure. |
| `render/src/bvh.cpp`        | The implementation of the `bvh_node` class, including tree construction and optimized traversal. |
| `render/include/camera.hpp`   | The header file for the `camera` class.                                          |
| `render/src/camera.cpp`     | The implementation of the `camera` class, which handles ray generation.          |
| `render/include/color.hpp`    | The header file for color utility functions.                                     |
| `render/src/color.cpp`      | The implementation of color utility functions.                                   |
| `render/include/cylinder.hpp` | The header file for the `cylinder` primitive.                                    |
| `render/src/cylinder.cpp`   | The implementation of the ray-cylinder intersection logic.                       |
| `render/include/hittable.hpp` | The header file for the `hittable` abstract base class, defining the interface for ray-traceable objects. |
| `render/include/hittable_list.hpp` | The header file for the `hittable_list` class, which stores a list of hittable objects (can represent the scene or a BVH node's children). |
| `render/src/hittable_list.cpp` | The implementation of the `hittable_list` class.                               |
| `render/include/interval.hpp` | A utility class for representing 1D intervals, used for ray `t_min` and `t_max`. |
| `render/include/material.hpp` | The header file for the `material` abstract base class and its derived classes (Lambertian, Metal, Dielectric, Diffuse Light). |
| `render/src/material.cpp`   | The implementation of the `scatter` and `emitted` functions for the different materials. |
| `render/include/math_utils.hpp` | The header file for general mathematical utility functions.                    |
| `render/include/ray.hpp`      | The header file for the `ray` class.                                             |
| `render/include/renderer.hpp` | The header file for the `renderer` class.                                        |
| `render/src/renderer.cpp`   | The implementation of the `renderer` class, containing the main rendering loop, parallelization, and `ray_color` function. |
| `render/include/sphere.hpp`   | The header file for the `sphere` primitive.                                      |
| `render/src/sphere.cpp`     | The implementation of the ray-sphere intersection logic.                         |
| `render/include/vec3.hpp`     | The header file for the `vec3` class, used for points, vectors, and colors, with inlined operations for performance. |

## Scene Grammar

The scene description language is a simple text-based format. For a detailed description of the grammar, please see the `README.md` file in the `common/` directory.

## Usage

To build the renderer, ensure you have CMake and a C++20 compatible compiler. Then, create a `build` directory and run `cmake` and `make` from there:

```bash
mkdir build
cd build
cmake ..
make
```

The renderer uses OpenMP for CPU parallelism. Your system must have an OpenMP installation that the compiler can link to.

### Running the Renderer

To run the standalone `render` executable, it takes an optional scene file as a command-line argument. If no file is provided, a default scene with four spheres is rendered.

You can also specify the following options:

| Flag                        | Description                                                                    |
| --------------------------- | ------------------------------------------------------------------------------ |
| `[scene_file]`              | Path to the `.scene` file to render.                                           |
| `--width <pixels>`          | Sets the width of the output image.                                            |
| `--samples <count>`         | Sets the number of anti-aliasing samples per pixel.                            |
| `--depth <count>`           | Sets the maximum ray bounce depth.                                             |
| `--frame-scene`             | Automatically adjusts the camera to frame the main objects in the scene.       |

**Example:**

To render the `stress_test.scene` at a resolution of 1280px wide with auto-framing:

```bash
./render ../render/examples/stress_test.scene --width 1280 --frame-scene > stress_test_framed.ppm
```