#pragma once

#include <raylib.h>
#include "champions.hpp"

struct HudView {
	std::string_view championName;
	float health, maxHealth;
	float mana, maxMana;
	std::array<float, 4> cooldownLeft;
	std::array<float, 4> cooldownTotal;
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
	void drawSlot(Rectangle r, char key, float left, float total) const;
	void drawStats(Rectangle r, const champion::Stats& s) const;
	float m_screenW;
	float m_screenH;
	Font m_font;
};