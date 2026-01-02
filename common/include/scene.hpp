#ifndef SCENE_H
#define SCENE_H

#include "hittable_list.hpp"
#include "vec3.hpp"

struct camera_desc {
    point3 position {0, 0, 1.5};
    point3 look_at  {0, 0, -1};
    vec3   up       {0, 1, 0};
    double vfov {45.0};
};

class scene {
    public:
        hittable_list world;
        camera_desc camera;
    private: 

};

#endif
