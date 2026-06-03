#pragma once
#include "baseHeader.h"
#include <vector>
#include <list>

struct WakeVortex {
    M3DVector3f position;
    M3DVector3f direction;
    float circulation;
    float radius;
    float turbulence;
    float age;
    float decayRate;
};

class WakeManager {
    std::list<WakeVortex> vortices;
    float maxAge = 20.0f;

public:
    void update(float dt) {
        for (auto it = vortices.begin(); it != vortices.end(); ) {
            it->age += dt;
            it->circulation *= exp(-it->decayRate * dt);
            
            // Advection by background wind
            extern M3DVector3f windVelocity;
            it->position[0] += windVelocity[0] * dt;
            it->position[1] += windVelocity[1] * dt;
            it->position[2] += windVelocity[2] * dt;
            
            if (it->age > maxAge || abs(it->circulation) < 0.01f) {
                it = vortices.erase(it);
            } else {
                ++it;
            }
        }
    }

    void addVortex(const WakeVortex& v) {
        if (vortices.size() > 1000) vortices.pop_front();
        vortices.push_back(v);
    }

    const std::list<WakeVortex>& getVortices() const { return vortices; }
    
    void clear() { vortices.clear(); }
};
