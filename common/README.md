# Scene Grammar

This document describes the grammar for the scene description language used by the ray tracer.

## Comments

Lines beginning with a `#` are treated as comments and are ignored by the parser.

## Materials

Materials are defined using the `material` keyword, followed by a name, a type, and the material's parameters.

### Lambertian

A Lambertian (diffuse) material is defined by its albedo (color).

```
material <name> lambertian <r> <g> <b>
```

*   `<name>`: The name of the material.
*   `<r>`, `g`, `b`: The RGB components of the albedo, where each component is a value between 0.0 and 1.0.

Example:

```
material my_diffuse lambertian 0.8 0.3 0.3
```

### Metal

A metal (reflective) material is defined by its albedo (color) and a fuzziness parameter.

```
material <name> metal <r> <g> <b> <fuzz>
```

*   `<name>`: The name of the material.
*   `<r>`, `g`, `b`: The RGB components of the albedo, where each component is a value between 0.0 and 1.0.
*   `<fuzz>`: The fuzziness of the reflection, where 0.0 is a perfect mirror and 1.0 is a very blurry reflection.

Example:

```
material my_metal metal 0.8 0.8 0.8 0.1
```

### Dielectric

A dielectric (refractive) material is defined by its index of refraction.

```
material <name> dielectric <ir>
```

*   `<name>`: The name of the material.
*   `<ir>`: The index of refraction of the material.

Example:

```
material glass dielectric 1.5
```

### Diffuse Light

A diffuse light is an emissive material that emits light of a given color.

```
material <name> diffuse_light <r> <g> <b>
```

*   `<name>`: The name of the material.
*   `<r>`, `g`, `b`: The RGB components of the emitted light, where each component is a value greater than or equal to 0.0.

Example:

```
material my_light diffuse_light 15 15 15
```

## Objects

Objects are defined by their type, parameters, and material.

### Sphere

A sphere is defined by its center, radius, and material.

```
sphere <cx> <cy> <cz> <radius> <material_name>
```

*   `<cx>`, `<cy>`, `<cz>`: The coordinates of the center of the sphere.
*   `<radius>`: The radius of the sphere.
*   `<material_name>`: The name of the material to apply to the sphere.

Example:

```
sphere 0 0 -1 0.5 my_diffuse
```

### Cylinder

A cylinder is defined by its two endpoints, radius, and material.

```
cylinder <p1.x> <p1.y> <p1.z> <p2.x> <p2.y> <p2.z> <radius> <material_name>
```

*   `<p1.x>`, `<p1.y>`, `<p1.z>`: The coordinates of the first endpoint of the cylinder.
*   `<p2.x>`, `<p2.y>`, `<p2.z>`: The coordinates of the second endpoint of the cylinder.
*   `<radius>`: The radius of the cylinder.
*   `<material_name>`: The name of the material to apply to the cylinder.

Example:

```
cylinder 0 0 -1 0 1 -1 0.5 glass
```

## Camera

The camera is defined by its position, look-at point, up vector, and vertical field of view (vfov).

```
camera
position <x> <y> <z>
look_at <x> <y> <z>
up <x> <y> <z>
vfov <degrees>
end
```

Example:

```
camera
position 0 2 15
look_at 0 0 0
up 0 1 0
vfov 50
end
```
