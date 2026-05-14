#pragma once

#include <Windows.h>
#include <d3d9.h>
#include <string>

namespace gui {
	extern std::string MenuTitle;
	extern float menu_title_draw_alpha;

	extern bool open;
	extern bool setup;

	extern HWND window;
	extern WNDPROC originalWindowProcess;

	extern LPDIRECT3DDEVICE9 device;
	extern LPDIRECT3D9 d3d9;

	void Setup();
	void FinalizeBootstrapSetup() noexcept;

	void Destroy() noexcept;
	void TearDownHackHostWindow() noexcept;
	void UpdateMenuTitle();
	void SetupMenu(LPDIRECT3DDEVICE9 currentDevice) noexcept;
	void Render() noexcept;
	void DestroyDirectX() noexcept;
}
