#include "player.hpp"

#include <print>

Player::Player()
{
}

Player::~Player()
{
}

void Player::Draw()
{
	DrawRectangleV(m_position, { 32, 32 }, RED);
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
