#include <iostream>
#include "GraphicHook.h"
#include <d3d9.h>
#include <detours.h>
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "offsets.h"

extern bool bAimbot, bGlowHack, bNoRecoil, bTriggerBot;   ////////////////////////////////////////////////////

using endScene = HRESULT (__stdcall*)(IDirect3DDevice9* pDevice);
endScene originalEndScene = nullptr; //An original endscene which is null now.

bool g_ShowMenu = false;

WNDPROC originalWndProc = NULL;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_ShowMenu && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
    {
        return true;
    }
    return CallWindowProc(originalWndProc, hWnd, msg, wParam, lParam);
}

void InitImGui(IDirect3DDevice9* pDevice)
{
    D3DDEVICE_CREATION_PARAMETERS parameters;
    pDevice->GetCreationParameters(&parameters);

    HWND window = parameters.hFocusWindow;
    originalWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX9_Init(pDevice);
    return;
}

HRESULT __stdcall hookedEndScene(IDirect3DDevice9* pDevice) //A function containing a bunch of rendering process, that is gonna be hooked.
{
    static bool initialized = false;

    if (!initialized)
    {
        InitImGui(pDevice);
        initialized = true;
    }
    else
    {
        if (g_ShowMenu)
        {
            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Menu", &g_ShowMenu);
            ImGui::Checkbox("Aim bot", &bAimbot);
            ImGui::End();

            ImGui::EndFrame();
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        }
    }
    return originalEndScene(pDevice);
}

void hookEndScene() {
    uintptr_t shaderapidx9 = reinterpret_cast<uintptr_t>(GetModuleHandle("shaderapidx9.dll"));
    IDirect3DDevice9* pDevice = *reinterpret_cast<IDirect3DDevice9**>(shaderapidx9 + dwppDirect3DDevice9);

    void** vTable = *reinterpret_cast<void***>(pDevice);

    originalEndScene = (endScene)DetourFunction((PBYTE)vTable[42],(PBYTE)hookedEndScene);
}
