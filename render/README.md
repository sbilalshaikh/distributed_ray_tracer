# Renderer

This is a simple ray tracer that renders scenes described in a custom scene description language. It supports basic geometric primitives, materials, and lighting.

## Features

*   **Geometric Primitives:**
    *   Spheres
    *   Cylinders
*   **Materials:**
    *   Lambertian (diffuse)
    *   Metal (reflective)
    *   Dielectric (refractive)
*   **Lighting:**
    *   Diffuse (emissive) materials
*   **Camera:**
    *   Configurable position, look-at point, up vector, and vertical field of view (vfov).
*   **Command-line Controls:**
    *   Control image width, samples per pixel, and max ray depth.

## Code Structure

| File                      | Description                                                                      |
| ------------------------- | -------------------------------------------------------------------------------- |
| `render/src/main.cpp`       | The main entry point of the renderer. Parses command-line arguments, loads a scene, and starts the rendering process. |
| `render/include/renderer.hpp` | The header file for the `renderer` class.                                        |
| `render/src/renderer.cpp`   | The implementation of the `renderer` class, which contains the main rendering loop and the `ray_color` function. |
| `render/include/camera.hpp`   | The header file for the `camera` class.                                          |
| `render/src/camera.cpp`     | The implementation of the `camera` class, which handles ray generation.          |
| `render/include/hittable.hpp` | The header file for the `hittable` abstract base class.                          |
| `render/include/hittable_list.hpp` | The header file for the `hittable_list` class, which stores a list of hittable objects. |
| `render/src/hittable_list.cpp` | The implementation of the `hittable_list` class.                               |
| `render/include/sphere.hpp`   | The header file for the `sphere` class.                                          |
| `render/src/sphere.cpp`     | The implementation of the ray-sphere intersection logic.                         |
| `render/include/aabb.hpp`     | The header file for the `aabb` class (Axis-Aligned Bounding Box).              |
| `render/src/aabb.cpp`       | The implementation of the ray-AABB intersection logic.                           |
| `render/include/cylinder.hpp` | The header file for the `cylinder` class.                                        |
| `render/src/cylinder.cpp`   | The implementation of the ray-cylinder intersection logic.                       |
| `render/include/material.hpp` | The header file for the `material` abstract base class and its derived classes. |
| `render/src/material.cpp`   | The implementation of the `scatter` functions for the different materials.       |
| `render/include/ray.hpp`      | The header file for the `ray` class.                                             |
| `render/include/vec3.hpp`     | The header file for the `vec3` class, which is used for points, vectors, and colors. |
| `render/src/vec3.cpp`       | The implementation of the `vec3` class.                                          |
| `render/include/color.hpp`    | The header file for color utility functions.                                     |
| `render/src/color.cpp`      | The implementation of color utility functions.                                   |
| `render/include/math_utils.hpp` | The header file for math utility functions.                                    |

## Scene Grammar

The scene description language is a simple text-based format. For a detailed description of the grammar, please see the `README.md` file in the `common/` directory.

## Usage

To build the renderer, create a `build` directory and run `cmake` and `make` from there:

```bash
mkdir build
cd build
cmake ..
make
```

The renderer uses OpenMP for CPU parralelism. You have an OpenMP installation that the compiler can link to. 

To run the renderer, use the `render` executable. It takes a scene file as a command-line argument. You can also specify the width, samples per pixel, and max depth:

```bash
./render [scene_file] [--width <pixels>] [--samples <count>] [--depth <count>] > output.ppm
```

For example, to render the `showcase.scene` at 1920 pixels wide with 1000 samples per pixel and a max depth of 50, you would run:

```bash
./render ../render/examples/showcase.scene --width 1920 --samples 1000 --depth 50 > showcase.ppm
```
