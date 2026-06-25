#include "brainrot.hpp"

#include <algorithm>
#include <cfloat>

#include "../../ext/imgui/imgui.h"
#include "../framework/settings.h"
#include "functions.hpp"

#include <mmsystem.h>
#include <cstdio>

#include "you_died_mp3.h" // defines: unsigned char YOUDIED[] - raw bytes of YOU DIED(HD).mp3

#pragma comment(lib, "winmm.lib")

namespace
{
	constexpr const char* kYouDiedText = "YOU DIED";
	constexpr float kBaseFontSize = 96.f;
	constexpr double kAnimDuration = 4.5;
	constexpr double kOverlayFadeInEnd = 1.2;
	constexpr double kTextAnimStart = 0.25;
	constexpr double kTextAnimEnd = 2.4;
	constexpr double kFadeOutStart = 3.1;

	float smoothstep(float t) noexcept
	{
		t = std::clamp(t, 0.f, 1.f);
		return t * t * (3.f - 2.f * t);
	}

	float easeOutCubic(float t) noexcept
	{
		t = std::clamp(t, 0.f, 1.f);
		const float inv = 1.f - t;
		return 1.f - inv * inv * inv;
	}

	// Drops the embedded mp3 to a temp file (once), then (re)starts it through MCI so
	// the stinger plays in sync with the overlay. Called on the killcam rising edge.
	void playYouDiedSound() noexcept
	{
		static int soundState = 0; // 0 = not tried, 1 = ready, -1 = unavailable

		if (soundState == 0)
		{
			soundState = -1;

			char tempDir[MAX_PATH]{};
			if (GetTempPathA(MAX_PATH, tempDir) != 0)
			{
				const std::string path = std::string(tempDir) + "dismay_youdied.mp3";

				FILE* file = nullptr;
				if (fopen_s(&file, path.c_str(), "wb") == 0 && file != nullptr)
				{
					fwrite(YOUDIED, 1, sizeof(YOUDIED), file);
					fclose(file);

					std::string openCmd = "open \"" + path + "\" type mpegvideo alias dismayYouDied";
					if (mciSendStringA(openCmd.c_str(), nullptr, 0, nullptr) == 0)
					{
						soundState = 1;
					}
					else
					{
						// Fall back to letting MCI pick the device by the .mp3 extension.
						openCmd = "open \"" + path + "\" alias dismayYouDied";
						if (mciSendStringA(openCmd.c_str(), nullptr, 0, nullptr) == 0)
							soundState = 1;
					}
				}
			}
		}

		if (soundState != 1)
		{
			return;
		}

		// Rewind so repeated deaths always restart the stinger from the top.
		mciSendStringA("seek dismayYouDied to start", nullptr, 0, nullptr);
		mciSendStringA("play dismayYouDied", nullptr, 0, nullptr);
	}
}

namespace brainrot
{
	void render() noexcept
	{
		if (!features::brainrot)
		{
			return;
		}

		// Fire the "YOU DIED" animation on the rising edge of the killcam flag - the
		// instant the game starts playing our killcam (i.e. we just got killed). The
		// killcam flag is only ever set during a live match, so it doubles as our
		// "are we actually in game" guard.
		static bool wasInKillcam = false;
		static double animStart = -1.0;

		const bool inKillcam = functions::isInKillcam();
		if (inKillcam && !wasInKillcam)
		{
			animStart = ImGui::GetTime();
			playYouDiedSound();
		}
		wasInKillcam = inKillcam;

		if (animStart < 0.0)
		{
			return;
		}

		const double elapsed = ImGui::GetTime() - animStart;
		if (elapsed < 0.0 || elapsed >= kAnimDuration)
		{
			animStart = -1.0;
			return;
		}

		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		if (drawList == nullptr)
		{
			return;
		}

		const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
		const ImVec2 screenMin(0.f, 0.f);
		const ImVec2 screenMax(displaySize.x, displaySize.y);

		float overlayAlpha = 0.f;
		if (elapsed < kOverlayFadeInEnd)
		{
			overlayAlpha = smoothstep(static_cast<float>(elapsed / kOverlayFadeInEnd));
		}
		else if (elapsed < kFadeOutStart)
		{
			overlayAlpha = 1.f;
		}
		else
		{
			const float fadeT = static_cast<float>((elapsed - kFadeOutStart) / (kAnimDuration - kFadeOutStart));
			overlayAlpha = 1.f - smoothstep(fadeT);
		}

		const int bgAlpha = static_cast<int>(overlayAlpha * 255.f);
		drawList->AddRectFilled(screenMin, screenMax, IM_COL32(0, 0, 0, bgAlpha));

		ImFont* font = fonts::optimus_princeps ? fonts::optimus_princeps : fonts::morpheus_title;
		if (font == nullptr)
		{
			font = ImGui::GetFont();
		}

		float textProgress = 0.f;
		if (elapsed >= kTextAnimStart)
		{
			textProgress = static_cast<float>(
				(elapsed - kTextAnimStart) / (kTextAnimEnd - kTextAnimStart));
			textProgress = std::clamp(textProgress, 0.f, 1.f);
		}

		const float zoom = 0.55f + easeOutCubic(textProgress) * 0.45f;
		const float textFade = smoothstep(textProgress);
		float textAlpha = textFade * overlayAlpha;
		if (elapsed >= kFadeOutStart)
		{
			const float fadeT = static_cast<float>((elapsed - kFadeOutStart) / (kAnimDuration - kFadeOutStart));
			textAlpha *= 1.f - smoothstep(fadeT);
		}

		const float fontSize = kBaseFontSize * zoom;
		const ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, kYouDiedText, nullptr);
		const ImVec2 textPos(
			(displaySize.x - textSize.x) * 0.5f,
			(displaySize.y - textSize.y) * 0.5f);

		const int textAlphaByte = static_cast<int>(std::clamp(textAlpha, 0.f, 1.f) * 255.f);
		const ImU32 textColor = IM_COL32(139, 0, 0, textAlphaByte);
		drawList->AddText(font, fontSize, textPos, textColor, kYouDiedText);
	}
}
