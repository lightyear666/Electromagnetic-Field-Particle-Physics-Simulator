#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include "app.h"

static App* g_app = nullptr;
static bool g_appReady = false;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lp);
        g_app = reinterpret_cast<App*>(cs->lpCreateParams);
        return DefWindowProc(hwnd, msg, wp, lp);
    }

    if (msg == WM_CREATE) {
        auto cs = reinterpret_cast<CREATESTRUCT*>(lp);
        auto app = reinterpret_cast<App*>(cs->lpCreateParams);
        if (!app->init(hwnd, reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hwnd, GWLP_HINSTANCE)))) {
            MessageBoxA(hwnd, "OpenGL initialization failed.", "Physics Simulator", MB_ICONERROR | MB_OK);
            return -1;
        }
        g_appReady = true;
        return 0;
    }

    if (g_appReady && g_app) {
        if (msg == WM_DESTROY) {
            g_app->shutdown();
            g_appReady = false;
            PostQuitMessage(0);
            return 0;
        }
        return g_app->handle(msg, wp, lp);
    }

    return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int show) {
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    WNDCLASSEXA wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = "FieldPhysicsSimulatorWindow";

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(nullptr, "RegisterClassEx failed.", "Physics Simulator", MB_ICONERROR | MB_OK);
        return 1;
    }

    App app;
    HWND hwnd = CreateWindowExA(
        0,
        wc.lpszClassName,
        "OpenGL 2D Electromagnetic Physics Simulator",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1280,
        820,
        nullptr,
        nullptr,
        inst,
        &app);

    if (!hwnd) {
        MessageBoxA(nullptr, "CreateWindowEx failed.", "Physics Simulator", MB_ICONERROR | MB_OK);
        return 1;
    }

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}
