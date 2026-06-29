#include "scene.h"
#include <algorithm>
#include <cmath>

static Function1D parsed(const char* s) {
    Function1D f;
    f.parseFromText(s);
    return f;
}

ObjectRef Scene::addElectricField(Vec2 p) {
    ElectricField f;
    f.id = nextObjectId++;
    f.rect.center = p;
    f.rect.width = 4.0;
    f.rect.height = 2.4;
    f.strength = parsed("4");
    electricFields.push_back(f);
    return {ObjectKind::ElectricField, f.id};
}

ObjectRef Scene::addMagneticField(Vec2 p) {
    MagneticField f;
    f.id = nextObjectId++;
    f.rect.center = p;
    f.rect.width = 4.0;
    f.rect.height = 2.4;
    f.strength = parsed("1.5");
    magneticFields.push_back(f);
    return {ObjectKind::MagneticField, f.id};
}

ObjectRef Scene::addPlane(Vec2 p) {
    Plane plane;
    plane.id = nextObjectId++;
    plane.rect.center = p;
    plane.rect.width = 5.0;
    plane.rect.height = 0.45;
    planes.push_back(plane);
    return {ObjectKind::Plane, plane.id};
}

ObjectRef Scene::addEmitter(Vec2 p) {
    ParticleEmitter e;
    e.id = nextObjectId++;
    e.position = p;
    EmissionEntry entry;
    entry.time = 0.0;
    entry.kind = ParticleKind::Proton;
    entry.custom = builtInParticle(ParticleKind::Proton);
    entry.speed = 3.5;
    e.schedule.push_back(entry);
    emitters.push_back(e);
    return {ObjectKind::Emitter, e.id};
}

ObjectRef Scene::hitTest(Vec2 p) const {
    for (auto it = emitters.rbegin(); it != emitters.rend(); ++it) {
        RectBody r{it->position, 0.8, 0.8, it->rotation};
        if (pointInRect(p, r)) return {ObjectKind::Emitter, it->id};
    }
    for (auto it = planes.rbegin(); it != planes.rend(); ++it) if (pointInRect(p, it->rect)) return {ObjectKind::Plane, it->id};
    for (auto it = magneticFields.rbegin(); it != magneticFields.rend(); ++it) if (magneticContains(*it, p)) return {ObjectKind::MagneticField, it->id};
    for (auto it = electricFields.rbegin(); it != electricFields.rend(); ++it) if (electricContains(*it, p)) return {ObjectKind::ElectricField, it->id};
    return {};
}

RectBody Scene::rectOf(ObjectRef ref) const {
    if (ref.kind == ObjectKind::ElectricField) for (const auto& o : electricFields) if (o.id == ref.id) return o.rect;
    if (ref.kind == ObjectKind::MagneticField) for (const auto& o : magneticFields) if (o.id == ref.id) return o.rect;
    if (ref.kind == ObjectKind::Plane) for (const auto& o : planes) if (o.id == ref.id) return o.rect;
    if (ref.kind == ObjectKind::Emitter) for (const auto& o : emitters) if (o.id == ref.id) return {o.position, 0.8, 0.8, o.rotation};
    return {};
}

bool Scene::getCommon(ObjectRef ref, Vec2& p, double& rot, double* w, double* h) const {
    RectBody r = rectOf(ref);
    if (!valid(ref)) return false;
    p = r.center;
    rot = r.rotation;
    if (w) *w = r.width;
    if (h) *h = r.height;
    return true;
}

bool Scene::setCommon(ObjectRef ref, Vec2 p, double rot, double* w, double* h) {
    auto applyRect = [&](RectBody& r) {
        r.center = p;
        r.rotation = rot;
        if (w) r.width = std::max(0.05, *w);
        if (h) r.height = std::max(0.05, *h);
    };
    if (ref.kind == ObjectKind::ElectricField) for (auto& o : electricFields) if (o.id == ref.id) { applyRect(o.rect); return true; }
    if (ref.kind == ObjectKind::MagneticField) for (auto& o : magneticFields) if (o.id == ref.id) { applyRect(o.rect); return true; }
    if (ref.kind == ObjectKind::Plane) for (auto& o : planes) if (o.id == ref.id) { applyRect(o.rect); return true; }
    if (ref.kind == ObjectKind::Emitter) for (auto& o : emitters) if (o.id == ref.id) { o.position = p; o.rotation = rot; return true; }
    return false;
}

bool Scene::removeObject(ObjectRef ref) {
    if (ref.kind == ObjectKind::ElectricField) {
        auto old = electricFields.size();
        electricFields.erase(std::remove_if(electricFields.begin(), electricFields.end(), [&](const ElectricField& o) { return o.id == ref.id; }), electricFields.end());
        return electricFields.size() != old;
    }
    if (ref.kind == ObjectKind::MagneticField) {
        auto old = magneticFields.size();
        magneticFields.erase(std::remove_if(magneticFields.begin(), magneticFields.end(), [&](const MagneticField& o) { return o.id == ref.id; }), magneticFields.end());
        return magneticFields.size() != old;
    }
    if (ref.kind == ObjectKind::Plane) {
        auto old = planes.size();
        planes.erase(std::remove_if(planes.begin(), planes.end(), [&](const Plane& o) { return o.id == ref.id; }), planes.end());
        return planes.size() != old;
    }
    if (ref.kind == ObjectKind::Emitter) {
        auto old = emitters.size();
        emitters.erase(std::remove_if(emitters.begin(), emitters.end(), [&](const ParticleEmitter& o) { return o.id == ref.id; }), emitters.end());
        return emitters.size() != old;
    }
    return false;
}

void Scene::clearRuntime() {
    particles.clear();
    nextParticleId = 1;
    resetEmissionFlags();
}

void Scene::resetEmissionFlags() {
    for (auto& emitter : emitters) for (auto& entry : emitter.schedule) entry.emitted = false;
}

bool pointInTriangleLocal(Vec2 p, double w, double h) {
    Vec2 a{-w * 0.5, -h * 0.5};
    Vec2 b{-w * 0.5, h * 0.5};
    Vec2 c{w * 0.5, 0.0};
    auto sign = [](Vec2 p1, Vec2 p2, Vec2 p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    };
    double d1 = sign(p, a, b);
    double d2 = sign(p, b, c);
    double d3 = sign(p, c, a);
    bool hasNeg = d1 < 0 || d2 < 0 || d3 < 0;
    bool hasPos = d1 > 0 || d2 > 0 || d3 > 0;
    return !(hasNeg && hasPos);
}

static Vec2 triangleCornerFromParams(double leftDeg, double rightDeg, double baseLen, int index) {
    double base = std::max(0.05, baseLen);
    double left = degToRad(clampd(leftDeg, 5.0, 85.0));
    double right = degToRad(clampd(rightDeg, 5.0, 85.0));
    double cotLeft = 1.0 / std::tan(left);
    double cotRight = 1.0 / std::tan(right);
    double depth = base / std::max(0.05, cotLeft + cotRight);
    double apexY = -base * 0.5 + depth * cotLeft;
    double baseX = -depth * 0.5;
    double apexX = depth * 0.5;
    if (index == 0) return {baseX, -base * 0.5};
    if (index == 1) return {baseX, base * 0.5};
    return {apexX, apexY};
}

static bool pointInTriangleByCorners(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
    auto sign = [](Vec2 p1, Vec2 p2, Vec2 p3) {
        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
    };
    double d1 = sign(p, a, b);
    double d2 = sign(p, b, c);
    double d3 = sign(p, c, a);
    bool hasNeg = d1 < 0 || d2 < 0 || d3 < 0;
    bool hasPos = d1 > 0 || d2 > 0 || d3 > 0;
    return !(hasNeg && hasPos);
}

Vec2 electricTriangleCornerLocal(const ElectricField& f, int index) {
    return triangleCornerFromParams(f.triangleLeftDeg, f.triangleRightDeg, f.triangleBase, index);
}

Vec2 electricTriangleCornerWorld(const ElectricField& f, int index) {
    return localToWorld(electricTriangleCornerLocal(f, index), f.rect);
}

void updateElectricTriangleBounds(ElectricField& f) {
    Vec2 a = electricTriangleCornerLocal(f, 0);
    Vec2 b = electricTriangleCornerLocal(f, 1);
    Vec2 c = electricTriangleCornerLocal(f, 2);
    double minX = std::min({a.x, b.x, c.x});
    double maxX = std::max({a.x, b.x, c.x});
    double minY = std::min({a.y, b.y, c.y});
    double maxY = std::max({a.y, b.y, c.y});
    f.rect.width = std::max(0.05, maxX - minX);
    f.rect.height = std::max(0.05, maxY - minY);
    f.radius = std::max(f.rect.width, f.rect.height) * 0.5;
}

Vec2 magneticTriangleCornerLocal(const MagneticField& f, int index) {
    return triangleCornerFromParams(f.triangleLeftDeg, f.triangleRightDeg, f.triangleBase, index);
}

Vec2 magneticTriangleCornerWorld(const MagneticField& f, int index) {
    return localToWorld(magneticTriangleCornerLocal(f, index), f.rect);
}

void updateMagneticTriangleBounds(MagneticField& f) {
    Vec2 a = magneticTriangleCornerLocal(f, 0);
    Vec2 b = magneticTriangleCornerLocal(f, 1);
    Vec2 c = magneticTriangleCornerLocal(f, 2);
    double minX = std::min({a.x, b.x, c.x});
    double maxX = std::max({a.x, b.x, c.x});
    double minY = std::min({a.y, b.y, c.y});
    double maxY = std::max({a.y, b.y, c.y});
    f.rect.width = std::max(0.05, maxX - minX);
    f.rect.height = std::max(0.05, maxY - minY);
    f.radius = std::max(f.rect.width, f.rect.height) * 0.5;
}

static bool pointInMagneticTriangleLocal(const MagneticField& f, Vec2 p) {
    Vec2 a = magneticTriangleCornerLocal(f, 0);
    Vec2 b = magneticTriangleCornerLocal(f, 1);
    Vec2 c = magneticTriangleCornerLocal(f, 2);
    return pointInTriangleByCorners(p, a, b, c);
}

static bool pointInElectricTriangleLocal(const ElectricField& f, Vec2 p) {
    Vec2 a = electricTriangleCornerLocal(f, 0);
    Vec2 b = electricTriangleCornerLocal(f, 1);
    Vec2 c = electricTriangleCornerLocal(f, 2);
    return pointInTriangleByCorners(p, a, b, c);
}

bool electricContains(const ElectricField& f, Vec2 worldPoint) {
    if (f.shape == FieldShape::Rectangle) return pointInRect(worldPoint, f.rect);
    Vec2 local = worldToLocal(worldPoint, f.rect);
    if (f.shape == FieldShape::Triangle) return pointInElectricTriangleLocal(f, local);
    double d2 = lengthSq(local);
    double outer2 = f.radius * f.radius;
    if (f.shape == FieldShape::Circle) return d2 <= outer2;
    if (f.shape == FieldShape::Ring) {
        double inner = std::min(f.innerRadius, f.radius * 0.95);
        return d2 <= outer2 && d2 >= inner * inner;
    }
    return false;
}

bool magneticContains(const MagneticField& f, Vec2 worldPoint) {
    if (f.shape == FieldShape::Rectangle) return pointInRect(worldPoint, f.rect);
    Vec2 local = worldToLocal(worldPoint, f.rect);
    if (f.shape == FieldShape::Triangle) return pointInMagneticTriangleLocal(f, local);
    double d2 = lengthSq(local);
    double outer2 = f.radius * f.radius;
    if (f.shape == FieldShape::Circle) return d2 <= outer2;
    if (f.shape == FieldShape::Ring) {
        double inner = std::min(f.innerRadius, f.radius * 0.95);
        return d2 <= outer2 && d2 >= inner * inner;
    }
    return false;
}

ParticleDef builtInParticle(ParticleKind kind) {
    switch (kind) {
        case ParticleKind::Electron: return {"Electron", 1.0, -1.0, 0.10, {0.26f, 0.95f, 0.45f, 1.0f}};
        case ParticleKind::Proton: return {"Proton", 1.6, 1.0, 0.13, {1.0f, 0.42f, 0.35f, 1.0f}};
        case ParticleKind::Neutral: return {"Neutral", 1.2, 0.0, 0.12, {0.86f, 0.86f, 0.76f, 1.0f}};
        case ParticleKind::Custom: return {"Custom", 1.0, 1.0, 0.12, {0.92f, 0.78f, 0.26f, 1.0f}};
    }
    return {};
}

const char* objectName(ObjectKind kind) {
    switch (kind) {
        case ObjectKind::ElectricField: return "Electric Field";
        case ObjectKind::MagneticField: return "Magnetic Field";
        case ObjectKind::Plane: return "Plane";
        case ObjectKind::Emitter: return "Emitter";
        default: return "World";
    }
}

const char* particleName(ParticleKind kind) {
    switch (kind) {
        case ParticleKind::Electron: return "Electron";
        case ParticleKind::Proton: return "Proton";
        case ParticleKind::Neutral: return "Neutral";
        case ParticleKind::Custom: return "Custom";
    }
    return "Custom";
}
