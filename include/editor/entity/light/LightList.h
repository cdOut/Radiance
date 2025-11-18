#ifndef LIGHT_LIST_H
#define LIGHT_LIST_H

#include <functional>
#include "../Entity.h"
#include "../../Scene.h"
#include "Light.h"

struct LightData { 
    const char* name;
    std::function<Entity*(Scene*)> create;
};

static const std::vector<LightData> lightList = {
    { "Directional",    [](Scene* s){ return s->createEntity<DirectionalLight>();    }},
    { "Point",     [](Scene* s){ return s->createEntity<PointLight>();     }},
    { "Spotlight",   [](Scene* s){ return s->createEntity<SpotLight>();   }},
};

#endif