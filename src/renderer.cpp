#include "renderer.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

static void color(Color c) { glColor4f(c.r, c.g, c.b, c.a); }

static void vertex(Vec2 p) { glVertex2d(p.x, p.y); }

static Vec2 corner(const RectBody& r, double sx, double sy) {
    return localToWorld({sx * r.width * 0.5, sy * r.height * 0.5}, r);
}

static void fillRectWorld(const RectBody& r, Color c) {
    color(c);
    glBegin(GL_QUADS);
    vertex(corner(r, -1, -1)); vertex(corner(r, 1, -1)); vertex(corner(r, 1, 1)); vertex(corner(r, -1, 1));
    glEnd();
}

static void outlineRectWorld(const RectBody& r, Color c, double width = 1.0) {
    color(c);
    glLineWidth((float)width);
    glBegin(GL_LINE_LOOP);
    vertex(corner(r, -1, -1)); vertex(corner(r, 1, -1)); vertex(corner(r, 1, 1)); vertex(corner(r, -1, 1));
    glEnd();
    glLineWidth(1.0f);
}

static void lineWorld(Vec2 a, Vec2 b, Color c, double width = 1.0) {
    color(c);
    glLineWidth((float)width);
    glBegin(GL_LINES);
    vertex(a); vertex(b);
    glEnd();
    glLineWidth(1.0f);
}

static void circleWorld(Vec2 p, double r, Color c, bool fill) {
    color(c);
    glBegin(fill ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
    if (fill) vertex(p);
    for (int i = 0; i <= 36; ++i) {
        double a = i * 2.0 * PI / 36.0;
        vertex({p.x + std::cos(a) * r, p.y + std::sin(a) * r});
    }
    glEnd();
}

static Vec2 triangleCorner(const MagneticField& f, int index) {
    return magneticTriangleCornerWorld(f, index);
}

static Vec2 triangleCorner(const ElectricField& f, int index) {
    return electricTriangleCornerWorld(f, index);
}

static void fillTriangleWorld(const MagneticField& f, Color c) {
    color(c);
    glBegin(GL_TRIANGLES);
    vertex(triangleCorner(f, 0)); vertex(triangleCorner(f, 1)); vertex(triangleCorner(f, 2));
    glEnd();
}

static void fillTriangleWorld(const ElectricField& f, Color c) {
    color(c);
    glBegin(GL_TRIANGLES);
    vertex(triangleCorner(f, 0)); vertex(triangleCorner(f, 1)); vertex(triangleCorner(f, 2));
    glEnd();
}

static void outlineTriangleWorld(const MagneticField& f, Color c, double width = 1.0) {
    color(c);
    glLineWidth((float)width);
    glBegin(GL_LINE_LOOP);
    vertex(triangleCorner(f, 0)); vertex(triangleCorner(f, 1)); vertex(triangleCorner(f, 2));
    glEnd();
    glLineWidth(1.0f);
}

static void outlineTriangleWorld(const ElectricField& f, Color c, double width = 1.0) {
    color(c);
    glLineWidth((float)width);
    glBegin(GL_LINE_LOOP);
    vertex(triangleCorner(f, 0)); vertex(triangleCorner(f, 1)); vertex(triangleCorner(f, 2));
    glEnd();
    glLineWidth(1.0f);
}

static void fillRingWorld(const ElectricField& f, Color c) {
    color(c);
    double inner = std::min(f.innerRadius, f.radius * 0.95);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 72; ++i) {
        double a = i * 2.0 * PI / 72.0;
        Vec2 outer{std::cos(a) * f.radius, std::sin(a) * f.radius};
        Vec2 in{std::cos(a) * inner, std::sin(a) * inner};
        vertex(localToWorld(outer, f.rect));
        vertex(localToWorld(in, f.rect));
    }
    glEnd();
}

static void fillRingWorld(const MagneticField& f, Color c) {
    color(c);
    double inner = std::min(f.innerRadius, f.radius * 0.95);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i <= 72; ++i) {
        double a = i * 2.0 * PI / 72.0;
        Vec2 outer{std::cos(a) * f.radius, std::sin(a) * f.radius};
        Vec2 in{std::cos(a) * inner, std::sin(a) * inner};
        vertex(localToWorld(outer, f.rect));
        vertex(localToWorld(in, f.rect));
    }
    glEnd();
}

static void drawMagneticShape(const MagneticField& f, Color fill, Color outline, double width = 1.5) {
    if (f.shape == FieldShape::Rectangle) {
        fillRectWorld(f.rect, fill);
        outlineRectWorld(f.rect, outline, width);
    } else if (f.shape == FieldShape::Triangle) {
        fillTriangleWorld(f, fill);
        outlineTriangleWorld(f, outline, width);
    } else if (f.shape == FieldShape::Circle) {
        circleWorld(f.rect.center, f.radius, fill, true);
        circleWorld(f.rect.center, f.radius, outline, false);
    } else if (f.shape == FieldShape::Ring) {
        fillRingWorld(f, fill);
        circleWorld(f.rect.center, f.radius, outline, false);
        circleWorld(f.rect.center, std::min(f.innerRadius, f.radius * 0.95), outline, false);
    }
}

static void drawElectricShape(const ElectricField& f, Color fill, Color outline, double width = 1.5) {
    if (f.shape == FieldShape::Rectangle) {
        fillRectWorld(f.rect, fill);
        outlineRectWorld(f.rect, outline, width);
    } else if (f.shape == FieldShape::Triangle) {
        fillTriangleWorld(f, fill);
        outlineTriangleWorld(f, outline, width);
    } else if (f.shape == FieldShape::Circle) {
        circleWorld(f.rect.center, f.radius, fill, true);
        circleWorld(f.rect.center, f.radius, outline, false);
    } else if (f.shape == FieldShape::Ring) {
        fillRingWorld(f, fill);
        circleWorld(f.rect.center, f.radius, outline, false);
        circleWorld(f.rect.center, std::min(f.innerRadius, f.radius * 0.95), outline, false);
    }
}

static void arrowWorld(Vec2 a, Vec2 b, Color c) {
    lineWorld(a, b, c, 1.0);
    Vec2 d = normalize(b - a);
    Vec2 n{-d.y, d.x};
    double s = 0.12;
    lineWorld(b, b - d * s + n * s * 0.55, c, 1.0);
    lineWorld(b, b - d * s - n * s * 0.55, c, 1.0);
}

bool Renderer::init(HWND hwnd) {
    hwnd_ = hwnd;
    hdc_ = GetDC(hwnd_);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 16;
    pfd.iLayerType = PFD_MAIN_PLANE;
    int pf = ChoosePixelFormat(hdc_, &pfd);
    if (!pf || !SetPixelFormat(hdc_, pf, &pfd)) return false;
    hglrc_ = wglCreateContext(hdc_);
    if (!hglrc_ || !wglMakeCurrent(hdc_, hglrc_)) return false;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    fontBase_ = glGenLists(96);
    HFONT font = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "Consolas");
    HFONT old = (HFONT)SelectObject(hdc_, font);
    wglUseFontBitmapsA(hdc_, 32, 96, fontBase_);
    SelectObject(hdc_, old);
    DeleteObject(font);
    return true;
}

void Renderer::shutdown() {
    if (hglrc_) {
        wglMakeCurrent(nullptr, nullptr);
        if (fontBase_) glDeleteLists(fontBase_, 96);
        wglDeleteContext(hglrc_);
        hglrc_ = nullptr;
    }
    if (hwnd_ && hdc_) {
        ReleaseDC(hwnd_, hdc_);
        hdc_ = nullptr;
    }
}

ScreenRect Renderer::drawViewport(const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH) const {
    (void)view;
    int areaX = leftPanelW;
    int areaY = timelineH;
    int areaW = std::max(1, clientW - leftPanelW);
    int areaH = std::max(1, clientH - timelineH);
    return {areaX, areaY, areaW, areaH};
}

static Vec2 effectiveWorldSize(const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH) {
    int drawW = std::max(1, clientW - leftPanelW);
    int drawH = std::max(1, clientH - timelineH);
    double targetAspect = view.worldHeight <= 1e-9 ? 1.0 : view.worldWidth / view.worldHeight;
    double drawAspect = drawW / (double)drawH;
    if (drawAspect > targetAspect) {
        return {view.worldHeight * drawAspect, view.worldHeight};
    }
    return {view.worldWidth, view.worldWidth / std::max(1e-9, drawAspect)};
}

void Renderer::setupWorld(const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH) {
    ScreenRect vp = drawViewport(view, clientW, clientH, leftPanelW, timelineH);
    Vec2 world = effectiveWorldSize(view, clientW, clientH, leftPanelW, timelineH);
    glViewport(vp.x, vp.y, vp.w, vp.h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(view.center.x - world.x * 0.5,
            view.center.x + world.x * 0.5,
            view.center.y - world.y * 0.5,
            view.center.y + world.y * 0.5,
            -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Renderer::setupScreen(int clientW, int clientH) {
    glViewport(0, 0, std::max(1, clientW), std::max(1, clientH));
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, clientW, clientH, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Renderer::drawText(double x, double y, const char* text) {
    if (!fontBase_ || !text) return;
    glRasterPos2d(x, y);
    glPushAttrib(GL_LIST_BIT);
    glListBase(fontBase_ - 32);
    glCallLists((GLsizei)std::strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();
}

void Renderer::drawGrid(const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH) {
    setupWorld(view, clientW, clientH, leftPanelW, timelineH);
    Vec2 world = effectiveWorldSize(view, clientW, clientH, leftPanelW, timelineH);
    double left = view.center.x - world.x * 0.5;
    double right = view.center.x + world.x * 0.5;
    double bottom = view.center.y - world.y * 0.5;
    double top = view.center.y + world.y * 0.5;
    double stepX = view.worldWidth / std::max(1, view.tickX);
    double stepY = view.worldHeight / std::max(1, view.tickY);
    double startX = std::floor(left / stepX) * stepX;
    double startY = std::floor(bottom / stepY) * stepY;
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glColor4f(0.18f, 0.22f, 0.28f, 0.75f);
    for (double x = startX; x <= right + stepX * 0.5; x += stepX) {
        glVertex2d(x, bottom); glVertex2d(x, top);
    }
    for (double y = startY; y <= top + stepY * 0.5; y += stepY) {
        glVertex2d(left, y); glVertex2d(right, y);
    }
    glColor4f(0.78f, 0.82f, 0.88f, 0.92f);
    glVertex2d(left, 0); glVertex2d(right, 0);
    glVertex2d(0, bottom); glVertex2d(0, top);
    glEnd();

    setupScreen(clientW, clientH);
    glColor4f(0.72f, 0.78f, 0.84f, 1.0f);
    char buf[96];
    std::snprintf(buf, sizeof(buf), "World: %.1f x %.1f %s, center %.2f, %.2f", view.worldWidth, view.worldHeight, view.unit.c_str(), view.center.x, view.center.y);
    drawText(leftPanelW + 12, 22, buf);
    drawText(leftPanelW + clientW - leftPanelW - 42, clientH - timelineH - 10, "X");
    drawText(leftPanelW + (clientW - leftPanelW) / 2 + 8, 18, "Y");
}

void Renderer::drawScene(const Scene& scene, const SimulationSettings& sim, ObjectRef selected) {
    for (const auto& f : scene.electricFields) {
        drawElectricShape(f, {0.1f, 0.42f, 0.95f, 0.16f}, {0.25f, 0.62f, 1.0f, 0.78f}, 1.5);
        double spanW = (f.shape == FieldShape::Circle || f.shape == FieldShape::Ring) ? f.radius * 2.0 : f.rect.width;
        double spanH = (f.shape == FieldShape::Circle || f.shape == FieldShape::Ring) ? f.radius * 2.0 : f.rect.height;
        int nx = std::max(2, (int)(spanW / 0.8));
        int ny = std::max(1, (int)(spanH / 0.7));
        double sign = f.strength.eval(sim.time) >= 0.0 ? 1.0 : -1.0;
        for (int ix = 0; ix < nx; ++ix) for (int iy = 0; iy < ny; ++iy) {
            double lx = -spanW * 0.35 + ix * spanW * 0.7 / std::max(1, nx - 1);
            double ly = -spanH * 0.32 + iy * spanH * 0.64 / std::max(1, ny - 1);
            Vec2 center = localToWorld({lx, ly}, f.rect);
            if (!electricContains(f, center)) continue;
            Vec2 a = localToWorld({lx - 0.18 * sign, ly}, f.rect);
            Vec2 b = localToWorld({lx + 0.18 * sign, ly}, f.rect);
            arrowWorld(a, b, {0.65f, 0.86f, 1.0f, 0.9f});
        }
    }

    for (const auto& f : scene.magneticFields) {
        double b = f.strength.eval(sim.time) * (f.intoPage ? -1.0 : 1.0);
        drawMagneticShape(f, {0.45f, 0.18f, 0.95f, 0.15f}, {0.72f, 0.54f, 1.0f, 0.82f}, 1.5);
        double spanW = (f.shape == FieldShape::Circle || f.shape == FieldShape::Ring) ? f.radius * 2.0 : f.rect.width;
        double spanH = (f.shape == FieldShape::Circle || f.shape == FieldShape::Ring) ? f.radius * 2.0 : f.rect.height;
        int nx = std::max(2, (int)(spanW / 0.65));
        int ny = std::max(1, (int)(spanH / 0.65));
        for (int ix = 0; ix < nx; ++ix) for (int iy = 0; iy < ny; ++iy) {
            double lx = -spanW * 0.38 + ix * spanW * 0.76 / std::max(1, nx - 1);
            double ly = -spanH * 0.35 + iy * spanH * 0.70 / std::max(1, ny - 1);
            Vec2 c = localToWorld({lx, ly}, f.rect);
            if (!magneticContains(f, c)) continue;
            if (b >= 0.0) {
                circleWorld(c, 0.055, {0.86f, 0.72f, 1.0f, 0.95f}, true);
                circleWorld(c, 0.13, {0.86f, 0.72f, 1.0f, 0.55f}, false);
            } else {
                Vec2 dx = rotate({0.12, 0.12}, f.rect.rotation);
                Vec2 dy = rotate({0.12, -0.12}, f.rect.rotation);
                lineWorld(c - dx, c + dx, {0.86f, 0.72f, 1.0f, 0.95f});
                lineWorld(c - dy, c + dy, {0.86f, 0.72f, 1.0f, 0.95f});
            }
        }
    }

    for (const auto& p : scene.planes) {
        fillRectWorld(p.rect, p.collisionEnabled ? Color{0.64f, 0.54f, 0.42f, 0.42f} : Color{0.45f, 0.45f, 0.45f, 0.16f});
        outlineRectWorld(p.rect, p.collisionEnabled ? Color{0.92f, 0.78f, 0.52f, 0.95f} : Color{0.7f, 0.7f, 0.7f, 0.35f}, 2.0);
    }

    for (const auto& e : scene.emitters) {
        Vec2 dir = rotate({1, 0}, e.rotation);
        Vec2 n{-dir.y, dir.x};
        Vec2 a = e.position + dir * 0.42;
        Vec2 b = e.position - dir * 0.32 + n * 0.24;
        Vec2 c = e.position - dir * 0.32 - n * 0.24;
        glColor4f(0.95f, 0.72f, 0.22f, 0.86f);
        glBegin(GL_TRIANGLES); vertex(a); vertex(b); vertex(c); glEnd();
        arrowWorld(e.position, e.position + dir * 0.9, {1.0f, 0.83f, 0.34f, 0.9f});
    }

    for (const auto& p : scene.particles) {
        if (!p.alive) continue;
        if (p.trailEnabled && p.trail.size() > 1) {
            glColor4f(p.color.r, p.color.g, p.color.b, 0.58f);
            glLineWidth(1.8f);
            glBegin(GL_LINE_STRIP);
            for (Vec2 q : p.trail) vertex(q);
            glEnd();
            glLineWidth(1.0f);
        }
        circleWorld(p.position, p.radius, p.color, true);
        circleWorld(p.position, p.radius, {0.06f, 0.06f, 0.06f, 0.95f}, false);
        Color mark{0.05f, 0.05f, 0.05f, 1.0f};
        if (p.charge > 1e-6) {
            lineWorld({p.position.x - p.radius * 0.45, p.position.y}, {p.position.x + p.radius * 0.45, p.position.y}, mark);
            lineWorld({p.position.x, p.position.y - p.radius * 0.45}, {p.position.x, p.position.y + p.radius * 0.45}, mark);
        } else if (p.charge < -1e-6) {
            lineWorld({p.position.x - p.radius * 0.45, p.position.y}, {p.position.x + p.radius * 0.45, p.position.y}, mark);
        }
    }

    if (valid(selected)) {
        if (selected.kind == ObjectKind::ElectricField) {
            for (const auto& f : scene.electricFields) if (f.id == selected.id) drawElectricShape(f, {0, 0, 0, 0}, {1.0f, 0.95f, 0.28f, 1.0f}, 2.5);
        } else if (selected.kind == ObjectKind::MagneticField) {
            for (const auto& f : scene.magneticFields) if (f.id == selected.id) drawMagneticShape(f, {0, 0, 0, 0}, {1.0f, 0.95f, 0.28f, 1.0f}, 2.5);
        } else {
            outlineRectWorld(scene.rectOf(selected), {1.0f, 0.95f, 0.28f, 1.0f}, 2.5);
        }
    }
}

void Renderer::drawTimeline(const SimulationSettings& sim, int clientW, int clientH, int leftPanelW, int timelineH) {
    setupScreen(clientW, clientH);
    int y = clientH - timelineH;
    glColor4f(0.055f, 0.06f, 0.075f, 0.98f);
    glBegin(GL_QUADS); glVertex2i(0, y); glVertex2i(clientW, y); glVertex2i(clientW, clientH); glVertex2i(0, clientH); glEnd();

    int bx = leftPanelW + 18;
    int by = y + 18;
    int bw = 86;
    int bh = 34;
    glColor4f(0.18f, 0.24f, 0.34f, 1.0f);
    glBegin(GL_QUADS); glVertex2i(bx, by); glVertex2i(bx + bw, by); glVertex2i(bx + bw, by + bh); glVertex2i(bx, by + bh); glEnd();
    glColor4f(0.92f, 0.94f, 0.98f, 1.0f);
    drawText(bx + 18, by + 22, sim.playing ? "Pause" : "Play");

    int rx = bx + bw + 10;
    glColor4f(0.18f, 0.20f, 0.25f, 1.0f);
    glBegin(GL_QUADS); glVertex2i(rx, by); glVertex2i(rx + 68, by); glVertex2i(rx + 68, by + bh); glVertex2i(rx, by + bh); glEnd();
    glColor4f(0.92f, 0.94f, 0.98f, 1.0f);
    drawText(rx + 15, by + 22, "Reset");

    int sx0 = rx + 92;
    int sx1 = clientW - 156;
    int sy = y + timelineH / 2;
    glColor4f(0.32f, 0.36f, 0.42f, 1.0f);
    glLineWidth(5.0f);
    glBegin(GL_LINES); glVertex2i(sx0, sy); glVertex2i(sx1, sy); glEnd();
    glLineWidth(1.0f);
    double u = sim.duration <= 0.0 ? 0.0 : clampd(sim.time / sim.duration, 0.0, 1.0);
    int knob = sx0 + (int)((sx1 - sx0) * u);
    glColor4f(0.88f, 0.54f, 0.18f, 1.0f);
    glBegin(GL_QUADS); glVertex2i(knob - 7, sy - 13); glVertex2i(knob + 7, sy - 13); glVertex2i(knob + 7, sy + 13); glVertex2i(knob - 7, sy + 13); glEnd();

    char buf[64];
    std::snprintf(buf, sizeof(buf), "t = %.2f / %.1f s", sim.time, sim.duration);
    glColor4f(0.92f, 0.94f, 0.98f, 1.0f);
    drawText(clientW - 138, sy + 5, buf);
}

void Renderer::render(const Scene& scene, const ViewSettings& view, const SimulationSettings& sim, ObjectRef selected, int clientW, int clientH, int leftPanelW, int timelineH) {
    if (!hglrc_) return;
    wglMakeCurrent(hdc_, hglrc_);
    glClearColor(0.035f, 0.04f, 0.052f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    setupScreen(clientW, clientH);
    glColor4f(0.08f, 0.09f, 0.11f, 0.96f);
    glBegin(GL_QUADS); glVertex2i(0, 0); glVertex2i(leftPanelW, 0); glVertex2i(leftPanelW, clientH); glVertex2i(0, clientH); glEnd();

    drawGrid(view, clientW, clientH, leftPanelW, timelineH);
    setupWorld(view, clientW, clientH, leftPanelW, timelineH);
    drawScene(scene, sim, selected);
    drawTimeline(sim, clientW, clientH, leftPanelW, timelineH);
    SwapBuffers(hdc_);
}

bool Renderer::inDrawArea(int sx, int sy, int clientW, int clientH, int leftPanelW, int timelineH) const {
    return sx >= leftPanelW && sx < clientW && sy >= 0 && sy < clientH - timelineH;
}

Vec2 Renderer::screenToWorld(int sx, int sy, const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH) const {
    ScreenRect vp = drawViewport(view, clientW, clientH, leftPanelW, timelineH);
    Vec2 world = effectiveWorldSize(view, clientW, clientH, leftPanelW, timelineH);
    double nx = (sx - vp.x) / (double)std::max(1, vp.w);
    double ny = (clientH - sy - vp.y) / (double)std::max(1, vp.h);
    nx = clampd(nx, 0.0, 1.0);
    ny = clampd(ny, 0.0, 1.0);
    return {view.center.x - world.x * 0.5 + nx * world.x,
            view.center.y - world.y * 0.5 + ny * world.y};
}

Vec2 Renderer::worldUnitsPerPixel(const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH) const {
    ScreenRect vp = drawViewport(view, clientW, clientH, leftPanelW, timelineH);
    Vec2 world = effectiveWorldSize(view, clientW, clientH, leftPanelW, timelineH);
    return {world.x / std::max(1, vp.w), world.y / std::max(1, vp.h)};
}

double Renderer::timelineTimeFromMouse(int sx, const SimulationSettings& sim, int clientW, int leftPanelW) const {
    int sx0 = leftPanelW + 18 + 86 + 10 + 68 + 92;
    int sx1 = clientW - 156;
    double u = (sx - sx0) / (double)std::max(1, sx1 - sx0);
    return clampd(u, 0.0, 1.0) * sim.duration;
}
