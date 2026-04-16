#include "gui.h"
#include "config.h" // Add this include at the top
#include "engine_hooks.h"
#include "hooks.h"

#include "../ext/imgui/imgui.h"
#include "../ext/imgui/imgui_impl_dx9.h"
#include "../ext/imgui/imgui_impl_win32.h"
#include "../ext/imgui/imgui_internal.h"

#include "engine_hooks.h"
#include "functions.h"
#include "variables.h"

#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

using namespace functions;

static char sens[8] = "";

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window,
                                                             UINT message,
                                                             WPARAM wideParam,
                                                             LPARAM longParam);

// window process
LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam,
                               LPARAM longParam);

bool gui::SetupWindowClass(const char *windowClassName) noexcept {
  // Populate Window Class
  windowClass.cbSize = sizeof(WNDCLASSEX);
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = DefWindowProc;
  windowClass.cbClsExtra = 0;
  windowClass.cbWndExtra = 0;
  windowClass.hInstance = GetModuleHandle(NULL);
  windowClass.hIcon = NULL;
  windowClass.hCursor = NULL;
  windowClass.hbrBackground = NULL;
  windowClass.lpszMenuName = NULL;
  windowClass.lpszClassName = windowClassName;
  windowClass.hIconSm = NULL;

  // register
  if (!RegisterClassEx(&windowClass)) {
    return false;
  }

  return true;
}

void gui::DestroyWindowClass() noexcept {
  UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::SetupWindow(const char *windowName) noexcept {
  // Setup Temp Window
  window =
      CreateWindow(windowClass.lpszClassName, windowName, WS_OVERLAPPEDWINDOW,
                   0, 0, 100, 100, 0, 0, windowClass.hInstance, 0);
  if (!window) {
    return false;
  }
  return true;
}
void gui::DestroyWindow() noexcept {
  if (window) {
    DestroyWindow(window);
  }
}

bool gui::SetupDirectX() noexcept {
  const auto handle = GetModuleHandle("d3d9.dll");

  if (!handle) {
    return false;
  }

  using CreateFn = LPDIRECT3D9(__stdcall *)(UINT);

  const auto create =
      reinterpret_cast<CreateFn>(GetProcAddress(handle, "Direct3DCreate9"));

  if (!create) {
    return false;
  }

  d3d9 = create(D3D_SDK_VERSION);

  if (!d3d9) {
    return false;
  }

  D3DPRESENT_PARAMETERS params = {};
  params.BackBufferWidth = 0;
  params.BackBufferHeight = 0;
  params.BackBufferFormat = D3DFMT_UNKNOWN;
  params.BackBufferCount = 0;
  params.MultiSampleType = D3DMULTISAMPLE_NONE;
  params.MultiSampleQuality = NULL;
  params.hDeviceWindow = window;
  params.Windowed = 1;
  params.EnableAutoDepthStencil = 0;
  params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
  params.Flags = NULL;
  params.FullScreen_RefreshRateInHz = 0;
  params.PresentationInterval = 0;

  if (d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, window,
                         D3DCREATE_SOFTWARE_VERTEXPROCESSING |
                             D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
                         &params, &device) < 0)
    return false;

  return true;
}
void gui::DestroyDirectX() noexcept {
  if (device) {
    device->Release();
    device = NULL;
  }
  if (d3d9) {
    d3d9->Release();
    d3d9 = NULL;
  }
}

// Setup Device
void gui::Setup() {
  if (!SetupWindowClass("hackClass001"))
    throw std::runtime_error("Failed to create window class.");

  if (!SetupWindow("Hack Window"))
    throw std::runtime_error("Failed to create window.");

  if (!SetupDirectX())
    throw std::runtime_error("Failed to setup DirectX.");

  DestroyWindow();
  DestroyWindowClass();
}
void gui::SetupMenu(LPDIRECT3DDEVICE9 device) noexcept {
  auto params = D3DDEVICE_CREATION_PARAMETERS{};
  device->GetCreationParameters(&params);

  window = params.hFocusWindow;

  originalWindowProcess = reinterpret_cast<WNDPROC>(SetWindowLongPtr(
      window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProcess)));

  // Imgui
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(window);
  ImGui_ImplDX9_Init(device);

  functions::InitializeRawInput(window);

  setup = true;
}

void randomStyle() {
  ImGuiStyle *style = &ImGui::GetStyle();
  ImVec4 *colors = style->Colors;

  // Base colors for a pleasant and modern dark theme with dark accents
  colors[ImGuiCol_Text] =
      ImVec4(0.92f, 0.93f, 0.94f, 1.00f); // Light grey text for readability
  colors[ImGuiCol_TextDisabled] =
      ImVec4(0.50f, 0.52f, 0.54f, 1.00f); // Subtle grey for disabled text
  colors[ImGuiCol_WindowBg] =
      ImVec4(0.14f, 0.14f, 0.16f, 1.00f); // Dark background with a hint of blue
  colors[ImGuiCol_ChildBg] =
      ImVec4(0.16f, 0.16f, 0.18f, 1.00f); // Slightly lighter for child elements
  colors[ImGuiCol_PopupBg] =
      ImVec4(0.18f, 0.18f, 0.20f, 1.00f); // Popup background
  colors[ImGuiCol_Border] =
      ImVec4(0.28f, 0.29f, 0.30f, 0.60f); // Soft border color
  colors[ImGuiCol_BorderShadow] =
      ImVec4(0.00f, 0.00f, 0.00f, 0.00f); // No border shadow
  colors[ImGuiCol_FrameBg] =
      ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Frame background
  colors[ImGuiCol_FrameBgHovered] =
      ImVec4(0.22f, 0.24f, 0.26f, 1.00f); // Frame hover effect
  colors[ImGuiCol_FrameBgActive] =
      ImVec4(0.24f, 0.26f, 0.28f, 1.00f); // Active frame background
  colors[ImGuiCol_TitleBg] =
      ImVec4(0.14f, 0.14f, 0.16f, 1.00f); // Title background
  colors[ImGuiCol_TitleBgActive] =
      ImVec4(0.16f, 0.16f, 0.18f, 1.00f); // Active title background
  colors[ImGuiCol_TitleBgCollapsed] =
      ImVec4(0.14f, 0.14f, 0.16f, 1.00f); // Collapsed title background
  colors[ImGuiCol_MenuBarBg] =
      ImVec4(0.20f, 0.20f, 0.22f, 1.00f); // Menu bar background
  colors[ImGuiCol_ScrollbarBg] =
      ImVec4(0.16f, 0.16f, 0.18f, 1.00f); // Scrollbar background
  colors[ImGuiCol_ScrollbarGrab] =
      ImVec4(0.24f, 0.26f, 0.28f, 1.00f); // Dark accent for scrollbar grab
  colors[ImGuiCol_ScrollbarGrabHovered] =
      ImVec4(0.28f, 0.30f, 0.32f, 1.00f); // Scrollbar grab hover
  colors[ImGuiCol_ScrollbarGrabActive] =
      ImVec4(0.32f, 0.34f, 0.36f, 1.00f); // Scrollbar grab active
  colors[ImGuiCol_CheckMark] =
      ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Dark blue checkmark
  colors[ImGuiCol_SliderGrab] =
      ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // Dark blue slider grab
  colors[ImGuiCol_SliderGrabActive] =
      ImVec4(0.40f, 0.50f, 0.60f, 1.00f); // Active slider grab
  colors[ImGuiCol_Button] =
      ImVec4(0.24f, 0.34f, 0.44f, 1.00f); // Dark blue button
  colors[ImGuiCol_ButtonHovered] =
      ImVec4(0.28f, 0.38f, 0.48f, 1.00f); // Button hover effect
  colors[ImGuiCol_ButtonActive] =
      ImVec4(0.32f, 0.42f, 0.52f, 1.00f); // Active button
  colors[ImGuiCol_Header] =
      ImVec4(0.24f, 0.34f, 0.44f, 1.00f); // Header color similar to button
  colors[ImGuiCol_HeaderHovered] =
      ImVec4(0.28f, 0.38f, 0.48f, 1.00f); // Header hover effect
  colors[ImGuiCol_HeaderActive] =
      ImVec4(0.32f, 0.42f, 0.52f, 1.00f); // Active header
  colors[ImGuiCol_Separator] =
      ImVec4(0.28f, 0.29f, 0.30f, 1.00f); // Separator color
  colors[ImGuiCol_SeparatorHovered] =
      ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Hover effect for separator
  colors[ImGuiCol_SeparatorActive] =
      ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Active separator
  colors[ImGuiCol_ResizeGrip] =
      ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // Resize grip
  colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(0.40f, 0.50f, 0.60f, 1.00f); // Hover effect for resize grip
  colors[ImGuiCol_ResizeGripActive] =
      ImVec4(0.44f, 0.54f, 0.64f, 1.00f); // Active resize grip
  colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Inactive tab
  colors[ImGuiCol_TabHovered] =
      ImVec4(0.28f, 0.38f, 0.48f, 1.00f); // Hover effect for tab
  colors[ImGuiCol_TabActive] =
      ImVec4(0.24f, 0.34f, 0.44f, 1.00f); // Active tab color
  colors[ImGuiCol_TabUnfocused] =
      ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Unfocused tab
  colors[ImGuiCol_TabUnfocusedActive] =
      ImVec4(0.24f, 0.34f, 0.44f, 1.00f); // Active but unfocused tab
  colors[ImGuiCol_PlotLines] = ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Plot lines
  colors[ImGuiCol_PlotLinesHovered] =
      ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Hover effect for plot lines
  colors[ImGuiCol_PlotHistogram] =
      ImVec4(0.36f, 0.46f, 0.56f, 1.00f); // Histogram color
  colors[ImGuiCol_PlotHistogramHovered] =
      ImVec4(0.40f, 0.50f, 0.60f, 1.00f); // Hover effect for histogram
  colors[ImGuiCol_TableHeaderBg] =
      ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Table header background
  colors[ImGuiCol_TableBorderStrong] =
      ImVec4(0.28f, 0.29f, 0.30f, 1.00f); // Strong border for tables
  colors[ImGuiCol_TableBorderLight] =
      ImVec4(0.24f, 0.25f, 0.26f, 1.00f); // Light border for tables
  colors[ImGuiCol_TableRowBg] =
      ImVec4(0.20f, 0.22f, 0.24f, 1.00f); // Table row background
  colors[ImGuiCol_TableRowBgAlt] =
      ImVec4(0.22f, 0.24f, 0.26f, 1.00f); // Alternate row background
  colors[ImGuiCol_TextSelectedBg] =
      ImVec4(0.24f, 0.34f, 0.44f, 0.35f); // Selected text background
  colors[ImGuiCol_DragDropTarget] =
      ImVec4(0.46f, 0.56f, 0.66f, 0.90f); // Drag and drop target
  colors[ImGuiCol_NavHighlight] =
      ImVec4(0.46f, 0.56f, 0.66f, 1.00f); // Navigation highlight
  colors[ImGuiCol_NavWindowingHighlight] =
      ImVec4(1.00f, 1.00f, 1.00f, 0.70f); // Windowing highlight
  colors[ImGuiCol_NavWindowingDimBg] =
      ImVec4(0.80f, 0.80f, 0.80f, 0.20f); // Dim background for windowing
  colors[ImGuiCol_ModalWindowDimBg] =
      ImVec4(0.80f, 0.80f, 0.80f, 0.35f); // Dim background for modal windows

  // Style adjustments
  style->WindowPadding = ImVec2(8.00f, 8.00f);
  style->FramePadding = ImVec2(5.00f, 2.00f);
  style->CellPadding = ImVec2(6.00f, 6.00f);
  style->ItemSpacing = ImVec2(6.00f, 6.00f);
  style->ItemInnerSpacing = ImVec2(6.00f, 6.00f);
  style->TouchExtraPadding = ImVec2(0.00f, 0.00f);
  style->IndentSpacing = 25;
  style->ScrollbarSize = 11;
  style->GrabMinSize = 10;
  style->WindowBorderSize = 1;
  style->ChildBorderSize = 1;
  style->PopupBorderSize = 1;
  style->FrameBorderSize = 1;
  style->TabBorderSize = 1;
  style->WindowRounding = 7;
  style->ChildRounding = 4;
  style->FrameRounding = 3;
  style->PopupRounding = 4;
  style->ScrollbarRounding = 9;
  style->GrabRounding = 3;
  style->LogSliderDeadzone = 4;
  style->TabRounding = 4;
}

void gui::Destroy() noexcept {
  ImGui_ImplDX9_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

  // restore wnd proc
  SetWindowLongPtr(window, GWLP_WNDPROC,
                   reinterpret_cast<LONG_PTR>(originalWindowProcess));

  DestroyDirectX();
}
void gui::Render() noexcept {
  /*
  ImGuiIO& io = ImGui::GetIO();
  ImFont* Verdana =
  io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Verdana.ttf", (13.666f
  + 1.0f), NULL, io.Fonts->GetGlyphRangesDefault()); //13 and 666 soo edgy. //
  default windows font ImGui::PushFont(Verdana);
  */
  if (!gui::windowSizeStartUp) {
    ImGui::SetNextWindowSize(ImVec2(WIDTH, HEIGHT));
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigDebugHighlightIdConflicts = false;
    gui::windowSizeStartUp = true;
  }

  ImGui::Begin("dismay's MW2 degen tool", &open);
  {
    // randomStyle();
    /*
             if (ImGui::Checkbox("debug", &variables::debug))
    {
            ImGui::Text("Welcome to dismay's MW2 degen tool!");
    }
    */

    if (ImGui::BeginTabBar("MainTabBar")) {
      if (ImGui::BeginTabItem("Main")) {
        ImGui::Text("Main:");
        if (ImGui::Checkbox("Enable Text Chat", &variables::bChat))
          functions::funChat();
        ImGui::SameLine();
        if (ImGui::Checkbox("Enable Mouse 1:1", &variables::fFOVMin))
          functions::funFOVMin();
        if (variables::fFOVMin) {
          ImGui::SameLine();
          ImGui::SetNextItemWidth(120);
          if (ImGui::SliderFloat("##FOVMin", &variables::fFOVMinSlider, 1.0f, 160.0f, "%.1f"))
            functions::funFOVMin();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Iron Sight Intervention",
                            &variables::bIronSightInter))
          functions::doIronSight();
        ImGui::SameLine();
        static int mouseFixRadio = 1;
        ImGui::RadioButton("Mouse Fix", &mouseFixRadio, 1);
        if (ImGui::Checkbox("noSun", &variables::bDrawSun))
          functions::fuckTheSunAway();
        ImGui::SameLine();
        if (ImGui::Checkbox("noCamos", &variables::fNoCamoBool)) {
          functions::sendNoCamo();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("noFog", &variables::fNoCFogBool)) {
          functions::sendNoFog();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("noBullets", &variables::fNoBulletsBool)) {
          functions::sendNoBullets();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Movie Mode", &variables::fMovieBool)) {
          functions::sendMovie();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Clear Glass", &variables::bGlass)) {
          functions::clearGlass();
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Ping Text", &variables::bPingText))
          Cbuf_AddText(0, variables::bPingText ? "cg_scoreboardPingText 1"
                                               : "cg_scoreboardPingText 0");
        {
          variables::fSens = functions::readSensitivity();
          ImGui::InputFloat("Sensitivity", &variables::fSens, 0.1f, 1.0f,
                            "%.2f");
          if (variables::fSens < 0.0f)
            variables::fSens = 0.0f;
          functions::writeSensitivity(variables::fSens);
        }
        ImGui::SliderInt("Frames Per Second", &variables::iFPS, 0, 500);
        ImGui::SliderFloat("Field Of View", &variables::fFieldOfView, 65.0f,
                           120.0f, "%.0f"); // Min | Max | Flag
        if (ImGui::SliderFloat("Map Size", &variables::fMapSize, 1.0f, 2.0f))
          functions::sendMapSize();
        ImGui::Text("Fullbright: ");
        ImGui::SameLine();
        if (ImGui::Button("Invert")) {
          variables::iFullbright = 0;
          variables::iLightMap = 0;
          Cbuf_AddText(0, "r_fullbright 0;r_lightMap 0;");
        }
        ImGui::SameLine();
        if (ImGui::Button("Normal")) {
          variables::iFullbright = 0;
          variables::iLightMap = 1;
          Cbuf_AddText(0, "r_fullbright 0;r_lightMap 1;");
        }
        ImGui::SameLine();
        if (ImGui::Button("Super")) {
          variables::iFullbright = 0;
          variables::iLightMap = 2;
          Cbuf_AddText(0, "r_fullbright 0;r_lightMap 2;");
        }
        ImGui::SameLine();
        if (ImGui::Button("Slight")) {
          variables::iFullbright = 0;
          variables::iLightMap = 3;
          Cbuf_AddText(0, "r_fullbright 0;r_lightMap 3;");
        }
        ImGui::SameLine();
        if (ImGui::Button("Dullish")) {
          variables::iFullbright = 1;
          Cbuf_AddText(0, "r_fullbright 1;");
        }
        ImGui::Separator();
        if (ImGui::Button("Save Config")) {
          config::Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Config")) {
          config::Load();
          functions::sendFPSandFOV();
          functions::sendMapSize();
          functions::funChat();
          functions::funFOVMin();
          functions::funMouseFix();
          functions::fuckTheSunAway();
          functions::sendNoCamo();
          functions::sendNoFog();
          functions::sendNoBullets();
          functions::sendMovie();
          functions::clearGlass();
          const std::string lightMapCmd =
              "r_lightMap " + std::to_string(variables::iLightMap) + ";";
          const std::string fullbrightCmd =
              "r_fullbright " + std::to_string(variables::iFullbright) + ";";
          if (variables::iFullbright != 0) {
            Cbuf_AddText(0, lightMapCmd.c_str());
            Cbuf_AddText(0, fullbrightCmd.c_str());
          } else {
            Cbuf_AddText(0, fullbrightCmd.c_str());
            Cbuf_AddText(0, lightMapCmd.c_str());
          }
        }
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Account")) {
        ImGui::Text("Account:");
        ImGui::SliderInt("Prestige", &variables::iPrestige, 0, 11);
        if (ImGui::Button("Send Prestige"))
          functions::sendPrestige(variables::iPrestige);
        if (ImGui::Button("Level 1"))
          functions::doLevel1();
        ImGui::SameLine();
        if (ImGui::Button("Level 70"))
          functions::doLevel70();
        if (ImGui::Button("Unlock All"))
          functions::unlockAll();
        if (ImGui::Button("Give Gold Deagle Classes"))
          functions::doGiveDeag();
        ImGui::Text("Stat Editor");
        ImGui::InputText("Wins", variables::BarracksWins,
                         IM_ARRAYSIZE(variables::BarracksWins));
        ImGui::InputText("Losses", variables::BarracksLosses,
                         IM_ARRAYSIZE(variables::BarracksLosses));
        ImGui::InputText("Ties", variables::BarracksTies,
                         IM_ARRAYSIZE(variables::BarracksTies));
        ImGui::InputText("Win Streak", variables::BarracksWinStreak,
                         IM_ARRAYSIZE(variables::BarracksWinStreak));
        ImGui::InputText("kills", variables::BarracksKills,
                         IM_ARRAYSIZE(variables::BarracksKills));
        ImGui::InputText("Headshots", variables::BarracksHeadshots,
                         IM_ARRAYSIZE(variables::BarracksHeadshots));
        ImGui::InputText("Assists", variables::BarracksAssists,
                         IM_ARRAYSIZE(variables::BarracksAssists));
        ImGui::InputText("KillStreaks", variables::BarracksKillStreak,
                         IM_ARRAYSIZE(variables::BarracksKillStreak));
        ImGui::InputText("Deaths", variables::BarracksDeaths,
                         IM_ARRAYSIZE(variables::BarracksDeaths));
        ImGui::InputText("Time Played", variables::BarracksTimePlayed,
                         IM_ARRAYSIZE(variables::BarracksTimePlayed));
        if (ImGui::Button("Save Stats"))
          functions::doSaveBarracks();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Server")) {
        ImGui::Text("Server:");
        ImGui::Text("Server Commands");
        ImGui::Text("s = setClientDvar (Infects everyone in the lobby)");
        ImGui::Text(
            "c = iPrintInBold (Puts Text In Center Of Screen, not permanent)");
        ImGui::Text("f = iPrintIn (Text Above Kill feed, not permanent)");
        ImGui::Text("J = setPlayerData (Allows You To Unlock Challenges, Sets "
                    "Stats, etc.)");
        ImGui::Text("M = setVisionNaked (Sets on of the _mp visions)");
        ImGui::Text("n = player Volume?? (default is 1. 99999999999 is "
                    "possible WARNING!!!)");
        ImGui::Text("u = Open popups?? (u 20, u 1, u 3, etc???. they stack. "
                    "tab to remove)");
        ImGui::Separator();
        ImGui::Checkbox("Enable Client Based Commands",
                        &variables::bEnableNameLogging);
        if (variables::bEnableNameLogging) {
          for (int i = 0; i <= 17; ++i) {
            char label[128];
            snprintf(label, sizeof(label), "[%d] %s", i,
                     functions::getPlayerName(i));
            if (i % 4 != 0)
              ImGui::SameLine();
            if (ImGui::Button(label, ImVec2(170, 20)))
              SV_GameSendServerCommand(i, 0, (char *)variables::ServerCommand);
          }
        }
        ImGui::InputText("Server Command", variables::ServerCommand,
                         IM_ARRAYSIZE(variables::ServerCommand));
        if (ImGui::Button("Send Server Command to ALL PLAYERS")) {
          for (int i = 0; i <= 17; ++i) {
            SV_GameSendServerCommand(i, 0, (char *)variables::ServerCommand);
          }
        }
        if (ImGui::Button("True Unlock All (all clients)")) {
          for (int i = 0; i <= 17; ++i) {
            SV_GameSendServerCommand(i, 0, (char *)"J 3643 0A");
            SV_GameSendServerCommand(i, 0, (char *)"J 3644 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3645 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3646 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3647 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3648 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3649 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3650 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3651 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3652 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3653 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3654 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3655 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3656 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3657 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3658 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3659 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3660 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3661 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3662 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3663 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3664 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3665 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3666 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3667 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3668 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3669 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3670 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3671 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3672 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3673 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3674 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3675 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3676 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3677 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3678 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3679 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3680 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3681 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3682 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3683 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3684 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3685 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3686 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3687 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3688 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3689 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3690 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3691 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3692 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3693 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3694 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3695 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3696 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3697 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3698 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3699 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3700 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3701 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3702 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3703 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3704 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3705 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3706 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3707 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3708 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3709 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3710 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3711 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3712 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3713 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3714 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3715 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3716 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3717 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3718 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3719 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3720 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3721 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3722 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3723 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3724 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3725 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3726 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 3727 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3728 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3729 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3730 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3731 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3732 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3733 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3734 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3735 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3736 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3737 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3738 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3739 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3740 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3741 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3742 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3743 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3744 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3745 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3746 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3747 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3748 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3749 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3750 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3751 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3752 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3753 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3754 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3755 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3756 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3757 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3758 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3759 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3760 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3761 02");
            SV_GameSendServerCommand(
                i, 0, (char *)"c \"^6Unlock ^5All^7: ^225 Percent\"");
            SV_GameSendServerCommand(i, 0, (char *)"J 3762 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3763 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3764 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3765 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3766 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3767 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3768 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3769 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3770 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3771 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3772 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3773 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3774 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3775 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3776 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3777 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3778 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3779 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3780 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3781 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3782 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3783 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3784 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3785 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3786 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3787 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3788 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3789 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3790 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3791 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3792 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3793 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3794 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3795 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3796 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3797 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3798 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3799 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3800 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3801 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3802 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3803 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3804 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3805 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3806 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3807 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3808 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3809 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3810 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3811 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3812 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3813 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3814 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3815 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3816 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3817 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3818 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3819 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3820 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3821 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3822 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3823 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3824 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3825 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3826 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3827 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3828 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3829 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3830 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3831 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3832 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3833 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3834 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3835 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3836 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3837 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3838 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3839 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3840 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3841 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3842 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3843 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3844 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3845 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3846 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3847 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3848 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3849 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3850 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3851 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3852 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3853 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3854 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3855 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3856 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3857 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3858 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3859 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3860 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3861 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3862 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3863 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3864 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3865 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3866 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3867 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3868 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3869 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3870 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3871 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3872 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3873 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3874 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3875 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3876 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3877 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3878 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3879 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3880 02");
            SV_GameSendServerCommand(
                i, 0, (char *)"c \"^6Unlock ^5All^7: ^250 Percent\"");
            SV_GameSendServerCommand(i, 0, (char *)"J 3881 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3882 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3883 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3884 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3885 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3886 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3887 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3888 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3889 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3890 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3891 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3892 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3893 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3894 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3895 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3896 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3897 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3898 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3899 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3900 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3901 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3902 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3903 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3904 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3905 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3906 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3907 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3908 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3909 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3910 06");
            SV_GameSendServerCommand(i, 0, (char *)"J 3911 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3912 06");
            SV_GameSendServerCommand(i, 0, (char *)"J 3913 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3914 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3915 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3916 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3917 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3918 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3919 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3920 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3921 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3922 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3923 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3924 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3925 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3926 09");
            SV_GameSendServerCommand(i, 0, (char *)"J 3927 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3928 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3929 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3930 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3931 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3932 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3933 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3934 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3935 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3936 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3937 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3938 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3939 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3940 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3941 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3942 07");
            SV_GameSendServerCommand(i, 0, (char *)"J 3943 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3944 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3945 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3946 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3947 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3948 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3949 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3950 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3951 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3952 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3953 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3954 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3955 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3956 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3957 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3958 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3959 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3960 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3961 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3962 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3963 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3964 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3965 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3966 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3967 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3968 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3969 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3970 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3971 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3972 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3973 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3974 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 3975 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 3976 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 3977 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 3978 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 3979 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 3980 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 3981 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 3982 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 3983 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3984 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3985 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3986 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3987 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3988 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3989 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3990 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3991 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3992 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3993 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3994 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3995 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3996 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3997 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 3998 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 3999 03");
            SV_GameSendServerCommand(
                i, 0, (char *)"c \"^6Unlock ^5All^7: ^275 Percent\"");
            SV_GameSendServerCommand(i, 0, (char *)"J 4000 03");
            SV_GameSendServerCommand(i, 0, (char *)"J 4001 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4002 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4003 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4004 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4005 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4006 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4007 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4008 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4009 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4010 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4011 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4012 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4013 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4014 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4015 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4016 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4017 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4018 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4019 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4020 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4021 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4022 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4023 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4024 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4025 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4026 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4027 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4028 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4029 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4030 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4031 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4032 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4033 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4034 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4035 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4036 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4037 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4038 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4039 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4040 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4041 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4042 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4043 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4044 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4045 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4046 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4047 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4048 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4049 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4050 03");
            SV_GameSendServerCommand(i, 0, (char *)"J 4051 03");
            SV_GameSendServerCommand(i, 0, (char *)"J 4052 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4053 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4054 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4055 03");
            SV_GameSendServerCommand(i, 0, (char *)"J 4056 03");
            SV_GameSendServerCommand(i, 0, (char *)"J 4057 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4058 03");
            SV_GameSendServerCommand(i, 0, (char *)"J 4059 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4060 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4061 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4062 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4063 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4064 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4065 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4066 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4067 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4068 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4069 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4070 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4071 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4072 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4073 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4074 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4075 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4076 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4077 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4078 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4079 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4080 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4081 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4082 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4083 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4084 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4085 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4086 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4087 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4088 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4089 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4090 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4091 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4092 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4093 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4094 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4095 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4096 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4097 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4098 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4099 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4100 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4101 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4102 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4103 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4104 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4105 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4106 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4107 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4108 04");
            SV_GameSendServerCommand(i, 0, (char *)"J 4109 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4110 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4111 03");
            SV_GameSendServerCommand(i, 0, (char *)"J 4112 03");
            SV_GameSendServerCommand(i, 0, (char *)"J 4113 03");
            SV_GameSendServerCommand(i, 0, (char *)"J 4114 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4115 03");
            SV_GameSendServerCommand(i, 0, (char *)"J 4116 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 4117 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 4118 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 4119 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 4120 05");
            SV_GameSendServerCommand(i, 0, (char *)"J 4121 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 4122 02");
            SV_GameSendServerCommand(i, 0, (char *)"J 6525 40");
            SV_GameSendServerCommand(i, 0, (char *)"J 6759 7A");
            SV_GameSendServerCommand(i, 0, (char *)"J 6760 4D");
            SV_GameSendServerCommand(i, 0, (char *)"J 6761 99");
            SV_GameSendServerCommand(i, 0, (char *)"J 6762 03");
            std::cout << "[>>>] Unlock All: Client " << i << " COMPLETED!"
                      << std::endl;
          }
        }
        ImGui::Separator();
        ImGui::Text("Vision List:");
        ImGui::Separator();
        ImGui::InputTextMultiline(
            "##VisionList", (char *)variables::FullVisionListString,
            IM_ARRAYSIZE(variables::FullVisionListString), ImVec2(0, 150),
            ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_Multiline);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Console")) {
        ImGui::Text("Console:");
        ImGui::InputText("Console", variables::Console,
                         IM_ARRAYSIZE(variables::Console));
        if (ImGui::Button("Send Console"))
          Cbuf_AddText(0, variables::Console);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Fun")) {
        ImGui::Text("Fun:");
        if (ImGui::Button("ui_debugMode 0"))
          Cbuf_AddText(0, "ui_debugMode 0");
        ImGui::SameLine();
        if (ImGui::Button("ui_debugMode 1"))
          Cbuf_AddText(0, "ui_debugMode 1");
        if (ImGui::Button("ui_showlist 0"))
          Cbuf_AddText(0, "ui_showlist 0");
        ImGui::SameLine();
        if (ImGui::Button("ui_showlist 1"))
          Cbuf_AddText(0, "ui_showlist 1");
        if (ImGui::Button("scoreboardfont 0"))
          Cbuf_AddText(0, "cg_scoreboardfont 1");
        ImGui::SameLine();
        if (ImGui::Button("scoreboardfont 1"))
          Cbuf_AddText(0, "cg_scoreboardfont 5");
        ImGui::SameLine();
        if (ImGui::Button("scoreboardfont 2"))
          Cbuf_AddText(0, "cg_scoreboardfont 6");
        ImGui::SameLine();
        if (ImGui::Button("scoreboardfont 3"))
          Cbuf_AddText(0, "cg_scoreboardfont 4");
        ImGui::SameLine();
        if (ImGui::Button("scoreboardfont 4"))
          Cbuf_AddText(0, "cg_scoreboardfont 9");
        ImGui::Text("Weapon View Model:");
        if (ImGui::SliderFloat("Gun X", &variables::fcg_gun_x, -5.0f, 5.0f,
                               "%.3f"))
          functions::sendViewModel();
        ImGui::SameLine();
        if (ImGui::Button("Default X")) {
          variables::fcg_gun_x = 0.0f;
          functions::sendViewModel();
        }
        if (ImGui::SliderFloat("Gun Y", &variables::fcg_gun_y, -5.0f, 5.0f,
                               "%.3f"))
          functions::sendViewModel();
        ImGui::SameLine();
        ImGui::SameLine();
        if (ImGui::Button("Default Y")) {
          variables::fcg_gun_y = 0.0f;
          functions::sendViewModel();
        }
        if (ImGui::SliderFloat("Gun Z", &variables::fcg_gun_z, -5.0f, 5.0f,
                               "%.3f"))
          functions::sendViewModel();
        ImGui::SameLine();
        if (ImGui::Button("Default Z")) {
          variables::fcg_gun_z = 0.0f;
          functions::sendViewModel();
        }
        ImGui::Separator();
        ImGui::Text("Cod Jumper:");
        if (ImGui::Checkbox("Enable Bounces", &variables::bBounces))
          functions::sendBouncesToggle();
        ImGui::SameLine();
        if (ImGui::Checkbox("Enable Elevators", &variables::bElevators))
          functions::sendElevatorsToggle();
        /*
        if (ImGui::Button("Jump Slowdown Enabled"))
                Cbuf_AddText(0, "jump_slowdownEnable 1"); ImGui::SameLine();
        if (ImGui::Button("Jump Slowdown Disabled"))
                Cbuf_AddText(0, "jump_slowdownEnable 0");
                DVAR HAS BEEN REMOVED? </3
        */

        ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Misc")) {
        ImGui::Text("Misc:");
        ImGui::Text("Menu Name:");
        ImGui::InputText("OpenMenu", variables::MenuName,
                         IM_ARRAYSIZE(variables::MenuName));
        if (ImGui::Button("Open Menu"))
          functions::sendOpenMenu(variables::MenuName);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Private Match")) {
        ImGui::Text("Private Match:");
        if (ImGui::Button("Give Shit ton of Ammo")) {
          *(int *)(0x1B0E42C + (functions::getHostId() * 0x366C)) =
              INT_MAX; // Primary Gun
          *(int *)(0x1B0E41C + (functions::getHostId() * 0x366C)) =
              INT_MAX; // Secondary Gun

          *(int *)(0x1B0E4A0 + (functions::getHostId() * 0x366C)) =
              INT_MAX; // equipment
          *(int *)(0x1B0E4B8 + (functions::getHostId() * 0x366C)) =
              INT_MAX; // equipment
        }

        if (ImGui::Button("Unlimited time"))
          Cbuf_AddText(
              0,
              "scr_dm_timelimit 0; scr_war_timelimit 0; scr_dom_timelimit 0");
        ImGui::SameLine();
        if (ImGui::Button("Unlimited score"))
          Cbuf_AddText(0, "scr_dm_scorelimit 0; scr_war_scorelimit 0; "
                          "scr_dom_scorelimit 0");
        ImGui::Text("Gamemode (requires restart)");
        if (ImGui::Button("dom"))
          Cbuf_AddText(0,
                       "g_gametype dom; ui_gametype dom; party_gametype dom");
        ImGui::SameLine();
        if (ImGui::Button("SnD"))
          Cbuf_AddText(0, "g_gametype sd; ui_gametype sd; party_gametype sd");
        ImGui::SameLine();
        if (ImGui::Button("ffa"))
          Cbuf_AddText(0,
                       "g_gametype ffa; ui_gametype ffa; party_gametype ffa");
        ImGui::SameLine();
        if (ImGui::Button("HQ"))
          Cbuf_AddText(
              0, "g_gametype koth; ui_gametype koth; party_gametype koth");
        ImGui::SameLine();
        if (ImGui::Button("dem"))
          Cbuf_AddText(0,
                       "g_gametype dem; ui_gametype dem; party_gametype dem");
        ImGui::SameLine();
        if (ImGui::Button("sab"))
          Cbuf_AddText(0,
                       "g_gametype sab; ui_gametype sab; party_gametype sab");
        ImGui::SameLine();
        if (ImGui::Button("CTF"))
          Cbuf_AddText(0,
                       "g_gametype ctf; ui_gametype ctf; party_gametype ctf");
        ImGui::SameLine();
        if (ImGui::Button("GTNW"))
          Cbuf_AddText(
              0, "g_gametype gtnw; ui_gametype gtnw; party_gametype gtnw");
        ImGui::Text("Tweaks:");
        if (ImGui::Button("Sprint Speed 0"))
          Cbuf_AddText(0, "player_sprintSpeedScale 1.5");
        ImGui::SameLine();
        if (ImGui::Button("Sprint Speed 1"))
          Cbuf_AddText(0, "player_sprintSpeedScale 3");
        ImGui::SameLine();
        if (ImGui::Button("Sprint Speed 2"))
          Cbuf_AddText(0, "player_sprintSpeedScale 5");
        if (ImGui::Button("360 Prone 0"))
          Cbuf_AddText(0, "bg_prone_yawcap 85");
        ImGui::SameLine();
        if (ImGui::Button("360 Prone 1"))
          Cbuf_AddText(0, "bg_prone_yawcap 360");
        if (ImGui::Button("360 Ladder 0"))
          Cbuf_AddText(0, "bg_ladder_yawcap 100");
        ImGui::SameLine();
        if (ImGui::Button("360 Ladder 1"))
          Cbuf_AddText(0, "bg_ladder_yawcap 360");
        if (ImGui::Button("Knockback 0"))
          Cbuf_AddText(0, "g_knockback 1000");
        ImGui::SameLine();
        if (ImGui::Button("Knockback 1"))
          Cbuf_AddText(0, "g_knockback 99999");
        if (ImGui::Button("BackSpeed 0"))
          Cbuf_AddText(0, "player_backSpeedScale 0.7");
        ImGui::SameLine();
        if (ImGui::Button("BackSpeed 1"))
          Cbuf_AddText(0, "player_backSpeedScale 5");
        ImGui::SameLine();
        if (ImGui::Button("BackSpeed 2"))
          Cbuf_AddText(0, "player_backSpeedScale 10");
        ImGui::SameLine();
        if (ImGui::Button("BackSpeed 3"))
          Cbuf_AddText(0, "player_backSpeedScale 20");
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Host")) {
        ImGui::Text("Host");
        ImGui::Combo("Select Map", &variables::imap_list_number,
                     variables::map_list, IM_ARRAYSIZE(variables::map_list), 5);
        ImGui::Combo("Select Gamemode", &variables::igamemode_list_number,
                     variables::gamemode_list,
                     IM_ARRAYSIZE(variables::gamemode_list), 5);
        if (ImGui::Button("Change Map")) {
          functions::ChangeGamemode();
          Cbuf_AddCall(0, reinterpret_cast<void*>(functions::ChangeMap));

        }
        ImGui::SameLine();
        if (ImGui::Button("Fast Restart")) {
          functions::ChangeGamemode();
          Cbuf_AddCall(0, reinterpret_cast<void*>(functions::FastRestart));
        }
        if (ImGui::Button("Lock Lobby")) {
          functions::ChangeGamemode();
          Cbuf_AddText(0, "xblive_privatematch 1");
        }
        if (ImGui::Button("Match Settings"))
          OpenMenu(0, "popup_gamesetup");
        if (ImGui::Button("Start Match")) {
          functions::ChangeGamemode();
          functions::doMaxPlayers(variables::iMaxPlayers);
          functions::doStartMatch();
          functions::doBalanceTeams();
        }
        if (ImGui::SliderInt("Max Players", &variables::iMaxPlayers, 2, 18))
          functions::doMaxPlayers(variables::iMaxPlayers);
        ImGui::Checkbox("FFA Team Fix", &variables::bFFATeamFix);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Exploits")) {
        ImGui::Text("Exploits:");
        ImGui::Text("Free DLC Exploit (requires DLC to be downloaded)");
        ImGui::RadioButton("No DLC", &variables::iDLCMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("DLC 1", &variables::iDLCMode, 1);
        ImGui::SameLine();
        ImGui::RadioButton("DLC 2", &variables::iDLCMode, 2);
        ImGui::Separator();
        ImGui::Text("Enter same custom portforward number as VPN to be able to "
                    "host on VPN");
        ImGui::InputInt("Custom port number:", &variables::customPort);
        if (ImGui::Button("Send port number"))
          functions::sendCustomPort();
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("About")) {
        ImGui::Text("About:");
        ImGui::Text("Version: v1.");
        ImGui::Text("Hotkeys:");
        ImGui::Text("F2 Send chat to console.");
        ImGui::Text("F3 Force host.");
        ImGui::Text("F4 Lock lobby.");
        ImGui::Text("F5 Start match.");
        ImGui::Text("Search TDM, F4 change settings, F5, to start");
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Credits")) {
        ImGui::Text("MAJOR Thanks to:");
        ImGui::TextLinkOpenURL("KingsleydotDev",
                               "https://github.com/KingsleydotDev");
        ImGui::TextLinkOpenURL("GRIIM", "https://x.com/GRIIMtB");
        ImGui::Text("Founder (for suggestions and testing)");
        ImGui::Separator();
        ImGui::Text("Check out my GitHub here: ");
        ImGui::TextLinkOpenURL("0xKale", "https://github.com/0xKale");

        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  }
  // ImGui::PopFont();
  ImGui::End();
}

LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam,
                               LPARAM longParam) {
  // toggle menu
  int toggleKey = variables::bUseInsertKey ? VK_INSERT : VK_HOME;
  if (GetAsyncKeyState(toggleKey) & 1) {
    gui::open = !gui::open;
  }
  // loops
  functions::handleMouseCursor();
  functions::sendFPSandFOV();
  functions::doDLCMaps();
  functions::doFFATeamFix();
  functions::handleHotkeys();
  functions::menuUITweaks();
  functions::ApplyRawInputToCamera();

  // --- 8000HZ RAW INPUT FIX ---
  if (functions::HandleRawInputMessage(message, wideParam, longParam)) {
    return DefWindowProcA(window, message, wideParam, longParam);
  }

  // Pass Messages to Imgui
  if (gui::open &&
      ImGui_ImplWin32_WndProcHandler(window, message, wideParam, longParam))
    return 1L;

  return CallWindowProc(gui::originalWindowProcess, window, message, wideParam,
                        longParam);
}
