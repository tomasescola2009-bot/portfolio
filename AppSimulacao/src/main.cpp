// Minimal DirectX11 + ImGui overlay that reads coords from a named mapping and draws a circle.

#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <tchar.h>
#include <stdint.h>
#include <stdio.h>
#include <iostream>

#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include <fstream>
#include <sstream>

const wchar_t* MAPPING_NAME = L"Local\\OverlayCoords";
const char* SETTINGS_FILE = "settings.ini";

struct Coords { float x; float y; };

struct Settings {
    float circleX = 400.0f;
    float circleY = 300.0f;
    float circleRadius = 20.0f;
    uint32_t circleColor = IM_COL32(255, 0, 0, 200);
};

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;
static bool g_menuMode = false;
static HWND g_hWnd = NULL;

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
    return true;
}

void CleanupDeviceD3D()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    // Toggle menu mode with INSERT
    if (msg == WM_KEYDOWN && wParam == VK_INSERT)
    {
        g_menuMode = !g_menuMode;
        // change extended window styles
        LONG_PTR ex = GetWindowLongPtr(hWnd, GWL_EXSTYLE);
        if (g_menuMode)
        {
            ex &= ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
            SetWindowLongPtr(hWnd, GWL_EXSTYLE, ex);
            // bring to foreground so user can interact
            SetForegroundWindow(hWnd);
        }
        else
        {
            ex |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
            SetWindowLongPtr(hWnd, GWL_EXSTYLE, ex);
            // ensure topmost
            SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
        return 0;
    }
    if (msg == WM_SIZE && g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
    {
        CleanupRenderTarget();
        g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget();
        return 0;
    }
    if (msg == WM_DESTROY) PostQuitMessage(0);
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool ReadCoordsFromMapping(Coords &out)
{
    HANDLE hMap = OpenFileMappingW(FILE_MAP_READ, FALSE, MAPPING_NAME);
    if (!hMap) return false;
    void* p = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, sizeof(Coords));
    if (!p) { CloseHandle(hMap); return false; }
    Coords tmp;
    memcpy(&tmp, p, sizeof(Coords));
    out = tmp;
    UnmapViewOfFile(p);
    CloseHandle(hMap);
    return true;
}

bool WriteCoordsToMapping(const Coords &in)
{
    HANDLE hMap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Coords), MAPPING_NAME);
    if (!hMap) return false;
    void* p = MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, sizeof(Coords));
    if (!p) { CloseHandle(hMap); return false; }
    memcpy(p, &in, sizeof(Coords));
    UnmapViewOfFile(p);
    CloseHandle(hMap);
    return true;
}

bool LoadSettings(Settings &out)
{
    std::ifstream file(SETTINGS_FILE);
    if (!file.is_open()) return false;
    std::string line;
    while (std::getline(file, line))
    {
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;
        std::string key = line.substr(0, eqPos);
        std::string val = line.substr(eqPos + 1);
        if (key == "circleX") out.circleX = std::stof(val);
        else if (key == "circleY") out.circleY = std::stof(val);
        else if (key == "circleRadius") out.circleRadius = std::stof(val);
        else if (key == "circleColor") out.circleColor = (uint32_t)std::stoul(val);
    }
    file.close();
    return true;
}

bool SaveSettings(const Settings &in)
{
    std::ofstream file(SETTINGS_FILE);
    if (!file.is_open()) return false;
    file << "circleX=" << in.circleX << "\n";
    file << "circleY=" << in.circleY << "\n";
    file << "circleRadius=" << in.circleRadius << "\n";
    file << "circleColor=" << in.circleColor << "\n";
    file.close();
    return true;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    // Register window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("OverlayImGuiClass"), NULL };
    RegisterClassEx(&wc);

    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);

    HWND hWnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        wc.lpszClassName, _T("OverlayImGui"), WS_POPUP, 0, 0, sx, sy, NULL, NULL, wc.hInstance, NULL);

    ShowWindow(hWnd, SW_SHOWNOACTIVATE);
    UpdateWindow(hWnd);
    g_hWnd = hWnd;

    // Initialize Direct3D
    if (!CreateDeviceD3D(hWnd)) {
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load settings
    Settings settings;
    LoadSettings(settings);
    Coords coords{ settings.circleX, settings.circleY };

    // Main loop
    bool running = true;
    MSG msg;
    while (running)
    {
        while (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }

        // Read coords
        Coords tmp;
        if (ReadCoordsFromMapping(tmp)) coords = tmp;

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Menu UI when in menu mode (visual indicator + interactive menu)
        if (g_menuMode)
        {
            // Draw a semi-transparent green border around the screen to indicate Menu mode
            ImDrawList* bg = ImGui::GetBackgroundDrawList();
            bg->AddRect(ImVec2(0, 0), ImVec2((float)sx, (float)sy), IM_COL32(0, 255, 0, 100), 0.0f, 0, 5.0f);

            // Small status panel
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.6f);
            ImGui::Begin("Overlay Status", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "[MODO MENU ATIVO - INSERT para fechar]");
            ImGui::End();

            ImGui::SetNextWindowBgAlpha(0.6f);
            ImGui::Begin("Overlay Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::Text("Mode: Menu (interactive)");
            ImGui::Text("Press Insert to toggle overlay/menu");
            ImGui::Separator();
            ImGui::Text("Circle Settings:");
            if (ImGui::SliderFloat("Radius", &settings.circleRadius, 5.0f, 100.0f))
                SaveSettings(settings);
            ImVec4 colorVec4 = ImGui::ColorConvertU32ToFloat4(settings.circleColor);
            if (ImGui::ColorEdit4("Circle Color", (float*)&colorVec4, ImGuiColorEditFlags_AlphaBar))
            {
                settings.circleColor = ImGui::ColorConvertFloat4ToU32(colorVec4);
                SaveSettings(settings);
            }
            ImGui::Separator();
            ImGui::Text("Instructions:");
            ImGui::BulletText("Click or drag anywhere to move the circle.");
            if (ImGui::Button("Close Overlay")) { PostQuitMessage(0); running = false; }
            ImGui::End();

            // If user clicks/drags and ImGui is not capturing the mouse for widgets
            ImGuiIO& io2 = ImGui::GetIO();
            if (io2.MouseDown[0] && !ImGui::IsAnyItemActive() && !ImGui::IsPopupOpen(nullptr))
            {
                coords.x = io2.MousePos.x;
                coords.y = io2.MousePos.y;
                settings.circleX = coords.x;
                settings.circleY = coords.y;
                WriteCoordsToMapping(coords);
                SaveSettings(settings);
            }
        }

        // Draw a circle on the foreground draw list (always drawn)
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->AddCircleFilled(ImVec2(coords.x, coords.y), settings.circleRadius, settings.circleColor);

        ImGui::Render();

        // Render
        const float clear_color_with_alpha[4] = { 0.f, 0.f, 0.f, 0.f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);

        Sleep(16);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
