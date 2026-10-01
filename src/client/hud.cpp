#include "hud.hpp"

#include <algorithm>
#include <cmath>
#include <string>

void Hud::Load() {
    m_font = LoadFontEx("assets/fonts/Cinzel.ttf", 48, nullptr, 0);
    SetTextureFilter(m_font.texture, TEXTURE_FILTER_BILINEAR);
}

void Hud::Unload() {
	UnloadFont(m_font);
}

void Hud::SetScreenSize(const float width, const float height) {
	m_screenW = width;
	m_screenH = height;
}

void Hud::Draw(const HudView& view) const {
	const float panelW = 520.0f, panelH = 120.0f;
	const Rectangle panel = { (m_screenW - panelW) / 2.0f, m_screenH - panelH - 10.0f, panelW, panelH };
	DrawRectangleRounded(panel, 0.15f, 8, Fade(BLACK, 0.7f));

	drawStats({ panel.x + 10, panel.y + 10, 110, panel.height - 20 }, view.stats);

	const float slot = 56.0f, gap = 8.0f;
	float x = panel.x + 130;

	drawSlot({ x, panel.y + 10, slot * 0.7f, slot * 0.7f }, ' ', SlotView{});   // passive
	x += slot * 0.7f + gap;

	constexpr char keys[4] = { 'Q', 'W', 'E', 'R' };
	for (std::size_t i = 0; i < 4; i++) {
		drawSlot({ x, panel.y + 10, slot, slot }, keys[i], view.slots[i]);
		x += slot + gap;
	}

	const float barX = panel.x + 130.0f;
	const float barW = panel.width - 140.0f;

	const float healthY = panel.y + 74.0f;
	const float healthH = 18.0f;
	const float manaY = healthY + healthH + 4.0f;
	const float manaH = 14.0f;

	drawBar({ barX, healthY, barW, healthH }, view.health, view.maxHealth, GREEN);
	drawBar({ barX, manaY,   barW, manaH }, view.mana, view.maxMana, BLUE);
}

void Hud::drawBar(Rectangle r, float value, float max, Color c) const {
	const float fraction = (max > 0.0f) ? std::clamp(value / max, 0.0f, 1.0f) : 0.0f;
	DrawRectangleRec(r, DARKGRAY);
	DrawRectangleRec({ r.x, r.y, r.width * fraction, r.height }, c);
	DrawRectangleLinesEx(r, 1.0f, BLACK);

	const char* text = TextFormat("%.0f / %.0f", value, max);
	const float size = r.height - 4.0f;
	const Vector2 textSize = MeasureTextEx(m_font, text, size, 1.0f);
	DrawTextEx(m_font, text, { r.x + (r.width - textSize.x) / 2.0f, r.y + 2.0f }, size, 1.0f, WHITE);
}

void Hud::drawSlot(Rectangle r, char key, const SlotView& s) const {
	DrawRectangleRounded(r, 0.2f, 6, DARKGRAY);

	const Color tint = s.notEnoughMana ? Color{ 110, 110, 255, 255 } : WHITE;
	if (s.icon) {
		const Rectangle src = { 0, 0, static_cast<float>(s.icon->width), static_cast<float>(s.icon->height) };
		DrawTexturePro(*s.icon, src, r, { 0, 0 }, 0.0f, tint);
	}
	else if (!s.name.empty()) {
		const std::string shortName(s.name.substr(0, 3));
		const Vector2 size = MeasureTextEx(m_font, shortName.c_str(), 18, 1);
		DrawTextEx(m_font, shortName.c_str(), { r.x + (r.width - size.x) / 2, r.y + (r.height - size.y) / 2 }, 18, 1, tint);
	}

	if (s.cooldownLeft > 0.0f && s.cooldownTotal > 0.0f) {
		const float fraction = s.cooldownLeft / s.cooldownTotal;
		const Vector2 center = { r.x + r.width / 2, r.y + r.height / 2 };

		BeginScissorMode(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width), static_cast<int>(r.height));
		DrawCircleSector(center, r.width, -90.0f + 360.0f * (1.0f - fraction), 270.0f, 36, Fade(BLACK, 0.65f));
		EndScissorMode();

		const char* text = s.cooldownLeft >= 1.0f ? TextFormat("%.0f", std::ceil(s.cooldownLeft))
			: TextFormat("%.1f", s.cooldownLeft);
		const Vector2 size = MeasureTextEx(m_font, text, 22, 1);
		DrawTextEx(m_font, text, { center.x - size.x / 2, center.y - size.y / 2 }, 22, 1, WHITE);
	}

	DrawRectangleRoundedLinesEx(r, 0.2f, 6, 2.0f, GRAY);
	DrawTextEx(m_font, TextFormat("%c", key), { r.x + 4, r.y + 2 }, 14, 1, WHITE);
}

void Hud::drawStats(Rectangle r, const champion::Stats& s) const {
	const float size = 16.0f;
	float y = r.y;
	DrawTextEx(m_font, TextFormat("AD     %.0f", s.attackDamage), { r.x, y }, size, 1, WHITE); y += size + 4;
	DrawTextEx(m_font, TextFormat("Range  %.1f", s.attackRange), { r.x, y }, size, 1, WHITE); y += size + 4;
	DrawTextEx(m_font, TextFormat("Speed  %.1f", s.moveSpeed), { r.x, y }, size, 1, WHITE);
}
