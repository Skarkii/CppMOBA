#include "player.hpp"

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
