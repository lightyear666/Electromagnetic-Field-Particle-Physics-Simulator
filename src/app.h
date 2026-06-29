#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "physics.h"
#include "renderer.h"
#include "ui.h"

class App {
public:
    bool init(HWND hwnd, HINSTANCE inst);
    void shutdown();
    LRESULT handle(UINT msg, WPARAM wp, LPARAM lp);

private:
    HWND hwnd_ = nullptr;
    Renderer renderer_;
    UIManager ui_;
    Scene scene_;
    ViewSettings view_;
    SimulationSettings sim_;
    PhysicsWorld physics_;
    ToolMode toolMode_ = ToolMode::Select;
    ObjectRef selected_{};
    bool draggingObject_ = false;
    bool draggingTimeline_ = false;
    bool draggingView_ = false;
    Vec2 dragOffset_{};
    int lastMouseX_ = 0;
    int lastMouseY_ = 0;
    int clientW_ = 1200;
    int clientH_ = 780;
    DWORD lastTick_ = 0;

    static constexpr int LEFT_PANEL_W = 280;
    static constexpr int TIMELINE_H = 70;

    void layout();
    void render();
    void invalidate();
    void refreshNow();
    void advance();
    void resetSimulation();
    void seekTo(double target);
    void select(ObjectRef ref);
    void setTool(ToolMode mode);
    void onCommand(int id, int code);
    void onLeftDown(int x, int y);
    void onMouseMove(int x, int y, WPARAM flags);
    void onLeftUp();
    void onMouseWheel(int x, int y, int delta);
    bool saveScene();
    bool loadScene();
};
