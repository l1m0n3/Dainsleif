#include "pch.h"
#include "GraphicHook.h"
#include "LocalPlayer.h"

extern bool bQuit, bAimbot, bGlowHack, bNoRecoil, bTriggerBot;
extern uintptr_t moduleBase;
extern int fov; //decleard in dllmain.cpp

using endScene = HRESULT (__stdcall*)(IDirect3DDevice9* pDevice);
endScene originalEndScene = nullptr; //An original endscene which is null now.

bool g_ShowMenu = false;

HWND window = NULL;
WNDPROC originalWndProc = NULL;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_ShowMenu && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    return CallWindowProc(originalWndProc, hWnd, msg, wParam, lParam);
}

void InitImGui(IDirect3DDevice9* pDevice)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX9_Init(pDevice);
    return;
}

void ShutdownImGui()
{
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

ImVec4 enemyGlowColor(0.8f, 0.1f, 0.15f, 1.f);
ImVec4 localGlowColor(0.f, 0.255f, 0.7f, 1.f);

HRESULT __stdcall hookedEndScene(IDirect3DDevice9* pDevice) //A function containing a bunch of rendering process, that is gonna be hooked.
{
    if (g_ShowMenu)
    {
        LocalPlayer* lp = GetLocalPlayer(moduleBase);
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("HACK4CSGO", &g_ShowMenu);
        ImGui::Checkbox("Aim bot", &bAimbot);
        ImGui::Checkbox("Trigger bot", &bTriggerBot);
        ImGui::Checkbox("No Recoil", &bNoRecoil);
        ImGui::Checkbox("Glow Hack", &bGlowHack);
        if (bGlowHack)
        {
            ImGui::ColorEdit4("Enemy color", (float*)&enemyGlowColor);
            ImGui::ColorEdit4("Teammate color", (float*)&localGlowColor);
        }
        if (ImGui::SliderInt("Field of view(FOV)", &fov, 60, 120))
            lp->SetFOV(fov);

        if (ImGui::Button("set to default"))
        {
            bAimbot = false;
            bTriggerBot = false;
            bGlowHack = false;
            bNoRecoil = false;
            fov = 90;
        }
        if (ImGui::Button("Quit")) bQuit = true;
        ImGui::End();

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }
    return originalEndScene(pDevice);
}

void hookEndScene() {
    uintptr_t shaderapidx9 = reinterpret_cast<uintptr_t>(GetModuleHandle("shaderapidx9.dll"));
    IDirect3DDevice9* pDevice = *reinterpret_cast<IDirect3DDevice9**>(shaderapidx9 + dwppDirect3DDevice9);

    D3DDEVICE_CREATION_PARAMETERS parameters;
    pDevice->GetCreationParameters(&parameters);

    window = parameters.hFocusWindow;
    originalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));

    InitImGui(pDevice);

    void** vTable = *reinterpret_cast<void***>(pDevice);
    originalEndScene = reinterpret_cast<endScene>(DetourFunction(reinterpret_cast<PBYTE>(vTable[42]), reinterpret_cast<PBYTE>(hookedEndScene)));
}

void unhookEndScene()
{
    DetourRemove(reinterpret_cast<PBYTE>(originalEndScene), reinterpret_cast<PBYTE>(hookedEndScene));

    SetWindowLong(window, GWL_WNDPROC, reinterpret_cast<LONG>(originalWndProc));

    ShutdownImGui();
}