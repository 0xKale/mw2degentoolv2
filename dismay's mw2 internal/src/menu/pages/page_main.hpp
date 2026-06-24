#pragma once

#include "../../../ext/imgui/imgui.h"

namespace vars {

	extern bool enableTextChat;
	extern bool enableMouseOneToOne;
	extern bool highPollingMouseFix;

	extern int killstreakSlot1;
	extern int killstreakSlot2;
	extern int killstreakSlot3;
	extern bool ironSightIntervention;

	extern bool noSun;
	extern bool drawCamo;
	extern bool drawFog;
	extern bool drawBullets;
	extern bool movieMode;
	extern bool clearGlass;
	extern bool pingText;

	extern float mouseSensitivity;

	extern int framesPerSecond;
	extern float fieldOfView;
	extern float mapSize;
	extern float defaultFovMin;

	extern int selectedFullbrightMode;

	extern const char* fullbrightModes[5];

	extern char console[256];

	extern bool enableDLC;

	extern bool enableCustomPort;
	extern int customPort;

	extern int fullbright;
	extern int lightmap;

	extern bool enableCrosshair;
	extern ImVec4 crosshair_color;
	extern bool crosshairOutline;
	extern float crosshairGap;
	extern float crosshairLength;
	extern float crosshairThickness;
	extern float crosshairOutlineThickness;
	extern float crosshairScale;
	extern float crosshairLengthScale;
	extern float crosshairGapScale;
	extern bool crosshairCenterDot;
	extern bool crosshairTStyle;
	extern char csShareCodeInput[48];

	extern float fcg_gun_x;
	extern float fcg_gun_y;
	extern float fcg_gun_z;
}
