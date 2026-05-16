#include "crosshair_sharecode.h"
#include "../menu/pages/page_main.hpp"

#include <cstring>
#include <cstdint>

static const char DICTIONARY[] = "ABCDEFGHJKLMNOPQRSTUVWXYZabcdefhijkmnopqrstuvwxyz23456789";
static constexpr int DICT_LEN = 57;
static constexpr int PAYLOAD_CHARS = 25;
static constexpr int DECODED_BYTES = 18;

static int dictIndex(char c) noexcept
{
	for (int i = 0; i < DICT_LEN; ++i)
		if (DICTIONARY[i] == c) return i;
	return -1;
}

static void bignumMulAdd(uint8_t* buf, int len, int mul, int add) noexcept
{
	int carry = add;
	for (int i = len - 1; i >= 0; --i)
	{
		int v = buf[i] * mul + carry;
		buf[i] = static_cast<uint8_t>(v & 0xFF);
		carry = v >> 8;
	}
}

static int8_t uint8ToInt8(uint8_t v) noexcept
{
	return static_cast<int8_t>(v);
}

bool DecodeCrosshairShareCode(const char* shareCode, CsCrosshairDecoded& out) noexcept
{
	if (!shareCode) return false;

	char clean[64];
	int ci = 0;

	const char* p = shareCode;
	while (*p == ' ') ++p;

	if ((p[0] == 'C' || p[0] == 'c') &&
		(p[1] == 'S' || p[1] == 's') &&
		(p[2] == 'G' || p[2] == 'g') &&
		(p[3] == 'O' || p[3] == 'o'))
	{
		p += 4;
		if (*p == '-') ++p;
	}

	for (; *p && ci < 63; ++p)
	{
		if (*p == '-' || *p == ' ') continue;
		clean[ci++] = *p;
	}
	clean[ci] = '\0';

	if (ci != PAYLOAD_CHARS) return false;

	for (int i = 0; i < ci; ++i)
		if (dictIndex(clean[i]) < 0) return false;

	// Reverse
	for (int i = 0; i < ci / 2; ++i)
	{
		char tmp = clean[i];
		clean[i] = clean[ci - 1 - i];
		clean[ci - 1 - i] = tmp;
	}

	uint8_t buf[DECODED_BYTES] = {};

	for (int i = 0; i < PAYLOAD_CHARS; ++i)
	{
		int idx = dictIndex(clean[i]);
		bignumMulAdd(buf, DECODED_BYTES, DICT_LEN, idx);
	}

	// Checksum: buf[0] == sum(buf[1..]) & 0xFF
	int sum = 0;
	for (int i = 1; i < DECODED_BYTES; ++i)
		sum += buf[i];
	if (buf[0] != static_cast<uint8_t>(sum & 0xFF))
		return false;

	out.gap            = uint8ToInt8(buf[2]) / 10.f;
	out.outlineThickness = buf[3] / 2.f;
	out.red            = buf[4];
	out.green          = buf[5];
	out.blue           = buf[6];
	out.alpha          = buf[7];
	out.thickness      = buf[12] / 10.f;
	out.length         = buf[14] / 10.f;

	out.colorPreset       = buf[10] & 7;
	out.outlineEnabled    = (buf[10] & 8) == 8;
	out.centerDotEnabled  = ((buf[13] >> 4) & 1) == 1;
	out.alphaEnabled      = ((buf[13] >> 4) & 4) == 4;
	out.tStyleEnabled     = ((buf[13] >> 4) & 8) == 8;

	return true;
}

bool ApplyCsCrosshairToVars(const char* shareCode) noexcept
{
	CsCrosshairDecoded cs{};
	if (!DecodeCrosshairShareCode(shareCode, cs))
		return false;

	float r, g, b;
	switch (cs.colorPreset)
	{
	case 0: r = 1.f;   g = 0.f;   b = 0.f;   break; // Red
	case 1: r = 0.f;   g = 1.f;   b = 0.f;   break; // Green
	case 2: r = 1.f;   g = 1.f;   b = 0.f;   break; // Yellow
	case 3: r = 0.f;   g = 0.f;   b = 1.f;   break; // Blue
	case 4: r = 0.f;   g = 1.f;   b = 1.f;   break; // Cyan
	default: r = cs.red / 255.f; g = cs.green / 255.f; b = cs.blue / 255.f; break; // Custom
	}

	float a = cs.alphaEnabled ? (cs.alpha / 255.f) : 1.f;
	vars::crosshair_color = ImVec4(r, g, b, a);
	vars::crosshairOutline = cs.outlineEnabled;
	vars::crosshairGap = cs.gap;
	vars::crosshairLength = cs.length;
	vars::crosshairThickness = cs.thickness;
	vars::crosshairOutlineThickness = cs.outlineThickness;
	vars::crosshairCenterDot = cs.centerDotEnabled;
	vars::crosshairTStyle = cs.tStyleEnabled;
	vars::enableCrosshair = true;

	return true;
}
