#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include "scene.h"

struct ScreenRect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

class Renderer {
public:
    bool init(HWND hwnd);
    void shutdown();
    void render(const Scene& scene, const ViewSettings& view, const SimulationSettings& sim, ObjectRef selected, int clientW, int clientH, int leftPanelW, int timelineH);
    Vec2 screenToWorld(int sx, int sy, const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH) const;
    Vec2 worldUnitsPerPixel(const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH) const;
    bool inDrawArea(int sx, int sy, int clientW, int clientH, int leftPanelW, int timelineH) const;
    double timelineTimeFromMouse(int sx, const SimulationSettings& sim, int clientW, int leftPanelW) const;

private:
    HWND hwnd_ = nullptr;
    HDC hdc_ = nullptr;
    HGLRC hglrc_ = nullptr;
    GLuint fontBase_ = 0;

    ScreenRect drawViewport(const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH) const;
    void setupWorld(const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH);
    void setupScreen(int clientW, int clientH);
    void drawGrid(const ViewSettings& view, int clientW, int clientH, int leftPanelW, int timelineH);
    void drawScene(const Scene& scene, const SimulationSettings& sim, ObjectRef selected);
    void drawTimeline(const SimulationSettings& sim, int clientW, int clientH, int leftPanelW, int timelineH);
    void drawText(double x, double y, const char* text);
};
