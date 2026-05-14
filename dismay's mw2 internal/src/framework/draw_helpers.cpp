#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "draw_helpers.hpp"

#include <Windows.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

#include <d3d9.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../../ext/stb/stb_image.h"

#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"

namespace {

struct D3D9MemoryGifState {
	std::vector<IDirect3DTexture9*> frames;
	std::vector<UINT> delay_ms;
	double accum_ms = 0.0;
	size_t frame_index = 0;
	UINT pixel_w = 0;
	UINT pixel_h = 0;
};

static D3D9MemoryGifState g_d3d9MemoryGif;

void D3D9MemoryGifUnloadImpl() noexcept
{
	for (IDirect3DTexture9* const t : g_d3d9MemoryGif.frames)
	{
		if (t)
		{
			t->Release();
		}
	}
	g_d3d9MemoryGif.frames.clear();
	g_d3d9MemoryGif.delay_ms.clear();
	g_d3d9MemoryGif.accum_ms = 0.0;
	g_d3d9MemoryGif.frame_index = 0;
	g_d3d9MemoryGif.pixel_w = 0;
	g_d3d9MemoryGif.pixel_h = 0;
}

IDirect3DTexture9* CreateTextureFromRgba(
	IDirect3DDevice9* const dev,
	const UINT w,
	const UINT h,
	const BYTE* const rgba,
	const UINT srcStride) noexcept
{
	if (!dev || !rgba || w == 0 || h == 0)
	{
		return nullptr;
	}

	IDirect3DTexture9* tex = nullptr;
	HRESULT hr = dev->CreateTexture(w, h, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &tex, nullptr);
	if (FAILED(hr) || !tex)
	{
		return nullptr;
	}

	D3DLOCKED_RECT lr{};
	hr = tex->LockRect(0, &lr, nullptr, 0);
	if (FAILED(hr))
	{
		tex->Release();
		return nullptr;
	}

	for (UINT y = 0; y < h; ++y)
	{
		BYTE* const dst = static_cast<BYTE*>(lr.pBits) + static_cast<size_t>(y) * static_cast<size_t>(lr.Pitch);
		const BYTE* const src = rgba + static_cast<size_t>(y) * static_cast<size_t>(srcStride);
		for (UINT x = 0; x < w; ++x)
		{
			const BYTE* const s = src + x * 4u;
			BYTE* const d = dst + x * 4u;
			d[0] = s[2];
			d[1] = s[1];
			d[2] = s[0];
			d[3] = s[3];
		}
	}

	tex->UnlockRect(0);
	return tex;
}

}

ImU32 ksd::ColorWithAlpha(const ImVec4& color, const float alpha) noexcept
{
	ImVec4 result = color;
	result.w *= alpha;
	return ImGui::GetColorU32(result);
}

void ksd::DrawRectShadow(
	ImDrawList* const drawList,
	const ImVec2& min,
	const ImVec2& max,
	const ImU32 shadowColor,
	const float thickness,
	const ImVec2& offset,
	const ImDrawFlags cornerFlags,
	const float rounding) noexcept
{
	if (!drawList)
	{
		return;
	}

	if ((shadowColor & IM_COL32_A_MASK) == 0)
	{
		return;
	}

	const ImVec2 coreMin = min + offset;
	const ImVec2 coreMax = max + offset;

	const ImVec4 baseColor = ImGui::ColorConvertU32ToFloat4(shadowColor);
	const float baseAlpha = ImSaturate(baseColor.w);

	const float thick = ImMax(thickness, 1.f);

	const bool wantsRoundedOutline =
		rounding > 0.f &&
		(cornerFlags & ImDrawFlags_RoundCornersMask_) != ImDrawFlags_RoundCornersNone;

	if (wantsRoundedOutline)
	{
		constexpr int rings = 7;

		for (int i = rings; i >= 1; --i)
		{
			const float expand = thick * (float)i / (float)rings;

			const ImVec2 rmin(coreMin.x - expand, coreMin.y - expand);
			const ImVec2 rmax(coreMax.x + expand, coreMax.y + expand);

			const float w = rmax.x - rmin.x;
			const float h = rmax.y - rmin.y;
			if (w <= 1.f || h <= 1.f)
			{
				continue;
			}

			const float maxCorner = ImMax(0.5f * ImMin(w, h) - 1.f, 0.f);
			const float ringRound = ImMin(rounding + expand * 0.55f, maxCorner);

			const float emphasis = (float)(rings - i + 1) / (float)rings;
			ImVec4 ringColor = baseColor;
			ringColor.w = ImClamp(baseAlpha * 0.20f * ImPow(emphasis, 1.12f), 0.f, 0.24f);

			const ImU32 packed = ImGui::GetColorU32(ringColor);
			const float lineThickness = 1.55f + 0.25f * emphasis;

			drawList->AddRect(rmin, rmax, packed, ringRound, lineThickness, cornerFlags);
		}

		return;
	}

	constexpr int rings = 9;

	for (int i = rings; i >= 1; --i)
	{
		const float outerExpand = thick * (float)i / (float)rings;
		const float innerExpand = thick * (float)(i - 1) / (float)rings;

		const float emphasis = (float)(rings - i + 1) / (float)rings;
		ImVec4 ringColor = baseColor;
		ringColor.w = ImClamp(baseAlpha * 0.14f * ImPow(emphasis, 1.25f), 0.f, 0.24f);

		const ImU32 packed = ImGui::GetColorU32(ringColor);

		drawList->AddRectFilled(
			ImVec2(coreMin.x - outerExpand, coreMin.y - outerExpand),
			ImVec2(coreMax.x + outerExpand, coreMin.y - innerExpand),
			packed);

		drawList->AddRectFilled(
			ImVec2(coreMin.x - outerExpand, coreMax.y + innerExpand),
			ImVec2(coreMax.x + outerExpand, coreMax.y + outerExpand),
			packed);

		drawList->AddRectFilled(
			ImVec2(coreMin.x - outerExpand, coreMin.y - innerExpand),
			ImVec2(coreMin.x - innerExpand, coreMax.y + innerExpand),
			packed);

		drawList->AddRectFilled(
			ImVec2(coreMax.x + innerExpand, coreMin.y - innerExpand),
			ImVec2(coreMax.x + outerExpand, coreMax.y + innerExpand),
			packed);
	}
}

void ksd::DrawCircleShadow(
	ImDrawList* const drawList,
	const ImVec2& center,
	const float radius,
	const ImU32 shadowColor,
	const float thickness,
	const ImVec2& offset) noexcept
{
	if (!drawList)
	{
		return;
	}

	if ((shadowColor & IM_COL32_A_MASK) == 0)
	{
		return;
	}

	const ImVec2 pos = center + offset;

	const ImVec4 baseColor = ImGui::ColorConvertU32ToFloat4(shadowColor);
	const float baseAlpha = ImSaturate(baseColor.w);

	const float thick = ImMax(thickness, 5.f);
	const int layers = 14;

	for (int i = layers; i >= 1; --i)
	{
		const float haloRadius = radius + thick * (float)i / (float)layers;

		const float emphasis = (float)(layers - i + 1) / (float)layers;

		ImVec4 haloColor = baseColor;
		haloColor.w = ImClamp(baseAlpha * 0.085f * ImPow(emphasis, 1.3f), 0.f, 0.24f);

		drawList->AddCircleFilled(pos, haloRadius, ImGui::GetColorU32(haloColor));
	}
}

bool ksd::D3D9MemoryGif_Load(IDirect3DDevice9* const dev, const void* const imageBytes, const std::uint32_t imageSize) noexcept
{
	D3D9MemoryGifUnloadImpl();

	if (!dev || !imageBytes || imageSize == 0u)
	{
		return false;
	}
	if (imageSize > static_cast<std::uint32_t>(INT_MAX))
	{
		return false;
	}

	const int len = static_cast<int>(imageSize);
	const auto* const buf = static_cast<const stbi_uc*>(imageBytes);

	int w = 0;
	int h = 0;
	int z = 0;
	int comp = 0;
	int* stbDelays = nullptr;

	stbi_uc* data = stbi_load_gif_from_memory(buf, len, &stbDelays, &w, &h, &z, &comp, 4);
	if (!data)
	{
		data = stbi_load_from_memory(buf, len, &w, &h, &comp, 4);
		z = data ? 1 : 0;
	}

	if (!data || w <= 0 || h <= 0 || z <= 0 || w > 8192 || h > 8192)
	{
		if (data)
		{
			stbi_image_free(data);
		}
		if (stbDelays)
		{
			stbi_image_free(stbDelays);
		}
		return false;
	}

	UINT frameCount = static_cast<UINT>(z);
	if (frameCount > 512u)
	{
		frameCount = 512u;
	}

	const size_t frameBytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 4u;

	for (UINT fi = 0; fi < frameCount; ++fi)
	{
		const stbi_uc* const frameRgba = data + static_cast<size_t>(fi) * frameBytes;

		UINT delayMs = 100u;
		if (stbDelays && static_cast<int>(fi) < z)
		{
			const int d = stbDelays[fi];
			if (d > 0)
			{
				delayMs = static_cast<UINT>((std::max)(20, (std::min)(10000, d)));
			}
		}

		const UINT uw = static_cast<UINT>(w);
		const UINT uh = static_cast<UINT>(h);
		IDirect3DTexture9* const tex = CreateTextureFromRgba(dev, uw, uh, frameRgba, uw * 4u);
		if (!tex)
		{
			D3D9MemoryGifUnloadImpl();
			stbi_image_free(data);
			if (stbDelays)
			{
				stbi_image_free(stbDelays);
			}
			return false;
		}

		g_d3d9MemoryGif.frames.push_back(tex);
		g_d3d9MemoryGif.delay_ms.push_back(delayMs);
		if (g_d3d9MemoryGif.pixel_w == 0u && g_d3d9MemoryGif.pixel_h == 0u)
		{
			g_d3d9MemoryGif.pixel_w = uw;
			g_d3d9MemoryGif.pixel_h = uh;
		}
	}

	stbi_image_free(data);
	if (stbDelays)
	{
		stbi_image_free(stbDelays);
	}

	if (g_d3d9MemoryGif.delay_ms.size() != g_d3d9MemoryGif.frames.size())
	{
		D3D9MemoryGifUnloadImpl();
		return false;
	}

	return !g_d3d9MemoryGif.frames.empty();
}

void ksd::D3D9MemoryGif_Unload() noexcept
{
	D3D9MemoryGifUnloadImpl();
}

void ksd::D3D9MemoryGif_TickImGui() noexcept
{
	const size_t n = g_d3d9MemoryGif.frames.size();
	if (n <= 1 || g_d3d9MemoryGif.delay_ms.size() != n)
	{
		return;
	}

	g_d3d9MemoryGif.accum_ms += static_cast<double>(ImGui::GetIO().DeltaTime) * 1000.0;
	for (;;)
	{
		const UINT d = g_d3d9MemoryGif.delay_ms[g_d3d9MemoryGif.frame_index];
		if (g_d3d9MemoryGif.accum_ms < static_cast<double>(d))
		{
			break;
		}
		g_d3d9MemoryGif.accum_ms -= static_cast<double>(d);
		g_d3d9MemoryGif.frame_index = (g_d3d9MemoryGif.frame_index + 1u) % n;
	}
}

IDirect3DTexture9* ksd::D3D9MemoryGif_CurrentTexture() noexcept
{
	if (g_d3d9MemoryGif.frames.empty())
	{
		return nullptr;
	}
	return g_d3d9MemoryGif.frames[g_d3d9MemoryGif.frame_index % g_d3d9MemoryGif.frames.size()];
}

std::uint32_t ksd::D3D9MemoryGif_PixelWidth() noexcept
{
	return static_cast<std::uint32_t>(g_d3d9MemoryGif.pixel_w);
}

std::uint32_t ksd::D3D9MemoryGif_PixelHeight() noexcept
{
	return static_cast<std::uint32_t>(g_d3d9MemoryGif.pixel_h);
}

void ksd::D3D9MemoryGif_DrawInPill(
	ImDrawList* const drawList,
	const ImVec2& pillPos,
	const float pillW,
	const float pillH,
	const float tintAlpha01,
	const float fullBleedRounding) noexcept
{
	if (!drawList)
	{
		return;
	}

	IDirect3DTexture9* const tex = ksd::D3D9MemoryGif_CurrentTexture();
	if (!tex)
	{
		return;
	}

	const ImU32 bgImageTint = IM_COL32(255, 255, 255, static_cast<int>(255.f * ImClamp(tintAlpha01, 0.f, 1.f)));

	if (g_d3d9MemoryGif.pixel_w > 0u && g_d3d9MemoryGif.pixel_h > 0u)
	{
		const float iw = static_cast<float>(g_d3d9MemoryGif.pixel_w);
		const float ih = static_cast<float>(g_d3d9MemoryGif.pixel_h);
		const float scale = (std::min)(pillW / iw, pillH / ih);
		const float dw = iw * scale;
		const float dh = ih * scale;
		const float x0 = pillPos.x + (pillW - dw) * 0.5f;
		const float y0 = pillPos.y + (pillH - dh) * 0.5f;
		const float imgRound = (std::min)(fullBleedRounding, 0.12f * (std::min)(dw, dh));
		drawList->AddImageRounded(
			ImTextureRef(tex),
			ImVec2(x0, y0),
			ImVec2(x0 + dw, y0 + dh),
			ImVec2(0.f, 0.f),
			ImVec2(1.f, 1.f),
			bgImageTint,
			imgRound);
	}
	else
	{
		const ImVec2 bgMin(pillPos.x, pillPos.y);
		const ImVec2 bgMax(pillPos.x + pillW, pillPos.y + pillH);
		drawList->AddImageRounded(
			ImTextureRef(tex),
			bgMin,
			bgMax,
			ImVec2(0.f, 0.f),
			ImVec2(1.f, 1.f),
			bgImageTint,
			fullBleedRounding);
	}
}
