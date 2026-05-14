#include "pages.hpp"

#include "../../../ext/imgui/imgui_internal.h"

#include "../../game/iw4hooks.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include <Windows.h>
#include <shellapi.h>

namespace menu_pages {

namespace {

static const ImVec4 kDedigamerCellTextWhite(1.f, 1.f, 1.f, 1.f);

void DedigamerCellTextWhite(const char* const s) noexcept
{
	ksd::TableCellTextColored(kDedigamerCellTextWhite, s ? s : "");
}

} // namespace

void RenderDedigamerPage() noexcept
{
	dedigamer::g_tabOpen.store(true);

	{
		std::lock_guard<std::mutex> lock(dedigamer::g_state.mtx);
		if (dedigamer::g_state.reconnectPending &&
			GetTickCount() >= dedigamer::g_state.reconnectTriggerTick)
		{
			char urlBuf[512];
			std::snprintf(urlBuf, sizeof(urlBuf), "%s", dedigamer::g_state.lastJoinUrl.c_str());
			dedigamer::g_state.reconnectPending = false;
			if (urlBuf[0] != '\0')
			{
				ShellExecuteA(NULL, "open", urlBuf, NULL, NULL, SW_SHOWNORMAL);
			}
		}
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 3.f));
	ImGui::BeginGroup();
	{
		const ImVec2 origin(ImGui::GetCursorPos());
		const float leftX = origin.x - layout::leftPull;

		ImGui::SetCursorPos(ImVec2(leftX, origin.y));
		if (ksd::BeginChild(ICON_FA_GLOBE, "Server Browser", 420.f, 636))
		{
			ksd::Text("Dedigamer Servers:");
			ImGui::Separator();

			std::lock_guard<std::mutex> lock(dedigamer::g_state.mtx);

			const bool isFetching = dedigamer::g_state.fetching;
			const bool hasFetched = dedigamer::g_state.fetched;
			const int totalPlayers = dedigamer::g_state.totalPlayers;
			const int totalCapacity = dedigamer::g_state.totalCapacity;

			if (isFetching)
			{
				ksd::Text("Fetching...");
			}
			else if (!dedigamer::g_state.error.empty())
			{
				ksd::Text("Error: %s", dedigamer::g_state.error.c_str());
			}

			if (hasFetched && totalCapacity > 0)
			{
				const float pct = static_cast<float>(totalPlayers) / static_cast<float>(totalCapacity) * 100.f;
				ksd::Text("Total Players: %d / %d (%.1f%%)", totalPlayers, totalCapacity, static_cast<double>(pct));
			}
			else if (hasFetched)
			{
				ksd::Text("Total Players: %d / %d", totalPlayers, totalCapacity);
			}

			if (ksd::Button("Refresh", ImVec2(160.f, 28.f)))
			{
				dedigamer::g_state.lastFetchTick = 0;
			}
			ImGui::SameLine(0.f, 10.f);
			if (ksd::Button("Disconnect", ImVec2(160.f, 28.f)))
			{
				Cbuf_AddText(0, "disconnect");
			}
			ImGui::SameLine(0.f, 10.f);

			{
				bool hasLastJoin = !dedigamer::g_state.lastJoinUrl.empty();
				bool isPending = dedigamer::g_state.reconnectPending;

				if (!hasLastJoin || isPending)
				{
					ImGui::BeginDisabled();
					ksd::Button(isPending ? "Reconnecting..." : "Reconnect", ImVec2(160.f, 28.f));
					ImGui::EndDisabled();
					if (!hasLastJoin && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
						ImGui::SetTooltip("No recent join URL");
				}
				else
				{
					if (ksd::Button("Reconnect", ImVec2(160.f, 28.f)))
					{
						Cbuf_AddText(0, "disconnect");
						dedigamer::g_state.reconnectPending = true;
						dedigamer::g_state.reconnectTriggerTick = GetTickCount() + 1000;
					}
				}
			}

			static std::vector<int> order;
			const int srvCount = static_cast<int>(dedigamer::g_state.servers.size());

			if (hasFetched && srvCount > 0)
			{
				order.resize(srvCount);
				for (int i = 0; i < srvCount; i++) order[i] = i;

				const auto& srvs = dedigamer::g_state.servers;
				std::sort(order.begin(), order.end(), [&srvs](int a, int b) {
					const auto& sa = srvs[a];
					const auto& sb = srvs[b];
					const float ratioA = sa.totalPlayers > 0 ? static_cast<float>(sa.currentPlayers) / static_cast<float>(sa.totalPlayers) : 0.f;
					const float ratioB = sb.totalPlayers > 0 ? static_cast<float>(sb.currentPlayers) / static_cast<float>(sb.totalPlayers) : 0.f;
					if (ratioA != ratioB)
						return ratioA > ratioB;
					if (sa.currentPlayers != sb.currentPlayers)
						return sa.currentPlayers > sb.currentPlayers;
					return sa.name < sb.name;
				});

				ImGui::Separator();
				const float tableHeight = 300.f;
				const ImGuiStyle& tableStyle = ImGui::GetStyle();
				const float playersColW =
					ImGui::CalcTextSize("99/99").x + tableStyle.CellPadding.x * 2.f + 6.f;
				const float gametypeColW =
					ImGui::CalcTextSize("Ground War").x + tableStyle.CellPadding.x * 2.f + 4.f;
				const float uptimeColW =
					ImGui::CalcTextSize("99:99:99").x + tableStyle.CellPadding.x * 2.f + 4.f;
				const float mapColW =
					ImGui::CalcTextSize("mp_checkpoint").x + tableStyle.CellPadding.x * 2.f + 8.f;
				constexpr float kJoinColW = 96.f;
				constexpr float kJoinBtnW = 72.f;
				constexpr float kJoinBtnH = 30.f;
				constexpr float kJoinOffsetX = 16.f;
				constexpr float kRowMinH = 38.f;
				const float innerRowH = kRowMinH - tableStyle.CellPadding.y * 2.f;
				const ImVec4 headerClear(0.f, 0.f, 0.f, 0.f);

				if (ImGui::BeginTable(
						"DedigamerServers",
						6,
						ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
						ImVec2(0.f, tableHeight)))
				{
					ImGui::TableSetupColumn("Server", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("Players", ImGuiTableColumnFlags_WidthFixed, playersColW);
					ImGui::TableSetupColumn("Map", ImGuiTableColumnFlags_WidthFixed, mapColW);
					ImGui::TableSetupColumn("Gametype", ImGuiTableColumnFlags_WidthFixed, gametypeColW);
					ImGui::TableSetupColumn("Uptime", ImGuiTableColumnFlags_WidthFixed, uptimeColW);
					ImGui::TableSetupColumn("##Join", ImGuiTableColumnFlags_WidthFixed, kJoinColW);
					ImGui::TableHeadersRow();

					for (int oi = 0; oi < srvCount; ++oi)
					{
						const int si = order[oi];
						const DedigamerServer& srv = srvs[si];

						ImGui::TableNextRow(0, kRowMinH);
						const int rowIdx = ImGui::TableGetRowIndex();
						ImGui::TableSetColumnIndex(0);
						const float cellTopY = ImGui::GetCursorPosY();
						char selId[32];
						std::snprintf(selId, sizeof(selId), "##row%d", si);
						ImGui::PushStyleColor(ImGuiCol_Header, headerClear);
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered, headerClear);
						ImGui::PushStyleColor(ImGuiCol_HeaderActive, headerClear);
						ImGui::Selectable(
							selId,
							false,
							ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap,
							ImVec2(0.f, innerRowH));
						ImGui::PopStyleColor(3);
						bool rowHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
						rowHovered = rowHovered || (ImGui::TableGetHoveredRow() == rowIdx);
						ImGui::SameLine(0.f, 6.f);
						{
							const float textH = ImGui::GetTextLineHeight();
							const float nameY = cellTopY + ImMax(0.f, (innerRowH - textH) * 0.5f);
							ImGui::SetCursorPosY(nameY);
						}
						DedigamerCellTextWhite(srv.name.c_str());

						ImGui::TableSetColumnIndex(1);
						{
							const float textH = ImGui::GetTextLineHeight();
							const float padY = ImMax(0.f, (innerRowH - textH) * 0.5f);
							if (padY > 0.f)
								ImGui::Dummy(ImVec2(0.f, padY));
						}
						ksd::TableCellPlayers(srv.currentPlayers, srv.totalPlayers);

						ImGui::TableSetColumnIndex(2);
						{
							const float textH = ImGui::GetTextLineHeight();
							const float padY = ImMax(0.f, (innerRowH - textH) * 0.5f);
							if (padY > 0.f)
								ImGui::Dummy(ImVec2(0.f, padY));
						}
						DedigamerCellTextWhite(srv.map.c_str());

						ImGui::TableSetColumnIndex(3);
						{
							const float textH = ImGui::GetTextLineHeight();
							const float padY = ImMax(0.f, (innerRowH - textH) * 0.5f);
							if (padY > 0.f)
								ImGui::Dummy(ImVec2(0.f, padY));
						}
						DedigamerCellTextWhite(srv.gametype.c_str());

						ImGui::TableSetColumnIndex(4);
						{
							const float textH = ImGui::GetTextLineHeight();
							const float padY = ImMax(0.f, (innerRowH - textH) * 0.5f);
							if (padY > 0.f)
								ImGui::Dummy(ImVec2(0.f, padY));
						}
						DedigamerCellTextWhite(srv.uptime.c_str());

						ImGui::TableSetColumnIndex(5);
						{
							const float contentH = !srv.joinUrl.empty() ? kJoinBtnH : ImGui::GetTextLineHeight();
							const float padY = ImMax(0.f, (innerRowH - contentH) * 0.5f);
							if (padY > 0.f)
								ImGui::Dummy(ImVec2(0.f, padY));
						}
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + kJoinOffsetX);
						if (!srv.joinUrl.empty())
						{
							ImGui::PushID(si);
							if (ksd::Button("Join", ImVec2(kJoinBtnW, kJoinBtnH)))
							{
								dedigamer::g_state.lastJoinUrl = srv.joinUrl;
								ShellExecuteA(NULL, "open", srv.joinUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
							}
							if (ImGui::IsItemHovered())
								rowHovered = true;
							ImGui::PopID();
						}
						else
						{
							DedigamerCellTextWhite("\xe2\x80\x94");
						}

						ksd::TableRowHoverAccent(rowHovered, 0.1f);
					}
					ImGui::EndTable();
				}
			}
		}
		ksd::EndChild();
	}
	ImGui::EndGroup();
	ImGui::PopStyleVar();
}

} // namespace menu_pages
