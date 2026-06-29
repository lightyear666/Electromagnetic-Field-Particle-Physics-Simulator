#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "scene.h"

constexpr int IDC_TOOL_SELECT = 1001;
constexpr int IDC_TOOL_EFIELD = 1002;
constexpr int IDC_TOOL_BFIELD = 1003;
constexpr int IDC_TOOL_PLANE = 1004;
constexpr int IDC_TOOL_EMITTER = 1005;
constexpr int IDC_APPLY = 1010;
constexpr int IDC_ADD_ENTRY = 1011;
constexpr int IDC_DEL_ENTRY = 1012;
constexpr int IDC_PARTICLE_KIND = 1013;
constexpr int IDC_ENTRY_LIST = 1014;
constexpr int IDC_MAG_DIRECTION = 1015;
constexpr int IDC_SHAPE_KIND = 1016;
constexpr int IDC_DELETE_OBJ = 1017;
constexpr int IDC_SAVE_SCENE = 1018;
constexpr int IDC_LOAD_SCENE = 1019;
constexpr int IDC_TRAIL_BASE = 1100;
constexpr int MAX_TRAIL_CHECKS = 16;

enum class ToolMode { Select, AddElectricField, AddMagneticField, AddPlane, AddEmitter };

class UIManager {
public:
    void create(HWND parent, HINSTANCE inst);
    void layout(int width, int height, int leftPanelW, int timelineH);
    void sync(const Scene& scene, const ViewSettings& view, const SimulationSettings& sim, ObjectRef selected, ToolMode mode);
    bool apply(Scene& scene, ViewSettings& view, SimulationSettings& sim, ObjectRef selected);
    bool addEmissionEntry(Scene& scene, ObjectRef selected);
    bool deleteEmissionEntry(Scene& scene, ObjectRef selected);
    bool updateEmissionTrail(Scene& scene, ObjectRef selected, int slot);
    ObjectRef createElectric(Scene& scene, Vec2 worldPos);
    ObjectRef createMagnetic(Scene& scene, Vec2 worldPos);
    void onShapeChanged();
    void setStatus(const char* text);
    void updateParticleDefaults();

private:
    HWND parent_ = nullptr;
    HWND title_ = nullptr;
    HWND status_ = nullptr;
    HWND save_ = nullptr;
    HWND load_ = nullptr;
    HWND toolSelect_ = nullptr;
    HWND toolE_ = nullptr;
    HWND toolB_ = nullptr;
    HWND toolPlane_ = nullptr;
    HWND toolEmitter_ = nullptr;
    HWND labels_[9]{};
    HWND edits_[9]{};
    HWND apply_ = nullptr;
    HWND deleteObj_ = nullptr;
    HWND collision_ = nullptr;
    HWND combo_ = nullptr;
    HWND direction_ = nullptr;
    HWND shapeCombo_ = nullptr;
    HWND addEntry_ = nullptr;
    HWND delEntry_ = nullptr;
    HWND list_ = nullptr;
    HWND trailChecks_[MAX_TRAIL_CHECKS]{};
    int listX_ = 0;
    int listY_ = 0;
    int listH_ = 0;
    ObjectRef synced_{};

    HWND makeLabel(const char* text);
    HWND makeEdit(const char* text);
    HWND makeButton(const char* text, int id);
    void setVisible(HWND hwnd, bool visible);
    void setLabel(int index, const char* text);
    void setEdit(int index, const char* text);
    std::string getEdit(int index) const;
    double getDouble(int index, double fallback) const;
    int getInt(int index, int fallback) const;
    void fillEmitterList(const ParticleEmitter& emitter);
    void applyShapeLabels(int shapeIdx);
    void hideAllOptional();
};
