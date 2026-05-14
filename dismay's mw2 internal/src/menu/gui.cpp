#define IMGUI_DEFINE_MATH_OPERATORS
#include "gui.hpp"

#include <Windows.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <d3d9.h>

#include "../../ext/imgui/imgui_internal.h"
#include "../../ext/imgui/imgui_impl_win32.h"
#include "../../ext/imgui/imgui_impl_dx9.h"
#include "../framework/framework.hpp"
#include "../framework/fonts.h"
#include "../framework/images.h"
#include "../../ext/fonts/iconsfontawesome/fa.h"
#include "../../ext/fonts/iconsfontawesome/IconsFontAwesome6.h"
#include "pages/pages.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {

LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam, LPARAM longParam);

}

namespace gui {
	std::string MenuTitle = "DISMAY";
	float menu_title_draw_alpha = 1.f;

	bool open = true;
	bool setup = false;

	HWND window = nullptr;
	WNDCLASSEX windowClass = {};
	WNDPROC originalWindowProcess = nullptr;

	LPDIRECT3DDEVICE9 device = nullptr;
	LPDIRECT3D9 d3d9 = nullptr;

	void UpdateMenuTitle() {
		static const ULONGLONG pulseHalfMs = 450;
		static const ULONGLONG holdMs = 1000;
		static const ULONGLONG cycleMs = pulseHalfMs * 4 + holdMs;
		// Long quiet period between 666 flicker bursts (ms). Two rand chunks so MSVC rand()
		// (RAND_MAX 32767) still covers the full spread.
		static constexpr ULONGLONG k666CooldownMinMs = 90'000ULL; // 90s base
		static constexpr int k666CooldownRandChunkMs = 30'000;  // 0..29999 per draw

		static ULONGLONG cycleStart = GetTickCount64();
		static ULONGLONG next666At = GetTickCount64() + k666CooldownMinMs
			+ static_cast<ULONGLONG>(rand() % k666CooldownRandChunkMs)
			+ static_cast<ULONGLONG>(rand() % k666CooldownRandChunkMs);
		static bool in666Burst = false;
		static int burstPhase = 0;
		static int burstPhaseCount = 0;
		static ULONGLONG nextBurstPhaseAt = 0;

		const ULONGLONG now = GetTickCount64();

		if (in666Burst) {
			if (now >= nextBurstPhaseAt) {
				++burstPhase;
				if (burstPhase >= burstPhaseCount) {
					in666Burst = false;
					MenuTitle = "DISMAY";
					next666At = now + k666CooldownMinMs
						+ static_cast<ULONGLONG>(rand() % k666CooldownRandChunkMs)
						+ static_cast<ULONGLONG>(rand() % k666CooldownRandChunkMs);
				}
				else {
					MenuTitle = (burstPhase % 2 == 0) ? "666" : "DISMAY";
					nextBurstPhaseAt = now + 35 + static_cast<ULONGLONG>(rand() % 36);
				}
			}
		}
		else if (now >= next666At) {
			const int flashes = 3 + rand() % 4;
			burstPhaseCount = flashes * 2;
			in666Burst = true;
			burstPhase = 0;
			MenuTitle = "666";
			nextBurstPhaseAt = now + 35 + static_cast<ULONGLONG>(rand() % 36);
		}

		float pulseAlpha = 1.f;
		if (!in666Burst) {
			ULONGLONG dt = now - cycleStart;
			if (dt >= cycleMs) {
				cycleStart += (dt / cycleMs) * cycleMs;
				dt = now - cycleStart;
			}

			if (dt < pulseHalfMs) {
				const float u = static_cast<float>(dt) / static_cast<float>(pulseHalfMs);
				pulseAlpha = ImLerp(1.f, 0.f, u);
			}
			else if (dt < pulseHalfMs * 2) {
				const float u = static_cast<float>(dt - pulseHalfMs) / static_cast<float>(pulseHalfMs);
				pulseAlpha = ImLerp(0.f, 1.f, u);
			}
			else if (dt < pulseHalfMs * 2 + holdMs) {
				pulseAlpha = 1.f;
			}
			else if (dt < pulseHalfMs * 3 + holdMs) {
				const float u = static_cast<float>(dt - pulseHalfMs * 2 - holdMs) / static_cast<float>(pulseHalfMs);
				pulseAlpha = ImLerp(1.f, 0.f, u);
			}
			else {
				const float u = static_cast<float>(dt - pulseHalfMs * 3 - holdMs) / static_cast<float>(pulseHalfMs);
				pulseAlpha = ImLerp(0.f, 1.f, u);
			}
		}

		menu_title_draw_alpha = in666Burst ? 1.f : pulseAlpha;
	}

	bool SetupWindowClass(const char* windowClassName) noexcept {
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = DefWindowProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = GetModuleHandle(nullptr);
		windowClass.hIcon = nullptr;
		windowClass.hCursor = nullptr;
		windowClass.hbrBackground = nullptr;
		windowClass.lpszMenuName = nullptr;
		windowClass.lpszClassName = windowClassName;
		windowClass.hIconSm = nullptr;

		return RegisterClassEx(&windowClass) != 0;
	}

	void DestroyWindowClass() noexcept {
		if (!windowClass.lpszClassName || !windowClass.hInstance)
			return;
		UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
		windowClass = {};
	}

	bool SetupWindow(const char* windowName) noexcept {
		window = CreateWindow(
			windowClass.lpszClassName,
			windowName,
			WS_OVERLAPPEDWINDOW,
			0,
			0,
			100,
			100,
			nullptr,
			nullptr,
			windowClass.hInstance,
			nullptr
		);
		return window != nullptr;
	}

	void TearDownHackHostWindow() noexcept {
		if (!window) {
			return;
		}
		::DestroyWindow(window);
		window = nullptr;
	}

	bool SetupDirectX() noexcept {
		const auto handle = GetModuleHandle("d3d9.dll");
		if (!handle) {
			return false;
		}

		using CreateFn = LPDIRECT3D9(__stdcall*)(UINT);
		const auto create = reinterpret_cast<CreateFn>(GetProcAddress(handle, "Direct3DCreate9"));
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
		params.MultiSampleQuality = 0;
		params.hDeviceWindow = window;
		params.Windowed = 1;
		params.EnableAutoDepthStencil = 0;
		params.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
		params.Flags = 0;
		params.FullScreen_RefreshRateInHz = 0;
		params.PresentationInterval = 0;

		return d3d9->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_NULLREF,
			window,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
			&params,
			&device
		) >= 0;
	}

	void DestroyDirectX() noexcept {
		if (device) {
			device->Release();
			device = nullptr;
		}
		if (d3d9) {
			d3d9->Release();
			d3d9 = nullptr;
		}
	}

	void Setup() {
		if (!SetupWindowClass("hackClass001")) {
			throw std::runtime_error("Failed to create window class.");
		}
		if (!SetupWindow("Hack Window")) {
			throw std::runtime_error("Failed to create window.");
		}
		if (!SetupDirectX()) {
			throw std::runtime_error("Failed to setup DirectX.");
		}
	}

	void FinalizeBootstrapSetup() noexcept {
		DestroyDirectX();
		TearDownHackHostWindow();
		DestroyWindowClass();
	}

	void SetupMenu(LPDIRECT3DDEVICE9 currentDevice) noexcept {
		if (!currentDevice || setup)
			return;

		auto params = D3DDEVICE_CREATION_PARAMETERS{};
		if (FAILED(currentDevice->GetCreationParameters(&params)))
			return;

		window = params.hFocusWindow;
		if (!window)
			window = GetForegroundWindow();
		if (!window)
			return;

		originalWindowProcess = reinterpret_cast<WNDPROC>(
			SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProcess))
		);
		if (!originalWindowProcess)
			return;

		ImGui::CreateContext();
		ImGui::StyleColorsDark();

		ImGuiIO& io = ImGui::GetIO();
		fonts::inter_font      = io.Fonts->AddFontFromMemoryTTF(&robotLight, sizeof(robotLight), 17, nullptr, io.Fonts->GetGlyphRangesCyrillic());
		fonts::inter_font_b    = io.Fonts->AddFontFromMemoryTTF(&robotLight, sizeof(robotLight), 18.5f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
		fonts::inter_bold_font = io.Fonts->AddFontFromMemoryTTF(&robotMedium, sizeof(robotMedium), 20, nullptr, io.Fonts->GetGlyphRangesCyrillic());
		fonts::inter_bold_font2 = io.Fonts->AddFontFromMemoryTTF(&robotMedium, sizeof(robotMedium), 17, nullptr, io.Fonts->GetGlyphRangesCyrillic());
		fonts::inter_bold_font3 = io.Fonts->AddFontFromMemoryTTF(&robotMedium, sizeof(robotMedium), 18, nullptr, io.Fonts->GetGlyphRangesCyrillic());
		fonts::inter_bold_font4 = io.Fonts->AddFontFromMemoryTTF(&robotMedium, sizeof(robotMedium), 16, nullptr, io.Fonts->GetGlyphRangesCyrillic());
		static const ImWchar fa_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig fa_cfg{};
		fa_cfg.FontDataOwnedByAtlas = false;
		fonts::fa_font = io.Fonts->AddFontFromMemoryTTF(freesolid900, sizeof(freesolid900), 16.0f, &fa_cfg, fa_ranges);
		if (!fonts::fa_font)
			fonts::fa_font = fonts::inter_bold_font2;

		ImFontConfig morpheus_cfg{};
		morpheus_cfg.FontDataOwnedByAtlas = false;
		fonts::morpheus_title = io.Fonts->AddFontFromMemoryTTF(
			morpheus, sizeof(morpheus), 80.f, &morpheus_cfg, io.Fonts->GetGlyphRangesDefault());
		if (!fonts::morpheus_title)
			fonts::morpheus_title = fonts::inter_bold_font;

		io.FontDefault = fonts::inter_font ? fonts::inter_font : fonts::inter_bold_font2;

		if (!ImGui_ImplWin32_Init(window)) {
			ImGui::DestroyContext();
			return;
		}
		if (!ImGui_ImplDX9_Init(currentDevice)) {
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			return;
		}
		if (!ksd::D3D9MemoryGif_CurrentTexture())
		{
			(void)ksd::D3D9MemoryGif_Load(currentDevice, lain, static_cast<std::uint32_t>(sizeof(lain)));
		}
		setup = true;
	}

	void Destroy() noexcept {
		ksd::D3D9MemoryGif_Unload();
		if (setup) {
			ImGui_ImplDX9_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
			setup = false;
		}

		if (window && originalWindowProcess) {
			SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWindowProcess));
			originalWindowProcess = nullptr;
		}

		DestroyDirectX();
	}

	void Render() noexcept {
		ImGuiStyle* style = &ImGui::GetStyle();

		style->Colors[ImGuiCol_WindowBg] = colors::menu::window_bg;
		style->Colors[ImGuiCol_Border] = colors::menu::border;
		style->ItemSpacing = ImVec2(0, 5);
		style->WindowPadding = ImVec2(0, 0);
		style->WindowRounding = 8.f;

		//ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
		ImGui::SetNextWindowSize(settings::size_menu);
		ImGui::Begin("DISMAY", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus);

		{
			style->Colors[ImGuiCol_WindowBg] = colors::menu::window_bg;
			style->Colors[ImGuiCol_Border] = colors::menu::border;
			style->ItemSpacing = ImVec2(0, 5);
			style->WindowPadding = ImVec2(0, 0);
			style->WindowRounding = 8.f;
		}

		{
			ImGui::SetCursorPos(ImVec2(settings::menu_outer_margin_from_window_edge_px, 10));
			ImGui::BeginChild(
				"General Tabs",
				ImVec2(settings::menu_interior_body_width_px, 60),
				true,
				ImGuiWindowFlags_NoBackground);

			const auto& pos = ImGui::GetWindowPos();
			const auto& draw_list = ImGui::GetWindowDrawList();

			constexpr float tab_strip_start_x = 116.f;
			constexpr float kHeaderBarGap = 8.f;
			const float leftPillW = tab_strip_start_x - kHeaderBarGap;

			ImGui::GetStyle().AntiAliasedLines = true;
			ImGui::GetStyle().AntiAliasedLinesUseTex = true;
			ImGui::GetStyle().AntiAliasedFill = true;

			const ImU32 headerBarCol = ImGui::GetColorU32(menu::general_child);
			draw_list->AddRectFilled(
				ImVec2(pos.x, pos.y),
				ImVec2(pos.x + leftPillW, pos.y + 60.f),
				headerBarCol,
				10.f);
			ksd::D3D9MemoryGif_TickImGui();
			constexpr float kTitlePillH = 60.f;
			ksd::D3D9MemoryGif_DrawInPill(draw_list, ImVec2(pos.x, pos.y), leftPillW, kTitlePillH, 0.30f, 10.f);
			draw_list->AddRectFilled(
				ImVec2(pos.x + tab_strip_start_x, pos.y),
				ImVec2(pos.x + settings::menu_interior_body_width_px, pos.y + 60.f),
				headerBarCol,
				10.f);
			ImFont* const titleFont = fonts::morpheus_title ? fonts::morpheus_title : fonts::inter_bold_font;
			constexpr float titleFontSize = 34.f;
			constexpr float title666FontSize = 72.f;
			static constexpr const char kTitle666Spaced[] = "6 6 6";
			const float titleAlpha = ImClamp(menu_title_draw_alpha, 0.f, 1.f) * style->Alpha;
			const float menuTitleOpacity = ImClamp(menu_title_draw_alpha, 0.f, 1.f);
			const float dismayGlowRamp =
				(menuTitleOpacity <= 0.5f) ? 0.f : ((menuTitleOpacity - 0.5f) / 0.5f);

			const ImVec2 dismayTextNatural = titleFont->CalcTextSizeA(titleFontSize, FLT_MAX, 0.f, "DISMAY", nullptr);
			constexpr float kTitlePillPad = 10.f;
			float dismayTitleDrawSize = titleFontSize;
			if (dismayTextNatural.x > leftPillW - kTitlePillPad)
				dismayTitleDrawSize = titleFontSize * ((leftPillW - kTitlePillPad) / dismayTextNatural.x);
			const ImVec2 dismayTextSize = titleFont->CalcTextSizeA(dismayTitleDrawSize, FLT_MAX, 0.f, "DISMAY", nullptr);
			const float titleBandMidX = pos.x + leftPillW * 0.5f;
			const float dismayBaseX = titleBandMidX - dismayTextSize.x * 0.5f;
			const float dismayBaseY = pos.y + (60.f - dismayTextSize.y) * 0.5f;
			const ImVec2 dismayCenter(
				dismayBaseX + dismayTextSize.x * 0.5f,
				dismayBaseY + dismayTextSize.y * 0.5f);

			const ImVec2 dismayTextPos(dismayBaseX, dismayBaseY);
			const auto draw_dismay_neon = [&](ImFont* font, float fontSize, ImVec2 textPos, ImU32 corePacked,
				const char* text, float glowStrength) {
				const ImVec4 acc = colors::accent_color;
				// Subtle halo (no outer rad-3 ring — reads less “blurry”). Scales with dismayGlowRamp
				// once menu title opacity is above 50%.
				const float glowAgg = dismayGlowRamp * glowStrength * 0.26f * style->Alpha;
				for (int rad = 2; rad >= 1; --rad) {
					const float layerAlpha = (rad == 2) ? 0.038f : 0.065f;
					for (int dy = -rad; dy <= rad; ++dy) {
						for (int dx = -rad; dx <= rad; ++dx) {
							if ((std::max)(std::abs(dx), std::abs(dy)) != rad)
								continue;
							const float a = ImClamp(layerAlpha * glowAgg, 0.f, 1.f);
							const ImU32 halo = ImGui::GetColorU32(ImVec4(acc.x, acc.y, acc.z, a));
							draw_list->AddText(
								font, fontSize, ImVec2(textPos.x + static_cast<float>(dx), textPos.y + static_cast<float>(dy)),
								halo, text);
						}
					}
				}
				draw_list->AddText(font, fontSize, textPos, corePacked, text);
			};

			if (MenuTitle == "666") {
				const ImU32 dismayUnderCol = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.22f * titleAlpha));
				draw_dismay_neon(titleFont, dismayTitleDrawSize, dismayTextPos, dismayUnderCol, "DISMAY", 0.42f);

				const ImVec2 spaced666Size = titleFont->CalcTextSizeA(
					title666FontSize, FLT_MAX, 0.f, kTitle666Spaced, nullptr);
				constexpr float k666RaisePx = 10.f;
				const ImVec2 spaced666Pos(
					dismayCenter.x - spaced666Size.x * 0.5f,
					dismayCenter.y - spaced666Size.y * 0.5f - k666RaisePx);
				const ImU32 red666 = ImGui::GetColorU32(ImVec4(0.42f, 0.05f, 0.05f, titleAlpha));
				draw_list->AddText(titleFont, title666FontSize, spaced666Pos, red666, kTitle666Spaced);
			}
			else {
				const ImU32 coreNeon = ImGui::GetColorU32(ImVec4(
					ImLerp(1.f, colors::accent_color.x, 0.12f),
					ImLerp(1.f, colors::accent_color.y, 0.12f),
					ImLerp(1.f, colors::accent_color.z, 0.12f),
					titleAlpha));
				draw_dismay_neon(titleFont, dismayTitleDrawSize, dismayTextPos, coreNeon, MenuTitle.c_str(), 1.f);
			}

			const char* tab_labels[5] = { "Main", "Account", "Host", "Dedigamer", "About" };
			const float tab_widths[5] = { 80.0f, 90.0f, 74.0f, 102.0f, 72.0f };
			const float tab_spacing = 4.0f;
			float tab_offsets[5] = {};
			for (int i = 1; i < 5; ++i)
				tab_offsets[i] = tab_offsets[i - 1] + tab_widths[i - 1] + tab_spacing;

			const int safe_tab_index = (misc::tab_count < 0) ? 0 : ((misc::tab_count > 4) ? 4 : misc::tab_count);
			misc::anim_tab = ImLerp(misc::anim_tab, tab_offsets[safe_tab_index], ImGui::GetIO().DeltaTime * 15.f);

			const float tab_origin_x = tab_strip_start_x;
			const float tab_text_offset_x = 37.0f; // Matches custom Tab() text offset when icon is present.
			const float indicator_x = tab_origin_x + tab_text_offset_x + misc::anim_tab;
			const float indicator_w = ImGui::CalcTextSize(tab_labels[safe_tab_index]).x;
			ImDrawList* tabStripDrawUx = ImGui::GetWindowDrawList();
			const ImVec2 indicatorMinUx(pos.x + indicator_x, pos.y + 57);
			const ImVec2 indicatorMaxUx(pos.x + indicator_x + indicator_w, pos.y + 60);

			tabStripDrawUx->AddRectFilled(indicatorMinUx, indicatorMaxUx, ImGui::GetColorU32(colors::accent_color), 10.f, ImDrawFlags_RoundCornersTop);

			const ImU32 tab_indicator_glow_packed_ux = ksd::ColorWithAlpha(colors::accent_color, 0.48f);
			ksd::DrawRectShadow(
				tabStripDrawUx,
				indicatorMinUx,
				indicatorMaxUx,
				tab_indicator_glow_packed_ux,
				10.f,
				ImVec2(0.f, 0.f),
				ImDrawFlags_RoundCornersTop,
				10.f);

			ImGui::SetCursorPos(ImVec2(tab_strip_start_x, 12));
			ImGui::BeginGroup(); {
				if (ImGui::Tab("Main", pictures::aim_img, ImVec2(80, 40), 0 == misc::tab_count))
					misc::tab_count = 0;
				ImGui::SameLine(0, 4.f);
				if (ImGui::Tab("Account", pictures::visual_img, ImVec2(90, 40), 1 == misc::tab_count))
					misc::tab_count = 1;
				ImGui::SameLine(0, 4.f);
				if (ImGui::Tab("Host", pictures::misc_img, ImVec2(74, 40), 2 == misc::tab_count))
					misc::tab_count = 2;
				ImGui::SameLine(0, 4.f);
				if (ImGui::Tab("Dedigamer", pictures::misc_img, ImVec2(102, 40), 3 == misc::tab_count))
					misc::tab_count = 3;
				ImGui::SameLine(0, 4.f);
				if (ImGui::Tab("About", pictures::misc_img, ImVec2(72, 40), 4 == misc::tab_count))
					misc::tab_count = 4;
			} ImGui::EndGroup();

			ImGui::EndChild();
		}

		{
			misc::alpha_child = ImLerp(misc::alpha_child, (misc::tab_count == misc::active_tab_count) ? 1.f : 0.f, 15.f * ImGui::GetIO().DeltaTime);
			if (misc::alpha_child < 0.01f && misc::child_add < 0.01f) misc::active_tab_count = misc::tab_count;

			ImGui::SetCursorPos(ImVec2(settings::menu_outer_margin_from_window_edge_px, 80));
			ImGui::BeginChild(
				"Main",
				ImVec2(settings::menu_interior_body_width_px, 440),
				true,
				ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar);

			ImGui::SetCursorPos(ImVec2(settings::menu_body_side_inset_px, 100 - (misc::alpha_child * 100)));
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, misc::alpha_child * style->Alpha);

			if (misc::active_tab_count != 3)
				dedigamer::g_tabOpen.store(false);

			switch (misc::active_tab_count) {
				case 0:
					menu_pages::RenderMainPage();
					break;
				case 1:
					menu_pages::RenderAccountPage();
					break;
				case 2:
					menu_pages::RenderHostPage();
					break;
				case 3:
					menu_pages::RenderDedigamerPage();
					break;
				case 4:
					menu_pages::RenderAboutPage();
					break;
				default:
					menu_pages::RenderMainPage();
					break;
			}

			ImGui::PopStyleVar();
			ImGui::Spacing();
			ImGui::EndChild();
		}

		ImGui::End();

	}
}

namespace {
	LRESULT CALLBACK WindowProcess(HWND currentWindow, UINT message, WPARAM wideParam, LPARAM longParam) {
		if (GetAsyncKeyState(VK_INSERT) & 1) {
			gui::open = !gui::open;
		}

		ImGuiIO& io = ImGui::GetIO();
		io.MouseDrawCursor = gui::open;

		if (gui::open && ImGui_ImplWin32_WndProcHandler(currentWindow, message, wideParam, longParam)) {
			return 1L;
		}

		if (gui::originalWindowProcess)
			return CallWindowProc(gui::originalWindowProcess, currentWindow, message, wideParam, longParam);
		return DefWindowProc(currentWindow, message, wideParam, longParam);
	}
}
