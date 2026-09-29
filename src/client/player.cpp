#include "player.hpp"

#include <print>

Player::Player()
{
}

Player::~Player()
{
}

void Player::Draw() {
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
}

void Player::SetPosition(Vector2 pos)
{
	m_position = pos;
}

void Player::SetChampId(champion::Id id) {
	m_champId = id;
	//std::println("Champion Set : {}", GetChampName());
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
