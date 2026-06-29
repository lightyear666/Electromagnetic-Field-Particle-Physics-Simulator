#pragma once
#include <cmath>
#include <algorithm>

constexpr double PI = 3.14159265358979323846;

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator*(Vec2 a, double s) { return {a.x * s, a.y * s}; }
inline Vec2 operator*(double s, Vec2 a) { return a * s; }
inline Vec2 operator/(Vec2 a, double s) { return {a.x / s, a.y / s}; }
inline Vec2& operator+=(Vec2& a, Vec2 b) { a.x += b.x; a.y += b.y; return a; }
inline Vec2& operator-=(Vec2& a, Vec2 b) { a.x -= b.x; a.y -= b.y; return a; }
inline Vec2& operator*=(Vec2& a, double s) { a.x *= s; a.y *= s; return a; }

inline double dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
inline double lengthSq(Vec2 a) { return dot(a, a); }
inline double length(Vec2 a) { return std::sqrt(lengthSq(a)); }
inline Vec2 normalize(Vec2 a) { double l = length(a); return l < 1e-9 ? Vec2{0, 0} : a / l; }
inline Vec2 rotate(Vec2 a, double r) { double c = std::cos(r), s = std::sin(r); return {a.x * c - a.y * s, a.x * s + a.y * c}; }
inline Vec2 invRotate(Vec2 a, double r) { return rotate(a, -r); }
inline double degToRad(double d) { return d * PI / 180.0; }
inline double radToDeg(double r) { return r * 180.0 / PI; }
inline double clampd(double v, double lo, double hi) { return std::max(lo, std::min(hi, v)); }

struct RectBody {
    Vec2 center;
    double width = 1.0;
    double height = 1.0;
    double rotation = 0.0;
};

inline Vec2 worldToLocal(Vec2 p, const RectBody& r) {
    return invRotate(p - r.center, r.rotation);
}

inline Vec2 localToWorld(Vec2 p, const RectBody& r) {
    return r.center + rotate(p, r.rotation);
}

inline bool pointInRect(Vec2 p, const RectBody& r) {
    Vec2 q = worldToLocal(p, r);
    return std::abs(q.x) <= r.width * 0.5 && std::abs(q.y) <= r.height * 0.5;
}

inline Vec2 rectDirection(const RectBody& r) {
    return rotate({1.0, 0.0}, r.rotation);
}
