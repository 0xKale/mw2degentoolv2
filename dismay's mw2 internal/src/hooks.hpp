#pragma once
#include "menu/gui.hpp"
#include "framework/notify.hpp"
#include "dismay/functions.hpp"
#include "dismay/dedigamer/dedigamer.hpp"
#include <stdexcept>
#include <intrin.h>

#include "..//ext/minhook/minhook.h"

#include "../ext/imgui/imgui.h"
#include "../ext/imgui/imgui_impl_win32.h"
#include "../ext/imgui/imgui_impl_dx9.h"

namespace hooks
{
	inline void Setup();
	inline void Destroy() noexcept;

	// Helper to get Virtual Function address
	constexpr void* VirtualFunction(void* thisptr, size_t index) noexcept
	{
		return (*static_cast<void***>(thisptr))[index];
	}

	// EndScene
	using EndSceneFn = long(__stdcall*)(IDirect3DDevice9*) noexcept;
	inline EndSceneFn EndSceneOriginal = nullptr;
	inline long __stdcall EndScene(IDirect3DDevice9* device) noexcept;

	// Reset
	using ResetFn = HRESULT(__stdcall*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*) noexcept;
	inline ResetFn ResetOriginal = nullptr;
	inline HRESULT __stdcall Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept;
}

inline void hooks::Setup()
{
	if (MH_Initialize())
		throw std::runtime_error("Unable to initialize minhook");

	if (!gui::device)
		throw std::runtime_error("DirectX device not initialized");

	if (MH_CreateHook(
		VirtualFunction(gui::device, 42),
		&EndScene,
		reinterpret_cast<void**>(&EndSceneOriginal)
	)) throw std::runtime_error("Unable to hook EndScene()");

	if (MH_CreateHook(
		VirtualFunction(gui::device, 16),
		&Reset,
		reinterpret_cast<void**>(&ResetOriginal)
	)) throw std::runtime_error("Unable to hook Reset()");

	if (MH_EnableHook(MH_ALL_HOOKS))
		throw std::runtime_error("Unable to enable hooks");

	functions::startFeatureWorker();
	dedigamer::Init();
}

inline void hooks::Destroy() noexcept
{
	dedigamer::Shutdown();
	functions::stopFeatureWorker();
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

inline long __stdcall hooks::EndScene(IDirect3DDevice9* device) noexcept
{
	static const auto returnAddress = _ReturnAddress();

	const auto result = EndSceneOriginal(device);

	if (_ReturnAddress() == returnAddress) {
		return result;
	}

	const bool was_setup = gui::setup;
	if (!gui::setup)
		gui::SetupMenu(device);

	if (!gui::setup)
		return result;

	if (!was_setup && gui::setup)
		return result;

	if (!ImGui::GetCurrentContext())
		return result;

	ImGuiIO& io = ImGui::GetIO();
	if (io.BackendRendererUserData == nullptr)
	{
		if (!ImGui_ImplDX9_Init(device))
			return result;
	}
	if (io.BackendPlatformUserData == nullptr)
	{
		if (!ImGui_ImplWin32_Init(gui::window))
			return result;
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	functions::syncImGuiMouseDrawCursor();
	// ImGui draw lists must be filled on the render thread (here), not from FunctionWorkerLoop.
	functions::DrawCrosshairOverlay();

	notify::setupNotify();

	if (gui::open)
		gui::Render();

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	return result;
}

inline HRESULT __stdcall hooks::Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept
{
	if (!ResetOriginal || !device || !params) {
		return D3DERR_INVALIDCALL;
	}

	ImGui_ImplDX9_InvalidateDeviceObjects();

	const HRESULT result = ResetOriginal(device, params);

	if (SUCCEEDED(result)) {
		ImGui_ImplDX9_CreateDeviceObjects();
	}

	return result;
}