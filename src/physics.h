#pragma once
#include "scene.h"

class PhysicsWorld {
public:
    void step(Scene& scene, double dt, double t);

private:
    void spawnDue(Scene& scene, double previousTime, double t);
    void collideWithPlane(Particle& particle, const Plane& plane);
};
