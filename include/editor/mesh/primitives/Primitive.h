#ifndef PRIMITIVE_H
#define PRIMITIVE_H

#include <functional>
#include "../../core/Entity.h"
#include "../../core/Scene.h"

#include "Sphere.h"
#include "Plane.h"
#include "Cube.h"
#include "Cylinder.h"
#include "Cone.h"
#include "Torus.h"

struct Primitive { 
    const char* name;
    std::function<Entity*(Scene*)> create;
};

static const std::vector<Primitive> primitiveList = {
    { "Plane",    [](Scene* s){ return s->createEntity<Plane>();    }},
    { "Cube",     [](Scene* s){ return s->createEntity<Cube>();     }},
    { "Sphere",   [](Scene* s){ return s->createEntity<Sphere>();   }},
    { "Cylinder", [](Scene* s){ return s->createEntity<Cylinder>(); }},
    { "Cone",     [](Scene* s){ return s->createEntity<Cone>();     }},
    { "Torus",    [](Scene* s){ return s->createEntity<Torus>();    }}
};

#endif