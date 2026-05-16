#pragma once

struct CsCrosshairDecoded {
	float gap;
	float length;
	float thickness;
	float outlineThickness;
	int red, green, blue, alpha;
	int colorPreset; // 0=Red,1=Green,2=Yellow,3=Blue,4=Cyan,5=Custom
	bool alphaEnabled;
	bool outlineEnabled;
	bool centerDotEnabled;
	bool tStyleEnabled;
};

bool DecodeCrosshairShareCode(const char* shareCode, CsCrosshairDecoded& out) noexcept;
bool ApplyCsCrosshairToVars(const char* shareCode) noexcept;
