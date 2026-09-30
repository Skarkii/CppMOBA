#include "player.hpp"

#include <print>
#include <cmath>
#include <algorithm>

Player::Player() {
	m_scale = { 1, 1, 1 };

	m_health = m_maxHealth = 0.0f; 
}

Player::~Player()
{
}

void Player::Draw() {
	/*
	DrawRectangleV(m_position, { 32, 32 }, RED);

	const char* name = GetChampName().data();
	constexpr int fontSize = 10;
	const int textWidth = MeasureText(name, fontSize);

	const int x = static_cast<int>(m_position.x) + 16 - textWidth / 2;
	const int y = static_cast<int>(m_position.y) + 34;

	Color color = RED;
	if (m_self)
		color = GREEN;
	DrawText(name, x, y, fontSize, color);
	*/

	if (m_model) {
		// DrawModel(*m_model, m_position, 1.0f, WHITE);
		DrawModelEx(*m_model, m_position, { 0.0f, 1.0f, 0.0f }, m_rotation, m_scale, WHITE);
	}
}

void Player::DrawOverlay(Camera& cam) const {
	const Vector3 head = { m_position.x, m_position.y + 2.8f, m_position.z };
	const Vector2 screen = GetWorldToScreen(head, cam);

	constexpr float barWidth = 60.0f;
	constexpr float barHeight = 7.0f;
	const float barX = screen.x - barWidth / 2.0f;
	const float barY = screen.y;

	const float fraction = std::clamp(m_health  / m_maxHealth, 0.0f, 1.0f);
	const Color fill = m_self ? GREEN : RED;

	DrawRectangleRec({ barX, barY, barWidth, barHeight }, DARKGRAY);
	DrawRectangleRec({ barX, barY, barWidth * fraction, barHeight }, fill);
	DrawRectangleLinesEx({ barX, barY, barWidth, barHeight }, 1.0f, BLACK);

	const char* name = GetChampName().data();
	constexpr int fontSize = 14;
	const int textWidth = MeasureText(name, fontSize);
	DrawText(name, static_cast<int>(screen.x) - textWidth / 2,
		static_cast<int>(barY) - fontSize - 2, fontSize, BLACK);
}

void Player::SetPosition(const Vector3 pos) {
	const float dx = pos.x - m_position.x;
	const float dz = pos.z - m_position.z;

	if (dx * dx + dz * dz > 0.0001f)
		m_rotation = std::atan2(dx, dz) * RAD2DEG;

	m_position = pos;
} 

Vector3 Player::GetPosition() const {
	return m_position;
}

void Player::SetChampId(champion::Id id) {
	m_champId = id;
	//std::println("Champion Set : {}", GetChampName());
	m_health = m_maxHealth = champion::GetStats(id).maxHealth;
}

champion::Id Player::GetChampId() const {
	return m_champId;
}

std::string_view Player::GetChampName() const {
	return champion::Get(m_champId).name;
}

bool Player::IsSelf() const {
	return m_self;
}

void Player::SetSelf() {
	m_self = true;
}

void Player::SetModel(const Model* model) {
	m_model = model;
}

void Player::SetHealth(const float health) {
	m_health = health;
}
