#pragma once
#include "function1d.h"
#include "math2d.h"
#include <string>
#include <vector>

struct Color {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

enum class ObjectKind { None, ElectricField, MagneticField, Plane, Emitter };
enum class ParticleKind { Electron, Proton, Neutral, Custom };
enum class FieldShape { Rectangle, Triangle, Circle, Ring };

struct ObjectRef {
    ObjectKind kind = ObjectKind::None;
    int id = -1;
};

inline bool valid(ObjectRef r) { return r.kind != ObjectKind::None && r.id >= 0; }

struct ElectricField {
    int id = -1;
    RectBody rect;
    Function1D strength;
    FieldShape shape = FieldShape::Rectangle;
    double radius = 1.6;
    double innerRadius = 0.7;
    double triangleLeftDeg = 60.0;
    double triangleRightDeg = 60.0;
    double triangleBase = 2.4;
};

struct MagneticField {
    int id = -1;
    RectBody rect;
    Function1D strength;
    bool intoPage = false;
    FieldShape shape = FieldShape::Rectangle;
    double radius = 1.6;
    double innerRadius = 0.7;
    double triangleLeftDeg = 60.0;
    double triangleRightDeg = 60.0;
    double triangleBase = 2.4;
};

struct Plane {
    int id = -1;
    RectBody rect;
    bool collisionEnabled = true;
    double restitution = 0.8;
};

struct ParticleDef {
    std::string name;
    double mass = 1.0;
    double charge = 0.0;
    double radius = 0.12;
    Color color;
};

struct EmissionEntry {
    double time = 0.0;
    ParticleKind kind = ParticleKind::Proton;
    ParticleDef custom;
    double speed = 3.0;
    bool emitted = false;
    bool drawTrail = false;
};

struct ParticleEmitter {
    int id = -1;
    Vec2 position;
    double rotation = 0.0;
    std::vector<EmissionEntry> schedule;
};

struct Particle {
    int id = -1;
    Vec2 position;
    Vec2 velocity;
    double mass = 1.0;
    double charge = 0.0;
    double radius = 0.12;
    Color color;
    double birthTime = 0.0;
    bool alive = true;
    bool trailEnabled = false;
    std::vector<Vec2> trail;
};

struct ViewSettings {
    Vec2 center{0.0, 0.0};
    double worldWidth = 20.0;
    double worldHeight = 12.0;
    int tickX = 20;
    int tickY = 12;
    std::string unit = "m";
};

struct SimulationSettings {
    double time = 0.0;
    double duration = 20.0;
    double fixedDt = 1.0 / 120.0;
    bool playing = false;
};

struct Scene {
    int nextObjectId = 1;
    int nextParticleId = 1;
    std::vector<ElectricField> electricFields;
    std::vector<MagneticField> magneticFields;
    std::vector<Plane> planes;
    std::vector<ParticleEmitter> emitters;
    std::vector<Particle> particles;

    ObjectRef addElectricField(Vec2 p);
    ObjectRef addMagneticField(Vec2 p);
    ObjectRef addPlane(Vec2 p);
    ObjectRef addEmitter(Vec2 p);
    ObjectRef hitTest(Vec2 p) const;
    RectBody rectOf(ObjectRef ref) const;
    bool getCommon(ObjectRef ref, Vec2& p, double& rot, double* w = nullptr, double* h = nullptr) const;
    bool setCommon(ObjectRef ref, Vec2 p, double rot, double* w = nullptr, double* h = nullptr);
    bool removeObject(ObjectRef ref);
    void clearRuntime();
    void resetEmissionFlags();
};

bool magneticContains(const MagneticField& f, Vec2 worldPoint);
bool electricContains(const ElectricField& f, Vec2 worldPoint);
Vec2 electricTriangleCornerLocal(const ElectricField& f, int index);
Vec2 electricTriangleCornerWorld(const ElectricField& f, int index);
void updateElectricTriangleBounds(ElectricField& f);
Vec2 magneticTriangleCornerLocal(const MagneticField& f, int index);
Vec2 magneticTriangleCornerWorld(const MagneticField& f, int index);
void updateMagneticTriangleBounds(MagneticField& f);
bool pointInTriangleLocal(Vec2 p, double w, double h);

ParticleDef builtInParticle(ParticleKind kind);
const char* objectName(ObjectKind kind);
const char* particleName(ParticleKind kind);
