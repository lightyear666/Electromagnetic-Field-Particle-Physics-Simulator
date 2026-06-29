#include "physics.h"
#include <cmath>

void PhysicsWorld::step(Scene& scene, double dt, double t) {
    spawnDue(scene, t, t + dt);

    for (auto& p : scene.particles) {
        if (!p.alive || p.mass <= 1e-9) continue;
        Vec2 force{0, 0};

        for (const auto& field : scene.electricFields) {
            if (!electricContains(field, p.position)) continue;
            Vec2 e = rectDirection(field.rect) * field.strength.eval(t);
            force += e * p.charge;
        }

        for (const auto& field : scene.magneticFields) {
            if (!magneticContains(field, p.position)) continue;
            double bz = field.strength.eval(t) * (field.intoPage ? -1.0 : 1.0);
            force += {p.charge * p.velocity.y * bz, -p.charge * p.velocity.x * bz};
        }

        Vec2 accel = force / p.mass;
        p.velocity += accel * dt;
        p.position += p.velocity * dt;
        if (p.trailEnabled && (p.trail.empty() || lengthSq(p.position - p.trail.back()) > 0.0004)) {
            p.trail.push_back(p.position);
            if (p.trail.size() > 1200) p.trail.erase(p.trail.begin());
        }

        for (const auto& plane : scene.planes) {
            if (plane.collisionEnabled) collideWithPlane(p, plane);
        }
    }
}

void PhysicsWorld::spawnDue(Scene& scene, double previousTime, double t) {
    for (auto& emitter : scene.emitters) {
        for (auto& entry : emitter.schedule) {
            if (entry.emitted || entry.time < previousTime || entry.time > t) continue;
            ParticleDef def = entry.kind == ParticleKind::Custom ? entry.custom : builtInParticle(entry.kind);
            Particle p;
            p.id = scene.nextParticleId++;
            p.position = emitter.position;
            p.velocity = rotate({entry.speed, 0.0}, emitter.rotation);
            p.mass = std::max(0.001, def.mass);
            p.charge = def.charge;
            p.radius = std::max(0.02, def.radius);
            p.color = def.color;
            p.birthTime = entry.time;
            p.trailEnabled = entry.drawTrail;
            if (p.trailEnabled) p.trail.push_back(p.position);
            scene.particles.push_back(p);
            entry.emitted = true;
        }
    }
}

void PhysicsWorld::collideWithPlane(Particle& particle, const Plane& plane) {
    Vec2 local = worldToLocal(particle.position, plane.rect);
    double hx = plane.rect.width * 0.5;
    double hy = plane.rect.height * 0.5;
    Vec2 closest{clampd(local.x, -hx, hx), clampd(local.y, -hy, hy)};
    Vec2 delta = local - closest;
    double d2 = lengthSq(delta);

    Vec2 normalLocal{0, 0};
    double penetration = 0.0;

    if (d2 > 1e-12) {
        double d = std::sqrt(d2);
        if (d >= particle.radius) return;
        normalLocal = delta / d;
        penetration = particle.radius - d;
    } else if (std::abs(local.x) <= hx && std::abs(local.y) <= hy) {
        double px = hx - std::abs(local.x);
        double py = hy - std::abs(local.y);
        if (px < py) {
            normalLocal = {local.x >= 0 ? 1.0 : -1.0, 0.0};
            penetration = particle.radius + px;
        } else {
            normalLocal = {0.0, local.y >= 0 ? 1.0 : -1.0};
            penetration = particle.radius + py;
        }
    } else {
        return;
    }

    Vec2 normal = rotate(normalLocal, plane.rect.rotation);
    particle.position += normal * penetration;
    double vn = dot(particle.velocity, normal);
    if (vn < 0.0) {
        particle.velocity -= normal * ((1.0 + plane.restitution) * vn);
    }
}
