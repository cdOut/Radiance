#ifndef RAYLIGHTLIST_H
#define RAYLIGHTLIST_H

#include "RayLight.h"

class RayLightList {
    public:
        std::vector<std::shared_ptr<RayLight>> lights;

        RayLightList() {};
        RayLightList(std::shared_ptr<RayLight> light) { add(light); }

        void clear() { lights.clear(); }

        void add(std::shared_ptr<RayLight> light) { 
            lights.push_back(light);
        }
};

#endif