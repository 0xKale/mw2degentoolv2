#define IMGUI_DEFINE_MATH_OPERATORS
#include "notify.hpp"

#include "../../ext/imgui/imgui_internal.h"
#include "settings.h"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

namespace {

struct Entry {
	int id;
	std::string text;
	float time;
	float alpha;
	bool active;
	float timer;
	float pos;
};

constexpr float padX = 20.f;
constexpr float padY = 20.f;
constexpr float spacing = 8.f;
constexpr float itemPadding = 8.f;
constexpr float itemHeight = 20.f;
constexpr float notifyFontSize = 20.f;
constexpr float stroke = 1.f;
constexpr float linePad = 8.f;
constexpr float lineHeight = 2.f;

std::vector<Entry> entries;
int nextId = 0;
constexpr float animSpeed = 4.f;
constexpr float posSpeed = 8.f;
constexpr float fadeOutSpeed = 6.f;

static ImFont* pickNotifyFont()
{
	if (fonts::inter_font)
		return fonts::inter_font;
	if (fonts::inter_bold_font2)
		return fonts::inter_bold_font2;
	return ImGui::GetFont();
}

static ImVec2 renderOne(int id, float alpha, float timer, float pos, const std::string& text, float time)
{
	ImFont* const useFont = pickNotifyFont();
	const float fontSize = notifyFontSize;
	const ImVec2 textSize = useFont->CalcTextSizeA(fontSize, FLT_MAX, 0.f, text.c_str());
	const float winW = textSize.x + itemPadding * 2.f;
	const float bodyH = (std::max)(itemHeight, textSize.y);
	const float winH = bodyH + linePad + lineHeight;

	ImGui::SetNextWindowPos(ImVec2(padX, pos));
	ImGui::SetNextWindowSize(ImVec2(winW, winH));
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, stroke);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.12f, 0.14f, alpha));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.22f, 0.22f, 0.24f, alpha));

	char nameBuf[32];
	std::snprintf(nameBuf, sizeof(nameBuf), "notify_%d", id);
	ImGui::Begin(
		nameBuf,
		nullptr,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs);

	const ImVec2 winPos = ImGui::GetWindowPos();
	const ImVec2 winSize = ImGui::GetWindowSize();
	ImDrawList* const drawList = ImGui::GetWindowDrawList();

	const float progress = (time > 0.f) ? ImClamp(timer / time, 0.f, 1.f) : 1.f;
	const float lineW = (winSize.x - linePad * 2.f) * progress;
	const float barY = winSize.y - linePad - lineHeight;

	const ImVec4 acc = colors::accent_color;
	const ImVec4 barCol(acc.x, acc.y, acc.z, acc.w * alpha);
	drawList->PushClipRect(
		ImVec2(winPos.x + linePad, winPos.y),
		ImVec2(winPos.x + winSize.x - linePad, winPos.y + winSize.y));
	drawList->AddRectFilled(
		ImVec2(winPos.x + linePad, winPos.y + barY),
		ImVec2(winPos.x + winSize.x - linePad - lineW, winPos.y + winSize.y - linePad),
		ImGui::ColorConvertFloat4ToU32(barCol),
		4.f);
	drawList->PopClipRect();

	const float textY = winPos.y + (bodyH - textSize.y) * 0.5f;
	const ImVec2 textPos(winPos.x + itemPadding, textY);
	const ImU32 textCol = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, alpha));
	drawList->AddText(useFont, fontSize, textPos, textCol, text.c_str(), text.c_str() + text.size());

	const ImVec2 outSize = ImGui::GetWindowSize();
	ImGui::End();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);

	return outSize;
}

}

void notify::addNotify(std::string_view text, float time)
{
	entries.push_back({ nextId++, std::string(text), time, 0.f, true, 0.f, 0.f });
}

void notify::setupNotify()
{
	float delta = ImGui::GetIO().DeltaTime;
	if (delta <= 0.f)
		delta = 0.016f;
	if (delta > 0.1f)
		delta = 0.1f;

	float accumulatedY = 0.f;

	for (auto& entry : entries) {
		if (entry.active)
			entry.timer += delta;

		if (entry.timer >= entry.time)
			entry.active = false;

		if (entry.active)
			entry.alpha = ImLerp(entry.alpha, 1.f, animSpeed * delta);
		else {
			entry.alpha -= fadeOutSpeed * delta;
			if (entry.alpha < 0.f)
				entry.alpha = 0.f;
		}

		if (entry.alpha > 0.02f) {
			const float targetPos = accumulatedY + padY;
			entry.pos = ImLerp(entry.pos, targetPos, posSpeed * delta);

			const ImVec2 size = renderOne(entry.id, entry.alpha, entry.timer, entry.pos, entry.text, entry.time);
			accumulatedY += size.y + spacing;
		}
	}

	entries.erase(
		std::remove_if(
			entries.begin(),
			entries.end(),
			[](const Entry& e) { return !e.active && e.alpha <= 0.02f; }),
		entries.end());
}

void SendNotify(const char* text, float time)
{
	if (text)
		notify::addNotify(std::string_view(text), time);
}
