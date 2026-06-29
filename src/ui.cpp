#include "ui.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

static std::string fmt(double v) {
    char b[64];
    std::snprintf(b, sizeof(b), "%.4g", v);
    return b;
}

static Color randomParticleColor() {
    static bool seeded = false;
    if (!seeded) {
        std::srand((unsigned)std::time(nullptr));
        seeded = true;
    }
    auto channel = []() { return 0.28f + (std::rand() / (float)RAND_MAX) * 0.72f; };
    Color c{channel(), channel(), channel(), 1.0f};
    if (c.r + c.g + c.b < 1.15f) c.g = 0.9f;
    return c;
}

HWND UIManager::makeLabel(const char* text) {
    return CreateWindowA("STATIC", text, WS_CHILD | WS_VISIBLE, 0, 0, 10, 10, parent_, nullptr, nullptr, nullptr);
}

HWND UIManager::makeEdit(const char* text) {
    return CreateWindowA("EDIT", text, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 0, 0, 10, 10, parent_, nullptr, nullptr, nullptr);
}

HWND UIManager::makeButton(const char* text, int id) {
    return CreateWindowA("BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 10, 10, parent_, (HMENU)(INT_PTR)id, nullptr, nullptr);
}

void UIManager::create(HWND parent, HINSTANCE inst) {
    parent_ = parent;
    title_ = makeLabel("2D Field Physics Lab");
    status_ = makeLabel("Select a tool or edit world settings.");
    save_ = makeButton("Save", IDC_SAVE_SCENE);
    load_ = makeButton("Load", IDC_LOAD_SCENE);
    toolSelect_ = makeButton("Select / Drag", IDC_TOOL_SELECT);
    toolE_ = makeButton("Rect Electric Field", IDC_TOOL_EFIELD);
    toolB_ = makeButton("Magnetic Field", IDC_TOOL_BFIELD);
    toolPlane_ = makeButton("Plane", IDC_TOOL_PLANE);
    toolEmitter_ = makeButton("Particle Emitter", IDC_TOOL_EMITTER);
    for (int i = 0; i < 9; ++i) {
        labels_[i] = makeLabel("");
        edits_[i] = makeEdit("");
    }
    collision_ = CreateWindowA("BUTTON", "Collision enabled", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 0, 10, 10, parent_, nullptr, inst, nullptr);
    combo_ = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 0, 10, 150, parent_, (HMENU)(INT_PTR)IDC_PARTICLE_KIND, inst, nullptr);
    SendMessageA(combo_, CB_ADDSTRING, 0, (LPARAM)"Electron");
    SendMessageA(combo_, CB_ADDSTRING, 0, (LPARAM)"Proton");
    SendMessageA(combo_, CB_ADDSTRING, 0, (LPARAM)"Neutral");
    SendMessageA(combo_, CB_ADDSTRING, 0, (LPARAM)"Custom");
    SendMessageA(combo_, CB_SETCURSEL, 1, 0);
    direction_ = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 0, 10, 100, parent_, (HMENU)(INT_PTR)IDC_MAG_DIRECTION, inst, nullptr);
    SendMessageA(direction_, CB_ADDSTRING, 0, (LPARAM)"Out of page (dots)");
    SendMessageA(direction_, CB_ADDSTRING, 0, (LPARAM)"Into page (X)");
    SendMessageA(direction_, CB_SETCURSEL, 0, 0);
    shapeCombo_ = CreateWindowA("COMBOBOX", "", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 0, 0, 10, 120, parent_, (HMENU)(INT_PTR)IDC_SHAPE_KIND, inst, nullptr);
    SendMessageA(shapeCombo_, CB_ADDSTRING, 0, (LPARAM)"Rectangle");
    SendMessageA(shapeCombo_, CB_ADDSTRING, 0, (LPARAM)"Triangle");
    SendMessageA(shapeCombo_, CB_ADDSTRING, 0, (LPARAM)"Circle");
    SendMessageA(shapeCombo_, CB_ADDSTRING, 0, (LPARAM)"Ring");
    SendMessageA(shapeCombo_, CB_SETCURSEL, 0, 0);
    addEntry_ = makeButton("Add emission", IDC_ADD_ENTRY);
    delEntry_ = makeButton("Delete emission", IDC_DEL_ENTRY);
    list_ = CreateWindowA("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL, 0, 0, 10, 10, parent_, (HMENU)(INT_PTR)IDC_ENTRY_LIST, inst, nullptr);
    for (int i = 0; i < MAX_TRAIL_CHECKS; ++i) {
        trailChecks_[i] = CreateWindowA("BUTTON", "", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0, 0, 20, 20, parent_, (HMENU)(INT_PTR)(IDC_TRAIL_BASE + i), inst, nullptr);
    }
    deleteObj_ = makeButton("Delete selected object", IDC_DELETE_OBJ);
    apply_ = makeButton("Apply", IDC_APPLY);
}

void UIManager::layout(int width, int height, int leftPanelW, int timelineH) {
    (void)width;
    int x = 14;
    int y = 12;
    int w = leftPanelW - 28;
    SetWindowPos(save_, nullptr, x, y, (w - 8) / 2, 28, SWP_NOZORDER);
    SetWindowPos(load_, nullptr, x + (w + 8) / 2, y, (w - 8) / 2, 28, SWP_NOZORDER); y += 36;
    SetWindowPos(title_, nullptr, x, y, w, 22, SWP_NOZORDER); y += 32;
    SetWindowPos(toolSelect_, nullptr, x, y, w, 28, SWP_NOZORDER); y += 34;
    SetWindowPos(toolE_, nullptr, x, y, w, 28, SWP_NOZORDER); y += 34;
    SetWindowPos(toolB_, nullptr, x, y, w, 28, SWP_NOZORDER); y += 34;
    SetWindowPos(toolPlane_, nullptr, x, y, w, 28, SWP_NOZORDER); y += 34;
    SetWindowPos(toolEmitter_, nullptr, x, y, w, 28, SWP_NOZORDER); y += 42;
    SetWindowPos(status_, nullptr, x, y, w, 38, SWP_NOZORDER); y += 48;

    int rowY = y;
    for (int i = 0; i < 9; ++i) {
        SetWindowPos(labels_[i], nullptr, x, y, 82, 22, SWP_NOZORDER);
        SetWindowPos(edits_[i], nullptr, x + 88, y - 2, w - 88, 24, SWP_NOZORDER);
        y += 30;
    }
    SetWindowPos(direction_, nullptr, x + 88, rowY + 6 * 30 - 2, w - 88, 100, SWP_NOZORDER);
    SetWindowPos(combo_, nullptr, x + 88, rowY + 8 * 30 - 2, w - 88, 160, SWP_NOZORDER);
    SetWindowPos(shapeCombo_, nullptr, x + 88, rowY + 8 * 30 - 2, w - 88, 120, SWP_NOZORDER);
    SetWindowPos(collision_, nullptr, x, y, w, 24, SWP_NOZORDER); y += 30;
    SetWindowPos(addEntry_, nullptr, x, y, (w - 8) / 2, 28, SWP_NOZORDER);
    SetWindowPos(delEntry_, nullptr, x + (w + 8) / 2, y, (w - 8) / 2, 28, SWP_NOZORDER); y += 34;

    int bottom = height - timelineH - 12;
    int applyY = bottom - 30;
    int deleteY = applyY - 34;
    listX_ = x;
    listY_ = y;
    listH_ = std::max(52, deleteY - y - 8);
    SetWindowPos(list_, nullptr, x, y, w - 32, listH_, SWP_NOZORDER);
    for (int i = 0; i < MAX_TRAIL_CHECKS; ++i) {
        SetWindowPos(trailChecks_[i], nullptr, x + w - 24, y + 4 + i * 18, 22, 18, SWP_NOZORDER);
    }
    SetWindowPos(deleteObj_, nullptr, x, deleteY, w, 28, SWP_NOZORDER);
    SetWindowPos(apply_, nullptr, x, applyY, w, 30, SWP_NOZORDER);
}

void UIManager::setVisible(HWND hwnd, bool visible) {
    if (hwnd) ShowWindow(hwnd, visible ? SW_SHOW : SW_HIDE);
}

void UIManager::setLabel(int index, const char* text) {
    if (index >= 0 && index < 9) SetWindowTextA(labels_[index], text);
}

void UIManager::setEdit(int index, const char* text) {
    if (index >= 0 && index < 9) SetWindowTextA(edits_[index], text);
}

std::string UIManager::getEdit(int index) const {
    char b[256]{};
    if (index >= 0 && index < 9) GetWindowTextA(edits_[index], b, sizeof(b));
    return b;
}

double UIManager::getDouble(int index, double fallback) const {
    std::string s = getEdit(index);
    char* end = nullptr;
    double v = std::strtod(s.c_str(), &end);
    return end && *end == 0 ? v : fallback;
}

int UIManager::getInt(int index, int fallback) const {
    return (int)getDouble(index, fallback);
}

void UIManager::hideAllOptional() {
    setVisible(collision_, false);
    setVisible(combo_, false);
    setVisible(direction_, false);
    setVisible(shapeCombo_, false);
    setVisible(addEntry_, false);
    setVisible(delEntry_, false);
    setVisible(list_, false);
    for (int i = 0; i < MAX_TRAIL_CHECKS; ++i) setVisible(trailChecks_[i], false);
    setVisible(deleteObj_, false);
}

void UIManager::applyShapeLabels(int shapeIdx) {
    bool circle = shapeIdx == (int)FieldShape::Circle;
    bool ring = shapeIdx == (int)FieldShape::Ring;
    bool triangle = shapeIdx == (int)FieldShape::Triangle;
    setLabel(3, circle || ring ? "Radius" : (triangle ? "Left deg" : "Width"));
    setLabel(4, ring ? "Inner R" : (triangle ? "Right deg" : "Height"));
    setLabel(7, triangle ? "Base" : "");
    setVisible(labels_[4], !circle);
    setVisible(edits_[4], !circle);
    setVisible(labels_[7], triangle);
    setVisible(edits_[7], triangle);
}

void UIManager::onShapeChanged() {
    int shapeIdx = (int)SendMessageA(shapeCombo_, CB_GETCURSEL, 0, 0);
    if (shapeIdx < 0 || shapeIdx > 3) shapeIdx = 0;
    applyShapeLabels(shapeIdx);
    if (shapeIdx == (int)FieldShape::Triangle) {
        setEdit(3, "60");
        setEdit(4, "60");
        setEdit(7, "2.4");
    } else if (shapeIdx == (int)FieldShape::Circle) {
        setEdit(3, "1.6");
    } else if (shapeIdx == (int)FieldShape::Ring) {
        setEdit(3, "1.6");
        setEdit(4, "0.7");
    } else {
        setEdit(3, "4");
        setEdit(4, "2.4");
    }
}

void UIManager::fillEmitterList(const ParticleEmitter& emitter) {
    SendMessageA(list_, LB_RESETCONTENT, 0, 0);
    char b[256];
    for (int i = 0; i < MAX_TRAIL_CHECKS; ++i) setVisible(trailChecks_[i], false);
    for (size_t i = 0; i < emitter.schedule.size(); ++i) {
        const auto& e = emitter.schedule[i];
        std::snprintf(b, sizeof(b), "%02zu  %s  t=%.3gs  q=%.3g", i + 1, particleName(e.kind), e.time, (e.kind == ParticleKind::Custom ? e.custom.charge : builtInParticle(e.kind).charge));
        SendMessageA(list_, LB_ADDSTRING, 0, (LPARAM)b);
        if (i < MAX_TRAIL_CHECKS) {
            setVisible(trailChecks_[i], true);
            SendMessageA(trailChecks_[i], BM_SETCHECK, e.drawTrail ? BST_CHECKED : BST_UNCHECKED, 0);
        }
    }
}

void UIManager::sync(const Scene& scene, const ViewSettings& view, const SimulationSettings& sim, ObjectRef selected, ToolMode mode) {
    synced_ = selected;
    for (int i = 0; i < 9; ++i) { setVisible(labels_[i], true); setVisible(edits_[i], true); }
    hideAllOptional();

    if (!valid(selected) && (mode == ToolMode::AddElectricField || mode == ToolMode::AddMagneticField)) {
        bool magnetic = mode == ToolMode::AddMagneticField;
        SetWindowTextA(title_, magnetic ? "New Magnetic Field" : "New Electric Field");
        setVisible(labels_[0], false); setVisible(edits_[0], false);
        setVisible(labels_[1], false); setVisible(edits_[1], false);
        setLabel(2, "Rotate"); setEdit(2, "0");
        setLabel(3, "Width"); setEdit(3, "4");
        setLabel(4, "Height"); setEdit(4, "2.4");
        setLabel(5, magnetic ? "B f(t)" : "E f(t)"); setEdit(5, magnetic ? "1.5" : "4");
        setLabel(6, "Direction"); setVisible(labels_[6], magnetic); setVisible(edits_[6], false); setVisible(direction_, magnetic);
        setLabel(7, "Base"); setEdit(7, "2.4"); setVisible(labels_[7], false); setVisible(edits_[7], false);
        setLabel(8, "Shape"); setVisible(edits_[8], false); setVisible(shapeCombo_, true);
        int shapeIdx = (int)SendMessageA(shapeCombo_, CB_GETCURSEL, 0, 0);
        if (shapeIdx < 0 || shapeIdx > 3) { shapeIdx = 0; SendMessageA(shapeCombo_, CB_SETCURSEL, 0, 0); }
        if (shapeIdx == (int)FieldShape::Triangle) {
            setEdit(3, "60");
            setEdit(4, "60");
            setEdit(7, "2.4");
        }
        applyShapeLabels(shapeIdx);
        setStatus(magnetic ? "Choose magnetic shape/params, then click canvas." : "Choose electric shape/params, then click canvas.");
        return;
    }

    SetWindowTextA(title_, valid(selected) ? objectName(selected.kind) : "World Settings");
    setVisible(deleteObj_, valid(selected));

    if (!valid(selected)) {
        setLabel(0, "World W"); setEdit(0, fmt(view.worldWidth).c_str());
        setLabel(1, "World H"); setEdit(1, fmt(view.worldHeight).c_str());
        setLabel(2, "Ticks X"); setEdit(2, std::to_string(view.tickX).c_str());
        setLabel(3, "Ticks Y"); setEdit(3, std::to_string(view.tickY).c_str());
        setLabel(4, "Unit"); setEdit(4, view.unit.c_str());
        setLabel(5, "Duration"); setEdit(5, fmt(sim.duration).c_str());
        for (int i = 6; i < 9; ++i) { setVisible(labels_[i], false); setVisible(edits_[i], false); }
    } else {
        Vec2 p; double r = 0, w = 1, h = 1;
        scene.getCommon(selected, p, r, &w, &h);
        setLabel(0, "X"); setEdit(0, fmt(p.x).c_str());
        setLabel(1, "Y"); setEdit(1, fmt(p.y).c_str());
        setLabel(2, "Rotate"); setEdit(2, fmt(radToDeg(r)).c_str());
        setLabel(3, "Width"); setEdit(3, fmt(w).c_str());
        setLabel(4, "Height"); setEdit(4, fmt(h).c_str());
        setLabel(5, "f(t)"); setEdit(5, "");
        setLabel(6, "Mass"); setEdit(6, "1");
        setLabel(7, "Charge"); setEdit(7, "1");
        setLabel(8, "Particle");

        if (selected.kind == ObjectKind::ElectricField) {
            int shapeIdx = 0;
            for (const auto& o : scene.electricFields) if (o.id == selected.id) {
                setEdit(5, o.strength.source.c_str());
                shapeIdx = (int)o.shape;
                SendMessageA(shapeCombo_, CB_SETCURSEL, shapeIdx, 0);
                if (o.shape == FieldShape::Circle) {
                    setEdit(3, fmt(o.radius).c_str());
                } else if (o.shape == FieldShape::Ring) {
                    setEdit(3, fmt(o.radius).c_str());
                    setEdit(4, fmt(o.innerRadius).c_str());
                } else if (o.shape == FieldShape::Triangle) {
                    setEdit(3, fmt(o.triangleLeftDeg).c_str());
                    setEdit(4, fmt(o.triangleRightDeg).c_str());
                    setEdit(7, fmt(o.triangleBase).c_str());
                }
            }
            setLabel(5, "E f(t)");
            setVisible(labels_[6], false); setVisible(edits_[6], false);
            setVisible(direction_, false);
            setVisible(labels_[7], false); setVisible(edits_[7], false);
            setLabel(8, "Shape");
            setVisible(edits_[8], false);
            setVisible(shapeCombo_, true);
            applyShapeLabels(shapeIdx);
        } else if (selected.kind == ObjectKind::MagneticField) {
            int shapeIdx = 0;
            for (const auto& o : scene.magneticFields) if (o.id == selected.id) {
                setEdit(5, o.strength.source.c_str());
                SendMessageA(direction_, CB_SETCURSEL, o.intoPage ? 1 : 0, 0);
                shapeIdx = (int)o.shape;
                SendMessageA(shapeCombo_, CB_SETCURSEL, shapeIdx, 0);
                if (o.shape == FieldShape::Circle) {
                    setEdit(3, fmt(o.radius).c_str());
                } else if (o.shape == FieldShape::Ring) {
                    setEdit(3, fmt(o.radius).c_str());
                    setEdit(4, fmt(o.innerRadius).c_str());
                } else if (o.shape == FieldShape::Triangle) {
                    setEdit(3, fmt(o.triangleLeftDeg).c_str());
                    setEdit(4, fmt(o.triangleRightDeg).c_str());
                    setEdit(7, fmt(o.triangleBase).c_str());
                }
            }
            setLabel(5, "B f(t)");
            setLabel(6, "Direction");
            setVisible(edits_[6], false);
            setVisible(direction_, true);
            setVisible(labels_[7], false); setVisible(edits_[7], false);
            setLabel(8, "Shape");
            setVisible(edits_[8], false);
            setVisible(shapeCombo_, true);
            applyShapeLabels(shapeIdx);
        } else if (selected.kind == ObjectKind::Plane) {
            bool enabled = true;
            for (const auto& o : scene.planes) if (o.id == selected.id) enabled = o.collisionEnabled;
            SendMessageA(collision_, BM_SETCHECK, enabled ? BST_CHECKED : BST_UNCHECKED, 0);
            setVisible(collision_, true);
            for (int i = 5; i < 9; ++i) setVisible(edits_[i], false), setVisible(labels_[i], false);
        } else if (selected.kind == ObjectKind::Emitter) {
            setVisible(labels_[3], false); setVisible(edits_[3], false);
            setVisible(labels_[4], false); setVisible(edits_[4], false);
            setLabel(5, "Emit t"); setEdit(5, "0");
            setLabel(6, "Mass"); setEdit(6, "1");
            setLabel(7, "Charge"); setEdit(7, "1");
            setVisible(labels_[8], true); setVisible(edits_[8], false);
            setVisible(combo_, true);
            setVisible(addEntry_, true); setVisible(delEntry_, true); setVisible(list_, true);
            for (const auto& e : scene.emitters) if (e.id == selected.id) fillEmitterList(e);
        }
    }

    const char* tool = "Select";
    if (mode == ToolMode::AddElectricField) tool = "Add electric field: click canvas";
    if (mode == ToolMode::AddMagneticField) tool = "Add magnetic field: set params, click canvas";
    if (mode == ToolMode::AddPlane) tool = "Add plane: click canvas";
    if (mode == ToolMode::AddEmitter) tool = "Add emitter: click canvas";
    setStatus(tool);
}

bool UIManager::apply(Scene& scene, ViewSettings& view, SimulationSettings& sim, ObjectRef selected) {
    if (!valid(selected)) {
        view.worldWidth = std::max(1.0, getDouble(0, view.worldWidth));
        view.worldHeight = std::max(1.0, getDouble(1, view.worldHeight));
        view.tickX = std::max(1, getInt(2, view.tickX));
        view.tickY = std::max(1, getInt(3, view.tickY));
        view.unit = getEdit(4).empty() ? "m" : getEdit(4);
        sim.duration = std::max(0.1, getDouble(5, sim.duration));
        if (sim.time > sim.duration) sim.time = sim.duration;
        return true;
    }

    Vec2 p; double rot = 0, w = 1, h = 1;
    scene.getCommon(selected, p, rot, &w, &h);
    p.x = getDouble(0, p.x);
    p.y = getDouble(1, p.y);
    rot = degToRad(getDouble(2, radToDeg(rot)));

    if (selected.kind == ObjectKind::Emitter) {
        scene.setCommon(selected, p, rot, nullptr, nullptr);
        return true;
    }

    if (selected.kind == ObjectKind::ElectricField) {
        int shapeIdx = (int)SendMessageA(shapeCombo_, CB_GETCURSEL, 0, 0);
        if (shapeIdx < 0 || shapeIdx > 3) shapeIdx = 0;
        FieldShape shape = (FieldShape)shapeIdx;
        double radius = 1.6;
        double inner = 0.7;
        for (const auto& o : scene.electricFields) if (o.id == selected.id) {
            radius = o.radius;
            inner = o.innerRadius;
        }
        if (shape == FieldShape::Circle || shape == FieldShape::Ring) {
            radius = std::max(0.05, getDouble(3, radius));
            inner = shape == FieldShape::Ring ? std::max(0.01, getDouble(4, inner)) : 0.0;
            inner = std::min(inner, radius * 0.95);
            w = h = radius * 2.0;
        } else if (shape == FieldShape::Triangle) {
            w = std::max(0.05, w);
            h = std::max(0.05, h);
        } else {
            w = getDouble(3, w);
            h = getDouble(4, h);
            radius = std::max(w, h) * 0.5;
            inner = std::min(inner, radius * 0.95);
        }
        scene.setCommon(selected, p, rot, &w, &h);
        for (auto& o : scene.electricFields) if (o.id == selected.id) {
            Function1D f = o.strength;
            if (f.parseFromText(getEdit(5))) o.strength = f;
            o.shape = shape;
            o.radius = radius;
            o.innerRadius = inner;
            if (shape == FieldShape::Triangle) {
                o.triangleLeftDeg = clampd(getDouble(3, o.triangleLeftDeg), 5.0, 85.0);
                o.triangleRightDeg = clampd(getDouble(4, o.triangleRightDeg), 5.0, 85.0);
                o.triangleBase = std::max(0.05, getDouble(7, o.triangleBase));
                updateElectricTriangleBounds(o);
            }
        }
        return true;
    }

    if (selected.kind == ObjectKind::MagneticField) {
        int shapeIdx = (int)SendMessageA(shapeCombo_, CB_GETCURSEL, 0, 0);
        if (shapeIdx < 0 || shapeIdx > 3) shapeIdx = 0;
        FieldShape shape = (FieldShape)shapeIdx;
        double radius = 1.6;
        double inner = 0.7;
        for (const auto& o : scene.magneticFields) if (o.id == selected.id) {
            radius = o.radius;
            inner = o.innerRadius;
        }
        if (shape == FieldShape::Circle || shape == FieldShape::Ring) {
            radius = std::max(0.05, getDouble(3, radius));
            inner = shape == FieldShape::Ring ? std::max(0.01, getDouble(4, inner)) : 0.0;
            inner = std::min(inner, radius * 0.95);
            w = h = radius * 2.0;
        } else if (shape == FieldShape::Triangle) {
            w = std::max(0.05, w);
            h = std::max(0.05, h);
        } else {
            w = getDouble(3, w);
            h = getDouble(4, h);
            radius = std::max(w, h) * 0.5;
            inner = std::min(inner, radius * 0.95);
        }
        scene.setCommon(selected, p, rot, &w, &h);
        for (auto& o : scene.magneticFields) if (o.id == selected.id) {
            Function1D f = o.strength;
            if (f.parseFromText(getEdit(5))) o.strength = f;
            o.intoPage = SendMessageA(direction_, CB_GETCURSEL, 0, 0) == 1;
            o.shape = shape;
            o.radius = radius;
            o.innerRadius = inner;
            if (shape == FieldShape::Triangle) {
                o.triangleLeftDeg = clampd(getDouble(3, o.triangleLeftDeg), 5.0, 85.0);
                o.triangleRightDeg = clampd(getDouble(4, o.triangleRightDeg), 5.0, 85.0);
                o.triangleBase = std::max(0.05, getDouble(7, o.triangleBase));
                updateMagneticTriangleBounds(o);
            }
        }
        return true;
    }

    w = getDouble(3, w);
    h = getDouble(4, h);
    scene.setCommon(selected, p, rot, &w, &h);

    if (selected.kind == ObjectKind::Plane) {
        for (auto& o : scene.planes) if (o.id == selected.id) o.collisionEnabled = SendMessageA(collision_, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }
    return true;
}

ObjectRef UIManager::createElectric(Scene& scene, Vec2 worldPos) {
    ObjectRef ref = scene.addElectricField(worldPos);
    int shapeIdx = (int)SendMessageA(shapeCombo_, CB_GETCURSEL, 0, 0);
    if (shapeIdx < 0 || shapeIdx > 3) shapeIdx = 0;
    FieldShape shape = (FieldShape)shapeIdx;
    double rot = degToRad(getDouble(2, 0.0));
    double w = 4.0;
    double h = 2.4;
    double radius = 1.6;
    double inner = 0.7;
    if (shape == FieldShape::Circle || shape == FieldShape::Ring) {
        radius = std::max(0.05, getDouble(3, radius));
        inner = shape == FieldShape::Ring ? std::max(0.01, getDouble(4, inner)) : 0.0;
        inner = std::min(inner, radius * 0.95);
        w = h = radius * 2.0;
    } else if (shape == FieldShape::Triangle) {
        w = 2.4;
        h = 2.4;
    } else {
        w = std::max(0.05, getDouble(3, w));
        h = std::max(0.05, getDouble(4, h));
        radius = std::max(w, h) * 0.5;
    }
    scene.setCommon(ref, worldPos, rot, &w, &h);
    for (auto& o : scene.electricFields) if (o.id == ref.id) {
        Function1D f = o.strength;
        if (f.parseFromText(getEdit(5))) o.strength = f;
        o.shape = shape;
        o.radius = radius;
        o.innerRadius = inner;
        if (shape == FieldShape::Triangle) {
            o.triangleLeftDeg = clampd(getDouble(3, o.triangleLeftDeg), 5.0, 85.0);
            o.triangleRightDeg = clampd(getDouble(4, o.triangleRightDeg), 5.0, 85.0);
            o.triangleBase = std::max(0.05, getDouble(7, o.triangleBase));
            updateElectricTriangleBounds(o);
        }
    }
    return ref;
}

ObjectRef UIManager::createMagnetic(Scene& scene, Vec2 worldPos) {
    ObjectRef ref = scene.addMagneticField(worldPos);
    int shapeIdx = (int)SendMessageA(shapeCombo_, CB_GETCURSEL, 0, 0);
    if (shapeIdx < 0 || shapeIdx > 3) shapeIdx = 0;
    FieldShape shape = (FieldShape)shapeIdx;
    double rot = degToRad(getDouble(2, 0.0));
    double w = 4.0;
    double h = 2.4;
    double radius = 1.6;
    double inner = 0.7;
    if (shape == FieldShape::Circle || shape == FieldShape::Ring) {
        radius = std::max(0.05, getDouble(3, radius));
        inner = shape == FieldShape::Ring ? std::max(0.01, getDouble(4, inner)) : 0.0;
        inner = std::min(inner, radius * 0.95);
        w = h = radius * 2.0;
    } else if (shape == FieldShape::Triangle) {
        w = 2.4;
        h = 2.4;
    } else {
        w = std::max(0.05, getDouble(3, w));
        h = std::max(0.05, getDouble(4, h));
        radius = std::max(w, h) * 0.5;
    }
    scene.setCommon(ref, worldPos, rot, &w, &h);
    for (auto& o : scene.magneticFields) if (o.id == ref.id) {
        Function1D f = o.strength;
        if (f.parseFromText(getEdit(5))) o.strength = f;
        o.intoPage = SendMessageA(direction_, CB_GETCURSEL, 0, 0) == 1;
        o.shape = shape;
        o.radius = radius;
        o.innerRadius = inner;
        if (shape == FieldShape::Triangle) {
            o.triangleLeftDeg = clampd(getDouble(3, o.triangleLeftDeg), 5.0, 85.0);
            o.triangleRightDeg = clampd(getDouble(4, o.triangleRightDeg), 5.0, 85.0);
            o.triangleBase = std::max(0.05, getDouble(7, o.triangleBase));
            updateMagneticTriangleBounds(o);
        }
    }
    return ref;
}

void UIManager::updateParticleDefaults() {
    int idx = (int)SendMessageA(combo_, CB_GETCURSEL, 0, 0);
    if (idx < 0 || idx > 3) return;
    ParticleKind kind = (ParticleKind)idx;
    if (kind == ParticleKind::Custom) return;
    ParticleDef def = builtInParticle(kind);
    setEdit(6, fmt(def.mass).c_str());
    setEdit(7, fmt(def.charge).c_str());
}

bool UIManager::addEmissionEntry(Scene& scene, ObjectRef selected) {
    if (selected.kind != ObjectKind::Emitter) return false;
    for (auto& emitter : scene.emitters) if (emitter.id == selected.id) {
        int idx = (int)SendMessageA(combo_, CB_GETCURSEL, 0, 0);
        if (idx < 0) idx = 1;
        EmissionEntry e;
        e.kind = (ParticleKind)idx;
        e.time = std::max(0.0, getDouble(5, 0.0));
        e.speed = 3.5;
        e.custom = builtInParticle(ParticleKind::Custom);
        e.custom.mass = std::max(0.001, getDouble(6, 1.0));
        e.custom.charge = getDouble(7, 1.0);
        e.custom.radius = 0.12;
        if (e.kind == ParticleKind::Custom) e.custom.color = randomParticleColor();
        emitter.schedule.push_back(e);
        fillEmitterList(emitter);
        return true;
    }
    return false;
}

bool UIManager::deleteEmissionEntry(Scene& scene, ObjectRef selected) {
    if (selected.kind != ObjectKind::Emitter) return false;
    int sel = (int)SendMessageA(list_, LB_GETCURSEL, 0, 0);
    if (sel < 0) return false;
    for (auto& emitter : scene.emitters) if (emitter.id == selected.id && sel < (int)emitter.schedule.size()) {
        emitter.schedule.erase(emitter.schedule.begin() + sel);
        fillEmitterList(emitter);
        return true;
    }
    return false;
}

bool UIManager::updateEmissionTrail(Scene& scene, ObjectRef selected, int slot) {
    if (selected.kind != ObjectKind::Emitter || slot < 0 || slot >= MAX_TRAIL_CHECKS) return false;
    for (auto& emitter : scene.emitters) if (emitter.id == selected.id && slot < (int)emitter.schedule.size()) {
        emitter.schedule[slot].drawTrail = SendMessageA(trailChecks_[slot], BM_GETCHECK, 0, 0) == BST_CHECKED;
        return true;
    }
    return false;
}

void UIManager::setStatus(const char* text) {
    SetWindowTextA(status_, text ? text : "");
}
