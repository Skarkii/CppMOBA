#pragma once

#include <raylib.h>
#include "champions.hpp"
#include <array>
#include <string_view>

struct SlotView {
	const Texture2D* icon = nullptr;
	std::string_view name;
	float cooldownLeft = 0.0f;
	float cooldownTotal = 0.0f;
	bool notEnoughMana = false;
};

struct HudView {
	std::string_view championName;
	float health, maxHealth;
	float mana, maxMana;
	std::array<SlotView, 4> slots;
	champion::Stats stats;
};

class Hud {
public:
	void Load();
	void Unload();
	void SetScreenSize(const float width, const float height);
	void Draw(const HudView& view) const;
private:
	void drawBar(Rectangle r, float value, float max, Color c) const;
	void drawSlot(Rectangle r, char key, const SlotView& s) const;
	void drawStats(Rectangle r, const champion::Stats& s) const;
	float m_screenW;
	float m_screenH;
	Font m_font;
};