// main_win.cpp - Win32 + Direct3D 9 host for the ImGui UI (structure follows the official
// Dear ImGui example_win32_directx9). Event driven: the loop sleeps in MsgWaitForMultipleObjects,
// so an idle window uses ~0% CPU, and redraws slow to 1 fps while the drive buffer is low.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <d3d9.h>
#include <string>

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "App.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LPDIRECT3D9             g_d3d = NULL;
static LPDIRECT3DDEVICE9       g_dev = NULL;
static D3DPRESENT_PARAMETERS   g_pp = {};
static LPDIRECT3DTEXTURE9      g_previewTex = NULL;
static dis::App*               g_app = NULL;

static bool CreateDevice(HWND hwnd) {
    g_d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!g_d3d) return false;
    ZeroMemory(&g_pp, sizeof g_pp);
    g_pp.Windowed = TRUE;
    g_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_pp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_pp.EnableAutoDepthStencil = TRUE;
    g_pp.AutoDepthStencilFormat = D3DFMT_D16;
    g_pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;      // vsync keeps the GPU idle
    if (g_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_pp, &g_dev) == D3D_OK)
        return true;
    // very old integrated GPUs have no hardware vertex processing
    return g_d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_pp, &g_dev) == D3D_OK;
}

static void ResetDevice() {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    if (g_dev->Reset(&g_pp) == D3DERR_INVALIDCALL) return;
    ImGui_ImplDX9_CreateDeviceObjects();
}

static void UpdatePreviewTexture() {
    if (!g_dev || !g_app->takePreviewDirty()) return;
    if (!g_previewTex) {
        if (g_dev->CreateTexture(dis::App::kPreview, dis::App::kPreview, 1, 0, D3DFMT_A8R8G8B8,
                                 D3DPOOL_MANAGED, &g_previewTex, NULL) != D3D_OK) { g_previewTex = NULL; return; }
        g_app->setPreviewTexture(g_previewTex);
    }
    D3DLOCKED_RECT lr;
    if (g_previewTex->LockRect(0, &lr, NULL, 0) == D3D_OK) {
        const uint32_t* src = g_app->previewPixels();
        for (int y = 0; y < dis::App::kPreview; ++y)
            memcpy((BYTE*)lr.pBits + (size_t)y * lr.Pitch, src + y * dis::App::kPreview, dis::App::kPreview * 4);
        g_previewTex->UnlockRect(0);
    }
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_CLOSE:
        if (g_app && g_app->busyRealBurn()) {
            MessageBoxW(hWnd, L"A burn is in progress. Cancel it first (this ruins the disc), then close.",
                        L"DiscImageStudio", MB_OK | MB_ICONWARNING);
            return 0;
        }
        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0;      // disable ALT application menu
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static std::string ConfigPath() {
    wchar_t w[MAX_PATH * 2];
    DWORD n = GetModuleFileNameW(NULL, w, (DWORD)(sizeof w / sizeof w[0]));
    std::wstring p(w, n);
    size_t slash = p.find_last_of(L"\\/");
    p = (slash == std::wstring::npos ? std::wstring() : p.substr(0, slash + 1)) + L"config.ini";
    int len = WideCharToMultiByte(CP_UTF8, 0, p.c_str(), -1, NULL, 0, NULL, NULL);
    std::string out((size_t)len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, p.c_str(), -1, &out[0], len, NULL, NULL);
    out.resize((size_t)len - 1);
    return out;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    WNDCLASSEXW wc = { sizeof wc, CS_CLASSDC, WndProc, 0, 0, hInst, NULL, LoadCursor(NULL, IDC_ARROW),
                       NULL, NULL, L"DiscImageStudioWnd", NULL };
    RegisterClassExW(&wc);
    RECT rc = { 0, 0, 640, 700 };
    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;   // fixed size
    AdjustWindowRect(&rc, style, FALSE);
    HWND hwnd = CreateWindowW(wc.lpszClassName, L"DiscImageStudio", style, 100, 40,
                              rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInst, NULL);
    if (!CreateDevice(hwnd)) {
        MessageBoxW(hwnd, L"Could not create a Direct3D 9 device.", L"DiscImageStudio", MB_OK | MB_ICONERROR);
        UnregisterClassW(wc.lpszClassName, hInst);
        return 1;
    }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;                       // no imgui.ini
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_dev);

    dis::App app;
    g_app = &app;
    app.init(ConfigPath(), hwnd);

    bool running = true;
    while (running) {
        DWORD timeout = app.pressure() ? 1000 : (app.busy() ? 250 : 100);
        MsgWaitForMultipleObjects(0, NULL, FALSE, timeout, QS_ALLINPUT);
        MSG msg;
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;
        if (IsIconic(hwnd)) continue;

        UpdatePreviewTexture();
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        RECT cr; GetClientRect(hwnd, &cr);
        app.drawUi((float)(cr.right - cr.left), (float)(cr.bottom - cr.top));
        ImGui::EndFrame();

        g_dev->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_dev->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        g_dev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(30, 30, 34, 255), 1.0f, 0);
        if (g_dev->BeginScene() >= 0) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_dev->EndScene();
        } else {
            ImGui::Render();
        }
        HRESULT hr = g_dev->Present(NULL, NULL, NULL, NULL);
        if (hr == D3DERR_DEVICELOST && g_dev->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) ResetDevice();
    }

    g_app = NULL;
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (g_previewTex) g_previewTex->Release();
    if (g_dev) g_dev->Release();
    if (g_d3d) g_d3d->Release();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, hInst);
    return 0;
}
