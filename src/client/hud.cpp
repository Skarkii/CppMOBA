#include "hud.hpp"

#include <algorithm>

void Hud::Load() {
    m_font = LoadFontEx("assets/fonts/Phantom.otf", 48, nullptr, 0);
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
	drawSlot({ x, panel.y + 10, slot * 0.7f, slot * 0.7f }, ' ', 0, 0);
	x += slot * 0.7f + gap;

	constexpr char keys[4] = { 'Q', 'W', 'E', 'R' };
	for (int i = 0; i < 4; i++) {
		drawSlot({ x, panel.y + 10, slot, slot }, keys[i], view.cooldownLeft[i], view.cooldownTotal[i]);
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

void Hud::drawSlot(Rectangle r, char key, float left, float total) const {
    DrawRectangleRounded(r, 0.2f, 6, DARKGRAY);
    if (left > 0.0f && total > 0.0f) {
        const float fraction = left / total;
        DrawRectangleRec({ r.x, r.y, r.width, r.height * fraction }, Fade(BLACK, 0.6f));
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
