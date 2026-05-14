#pragma once

#include <cstdint>

#include "../../ext/imgui/imgui.h"

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace ksd {

ImU32 ColorWithAlpha(const ImVec4& color, float alpha) noexcept;

void DrawRectShadow(
	ImDrawList* drawList,
	const ImVec2& min,
	const ImVec2& max,
	ImU32 shadowColor,
	float thickness,
	const ImVec2& offset,
	ImDrawFlags cornerFlags,
	float rounding) noexcept;

void DrawCircleShadow(
	ImDrawList* drawList,
	const ImVec2& center,
	float radius,
	ImU32 shadowColor,
	float thickness,
	const ImVec2& offset) noexcept;


bool D3D9MemoryGif_Load(IDirect3DDevice9* device, const void* imageBytes, std::uint32_t imageSize) noexcept;
void D3D9MemoryGif_Unload() noexcept;
void D3D9MemoryGif_TickImGui() noexcept;
IDirect3DTexture9* D3D9MemoryGif_CurrentTexture() noexcept;
std::uint32_t D3D9MemoryGif_PixelWidth() noexcept;
std::uint32_t D3D9MemoryGif_PixelHeight() noexcept;
void D3D9MemoryGif_DrawInPill(
	ImDrawList* drawList,
	const ImVec2& pillPos,
	float pillW,
	float pillH,
	float tintAlpha01,
	float fullBleedRounding) noexcept;

}
