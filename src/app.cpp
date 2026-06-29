#include "app.h"
#include <windowsx.h>
#include <algorithm>
#include <commdlg.h>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>

static const char* SAVE_DIR = "C:/Users/29682/Desktop/test/";
static const char* SAVE_BASENAME = "physics_scene";

static std::string nextSavePath() {
    char path[512];
    for (int i = 1; i <= 999; ++i) {
        std::snprintf(path, sizeof(path), "%s%s+%03d.txt", SAVE_DIR, SAVE_BASENAME, i);
        std::ifstream probe(path);
        if (!probe.good()) return path;
    }
    return std::string(SAVE_DIR) + SAVE_BASENAME + "+999.txt";
}

static std::string chooseLoadPath(HWND owner) {
    char fileName[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFilter = "Physics Scene (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrInitialDir = SAVE_DIR;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = "txt";
    if (!GetOpenFileNameA(&ofn)) return {};
    return fileName;
}

bool App::init(HWND hwnd, HINSTANCE inst) {
    hwnd_ = hwnd;
    if (!renderer_.init(hwnd_)) return false;
    ui_.create(hwnd_, inst);
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    clientW_ = rc.right - rc.left;
    clientH_ = rc.bottom - rc.top;
    layout();
    ui_.sync(scene_, view_, sim_, selected_, toolMode_);
    SetTimer(hwnd_, 1, 16, nullptr);
    lastTick_ = GetTickCount();
    return true;
}

void App::shutdown() {
    KillTimer(hwnd_, 1);
    renderer_.shutdown();
}

bool App::saveScene() {
    std::string path = nextSavePath();
    std::ofstream out(path);
    if (!out) return false;
    out << "FIELD_SIM_1\n";
    out << "VIEW " << view_.center.x << ' ' << view_.center.y << ' ' << view_.worldWidth << ' ' << view_.worldHeight << ' ' << view_.tickX << ' ' << view_.tickY << ' ' << std::quoted(view_.unit) << "\n";
    out << "SIM " << sim_.duration << "\n";
    out << "ELECTRIC " << scene_.electricFields.size() << "\n";
    for (const auto& e : scene_.electricFields) {
        out << e.id << ' ' << e.rect.center.x << ' ' << e.rect.center.y << ' ' << e.rect.width << ' ' << e.rect.height << ' ' << e.rect.rotation << ' ' << std::quoted(e.strength.source) << ' ' << (int)e.shape << ' ' << e.radius << ' ' << e.innerRadius << ' ' << e.triangleLeftDeg << ' ' << e.triangleRightDeg << ' ' << e.triangleBase << "\n";
    }
    out << "MAGNETIC " << scene_.magneticFields.size() << "\n";
    for (const auto& m : scene_.magneticFields) {
        out << m.id << ' ' << m.rect.center.x << ' ' << m.rect.center.y << ' ' << m.rect.width << ' ' << m.rect.height << ' ' << m.rect.rotation << ' ' << std::quoted(m.strength.source) << ' ' << m.intoPage << ' ' << (int)m.shape << ' ' << m.radius << ' ' << m.innerRadius << ' ' << m.triangleLeftDeg << ' ' << m.triangleRightDeg << ' ' << m.triangleBase << "\n";
    }
    out << "PLANES " << scene_.planes.size() << "\n";
    for (const auto& p : scene_.planes) {
        out << p.id << ' ' << p.rect.center.x << ' ' << p.rect.center.y << ' ' << p.rect.width << ' ' << p.rect.height << ' ' << p.rect.rotation << ' ' << p.collisionEnabled << ' ' << p.restitution << "\n";
    }
    out << "EMITTERS " << scene_.emitters.size() << "\n";
    for (const auto& e : scene_.emitters) {
        out << e.id << ' ' << e.position.x << ' ' << e.position.y << ' ' << e.rotation << ' ' << e.schedule.size() << "\n";
        for (const auto& s : e.schedule) {
            out << s.time << ' ' << (int)s.kind << ' ' << s.speed << ' ' << s.custom.mass << ' ' << s.custom.charge << ' ' << s.custom.radius << ' '
                << s.custom.color.r << ' ' << s.custom.color.g << ' ' << s.custom.color.b << ' ' << s.custom.color.a << ' ' << s.drawTrail << "\n";
        }
    }
    return true;
}

bool App::loadScene() {
    std::string path = chooseLoadPath(hwnd_);
    if (path.empty()) return false;
    std::ifstream in(path);
    std::string tag;
    if (!(in >> tag) || tag != "FIELD_SIM_1") return false;

    Scene loaded;
    ViewSettings loadedView;
    SimulationSettings loadedSim = sim_;
    int maxObjectId = 0;

    if (!(in >> tag) || tag != "VIEW") return false;
    in >> loadedView.center.x >> loadedView.center.y >> loadedView.worldWidth >> loadedView.worldHeight >> loadedView.tickX >> loadedView.tickY >> std::quoted(loadedView.unit);
    if (!(in >> tag) || tag != "SIM") return false;
    in >> loadedSim.duration;

    size_t count = 0;
    if (!(in >> tag >> count) || tag != "ELECTRIC") return false;
    std::string line;
    std::getline(in, line);
    for (size_t i = 0; i < count; ++i) {
        ElectricField e;
        std::string source;
        int shape = 0;
        if (!std::getline(in, line)) return false;
        std::istringstream row(line);
        row >> e.id >> e.rect.center.x >> e.rect.center.y >> e.rect.width >> e.rect.height >> e.rect.rotation >> std::quoted(source);
        row >> shape >> e.radius >> e.innerRadius >> e.triangleLeftDeg >> e.triangleRightDeg >> e.triangleBase;
        e.strength.parseFromText(source);
        e.shape = (FieldShape)clampd(shape, 0, 3);
        if (e.shape == FieldShape::Triangle) updateElectricTriangleBounds(e);
        loaded.electricFields.push_back(e);
        maxObjectId = std::max(maxObjectId, e.id);
    }
    if (!(in >> tag >> count) || tag != "MAGNETIC") return false;
    std::getline(in, line);
    for (size_t i = 0; i < count; ++i) {
        MagneticField m;
        std::string source;
        int shape = 0;
        if (!std::getline(in, line)) return false;
        std::istringstream row(line);
        row >> m.id >> m.rect.center.x >> m.rect.center.y >> m.rect.width >> m.rect.height >> m.rect.rotation >> std::quoted(source) >> m.intoPage >> shape >> m.radius >> m.innerRadius;
        row >> m.triangleLeftDeg >> m.triangleRightDeg >> m.triangleBase;
        m.strength.parseFromText(source);
        m.shape = (FieldShape)clampd(shape, 0, 3);
        if (m.shape == FieldShape::Triangle) updateMagneticTriangleBounds(m);
        loaded.magneticFields.push_back(m);
        maxObjectId = std::max(maxObjectId, m.id);
    }
    if (!(in >> tag >> count) || tag != "PLANES") return false;
    for (size_t i = 0; i < count; ++i) {
        Plane p;
        in >> p.id >> p.rect.center.x >> p.rect.center.y >> p.rect.width >> p.rect.height >> p.rect.rotation >> p.collisionEnabled >> p.restitution;
        loaded.planes.push_back(p);
        maxObjectId = std::max(maxObjectId, p.id);
    }
    if (!(in >> tag >> count) || tag != "EMITTERS") return false;
    for (size_t i = 0; i < count; ++i) {
        ParticleEmitter e;
        size_t scheduleCount = 0;
        in >> e.id >> e.position.x >> e.position.y >> e.rotation >> scheduleCount;
        for (size_t j = 0; j < scheduleCount; ++j) {
            EmissionEntry s;
            int kind = 0;
            in >> s.time >> kind >> s.speed >> s.custom.mass >> s.custom.charge >> s.custom.radius >> s.custom.color.r >> s.custom.color.g >> s.custom.color.b >> s.custom.color.a >> s.drawTrail;
            s.kind = (ParticleKind)clampd(kind, 0, 3);
            e.schedule.push_back(s);
        }
        loaded.emitters.push_back(e);
        maxObjectId = std::max(maxObjectId, e.id);
    }
    loaded.nextObjectId = maxObjectId + 1;
    scene_ = loaded;
    view_ = loadedView;
    sim_.duration = std::max(0.1, loadedSim.duration);
    selected_ = {};
    sim_.playing = false;
    resetSimulation();
    ui_.sync(scene_, view_, sim_, selected_, toolMode_);
    return true;
}

void App::layout() {
    ui_.layout(clientW_, clientH_, LEFT_PANEL_W, TIMELINE_H);
    invalidate();
}

void App::render() {
    renderer_.render(scene_, view_, sim_, selected_, clientW_, clientH_, LEFT_PANEL_W, TIMELINE_H);
}

void App::invalidate() {
    if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
}

void App::refreshNow() {
    invalidate();
    if (hwnd_) UpdateWindow(hwnd_);
}

void App::advance() {
    DWORD now = GetTickCount();
    double elapsed = std::min(0.08, (now - lastTick_) / 1000.0);
    lastTick_ = now;
    if (!sim_.playing) return;
    double remaining = elapsed;
    while (remaining > 1e-9 && sim_.time < sim_.duration) {
        double dt = std::min(sim_.fixedDt, remaining);
        physics_.step(scene_, dt, sim_.time);
        sim_.time += dt;
        remaining -= dt;
    }
    if (sim_.time >= sim_.duration) {
        sim_.time = sim_.duration;
        sim_.playing = false;
        ui_.sync(scene_, view_, sim_, selected_, toolMode_);
    }
    refreshNow();
}

void App::resetSimulation() {
    scene_.clearRuntime();
    sim_.time = 0.0;
    refreshNow();
}

void App::seekTo(double target) {
    bool wasPlaying = sim_.playing;
    sim_.playing = false;
    scene_.clearRuntime();
    sim_.time = 0.0;
    target = clampd(target, 0.0, sim_.duration);
    while (sim_.time + sim_.fixedDt < target) {
        physics_.step(scene_, sim_.fixedDt, sim_.time);
        sim_.time += sim_.fixedDt;
    }
    double remain = target - sim_.time;
    if (remain > 1e-9) {
        physics_.step(scene_, remain, sim_.time);
        sim_.time = target;
    }
    sim_.playing = wasPlaying;
    refreshNow();
}

void App::select(ObjectRef ref) {
    selected_ = ref;
    ui_.sync(scene_, view_, sim_, selected_, toolMode_);
    invalidate();
}

void App::setTool(ToolMode mode) {
    toolMode_ = mode;
    ui_.sync(scene_, view_, sim_, selected_, toolMode_);
}

void App::onCommand(int id, int code) {
    if (id >= IDC_TRAIL_BASE && id < IDC_TRAIL_BASE + MAX_TRAIL_CHECKS) {
        if (ui_.updateEmissionTrail(scene_, selected_, id - IDC_TRAIL_BASE)) resetSimulation();
        return;
    }

    switch (id) {
        case IDC_SAVE_SCENE:
            ui_.setStatus(saveScene() ? "Scene saved." : "Save failed.");
            break;
        case IDC_LOAD_SCENE:
            ui_.setStatus(loadScene() ? "Scene loaded." : "Load failed.");
            break;
        case IDC_TOOL_SELECT: setTool(ToolMode::Select); break;
        case IDC_TOOL_EFIELD: setTool(ToolMode::AddElectricField); break;
        case IDC_TOOL_BFIELD: setTool(ToolMode::AddMagneticField); break;
        case IDC_TOOL_PLANE: setTool(ToolMode::AddPlane); break;
        case IDC_TOOL_EMITTER: setTool(ToolMode::AddEmitter); break;
        case IDC_APPLY:
            if (!((toolMode_ == ToolMode::AddElectricField || toolMode_ == ToolMode::AddMagneticField) && !valid(selected_))) {
                ui_.apply(scene_, view_, sim_, selected_);
                resetSimulation();
                ui_.sync(scene_, view_, sim_, selected_, toolMode_);
            }
            break;
        case IDC_ADD_ENTRY:
            if (ui_.addEmissionEntry(scene_, selected_)) resetSimulation();
            break;
        case IDC_DEL_ENTRY:
            if (ui_.deleteEmissionEntry(scene_, selected_)) resetSimulation();
            break;
        case IDC_DELETE_OBJ:
            if (valid(selected_) && scene_.removeObject(selected_)) {
                selected_ = {};
                resetSimulation();
                ui_.sync(scene_, view_, sim_, selected_, toolMode_);
            }
            break;
        case IDC_PARTICLE_KIND:
            if (code == CBN_SELCHANGE) ui_.updateParticleDefaults();
            break;
        case IDC_SHAPE_KIND:
            if (code == CBN_SELCHANGE) ui_.onShapeChanged();
            break;
    }
    invalidate();
}

void App::onLeftDown(int x, int y) {
    SetCapture(hwnd_);
    if (y >= clientH_ - TIMELINE_H) {
        int by = clientH_ - TIMELINE_H + 18;
        int bx = LEFT_PANEL_W + 18;
        if (x >= bx && x <= bx + 86 && y >= by && y <= by + 34) {
            sim_.playing = !sim_.playing;
            lastTick_ = GetTickCount();
            ui_.sync(scene_, view_, sim_, selected_, toolMode_);
            invalidate();
            return;
        }
        int rx = bx + 86 + 10;
        if (x >= rx && x <= rx + 68 && y >= by && y <= by + 34) {
            sim_.playing = false;
            resetSimulation();
            ui_.sync(scene_, view_, sim_, selected_, toolMode_);
            return;
        }
        draggingTimeline_ = true;
        seekTo(renderer_.timelineTimeFromMouse(x, sim_, clientW_, LEFT_PANEL_W));
        return;
    }

    if (!renderer_.inDrawArea(x, y, clientW_, clientH_, LEFT_PANEL_W, TIMELINE_H)) return;
    Vec2 world = renderer_.screenToWorld(x, y, view_, clientW_, clientH_, LEFT_PANEL_W, TIMELINE_H);

    if (toolMode_ == ToolMode::AddElectricField) {
        select(ui_.createElectric(scene_, world));
        setTool(ToolMode::Select);
        resetSimulation();
        return;
    }
    if (toolMode_ == ToolMode::AddMagneticField) {
        select(ui_.createMagnetic(scene_, world));
        setTool(ToolMode::Select);
        resetSimulation();
        return;
    }
    if (toolMode_ == ToolMode::AddPlane) {
        select(scene_.addPlane(world));
        setTool(ToolMode::Select);
        resetSimulation();
        return;
    }
    if (toolMode_ == ToolMode::AddEmitter) {
        select(scene_.addEmitter(world));
        setTool(ToolMode::Select);
        resetSimulation();
        return;
    }

    ObjectRef hit = scene_.hitTest(world);
    select(hit);
    if (valid(hit)) {
        RectBody r = scene_.rectOf(hit);
        draggingObject_ = true;
        dragOffset_ = r.center - world;
    } else {
        draggingView_ = true;
        lastMouseX_ = x;
        lastMouseY_ = y;
    }
}

void App::onMouseMove(int x, int y, WPARAM flags) {
    if (draggingTimeline_) {
        seekTo(renderer_.timelineTimeFromMouse(x, sim_, clientW_, LEFT_PANEL_W));
        return;
    }

    if (draggingView_ && (flags & MK_LBUTTON)) {
        Vec2 scale = renderer_.worldUnitsPerPixel(view_, clientW_, clientH_, LEFT_PANEL_W, TIMELINE_H);
        view_.center.x -= (x - lastMouseX_) * scale.x;
        view_.center.y += (y - lastMouseY_) * scale.y;
        lastMouseX_ = x;
        lastMouseY_ = y;
        refreshNow();
        return;
    }

    if (!draggingObject_ || !(flags & MK_LBUTTON) || !valid(selected_)) return;
    if (!renderer_.inDrawArea(x, y, clientW_, clientH_, LEFT_PANEL_W, TIMELINE_H)) return;
    Vec2 world = renderer_.screenToWorld(x, y, view_, clientW_, clientH_, LEFT_PANEL_W, TIMELINE_H) + dragOffset_;
    Vec2 p; double r = 0, w = 0, h = 0;
    scene_.getCommon(selected_, p, r, &w, &h);
    scene_.setCommon(selected_, world, r, &w, &h);
    scene_.clearRuntime();
    sim_.time = 0.0;
    refreshNow();
}

void App::onLeftUp() {
    draggingObject_ = false;
    draggingTimeline_ = false;
    draggingView_ = false;
    ReleaseCapture();
    ui_.sync(scene_, view_, sim_, selected_, toolMode_);
    invalidate();
}

void App::onMouseWheel(int x, int y, int delta) {
    POINT pt{x, y};
    ScreenToClient(hwnd_, &pt);
    if (!renderer_.inDrawArea(pt.x, pt.y, clientW_, clientH_, LEFT_PANEL_W, TIMELINE_H)) return;

    Vec2 before = renderer_.screenToWorld(pt.x, pt.y, view_, clientW_, clientH_, LEFT_PANEL_W, TIMELINE_H);
    double factor = delta > 0 ? 0.9 : 1.1;
    view_.worldWidth = clampd(view_.worldWidth * factor, 1.0, 10000.0);
    view_.worldHeight = clampd(view_.worldHeight * factor, 1.0, 10000.0);
    Vec2 after = renderer_.screenToWorld(pt.x, pt.y, view_, clientW_, clientH_, LEFT_PANEL_W, TIMELINE_H);
    view_.center += before - after;
    refreshNow();
}

LRESULT App::handle(UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_SIZE:
            clientW_ = LOWORD(lp);
            clientH_ = HIWORD(lp);
            layout();
            return 0;
        case WM_COMMAND:
            onCommand(LOWORD(wp), HIWORD(wp));
            return 0;
        case WM_LBUTTONDOWN:
            onLeftDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
            return 0;
        case WM_MOUSEMOVE:
            onMouseMove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), wp);
            return 0;
        case WM_LBUTTONUP:
            onLeftUp();
            return 0;
        case WM_MOUSEWHEEL:
            onMouseWheel(GET_X_LPARAM(lp), GET_Y_LPARAM(lp), GET_WHEEL_DELTA_WPARAM(wp));
            return 0;
        case WM_TIMER:
            advance();
            return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd_, &ps);
            render();
            EndPaint(hwnd_, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProc(hwnd_, msg, wp, lp);
}
