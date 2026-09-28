#include "player.hpp"

#include <cmath>

Player::Player(uint64_t token, champion::Id champId) {
	m_token = token;

	m_champId = champId;
	m_stats = champion::Get(champId).stats;
}

Player::~Player() {

}

void Player::updatePosition(const float dt) {	
	const float dx = m_pos_goal.x - m_pos.x;
	const float dy = m_pos_goal.y - m_pos.y;
	const float distance = std::sqrt(dx * dx + dy * dy);

	const float step = m_stats.moveSpeed * dt;

	if (distance <= step) {
		m_pos = m_pos_goal;
		return;
	}

	m_pos.x += dx / distance * step;
	m_pos.y += dy / distance * step;
}

uint64_t Player::GetToken() const {
	return m_token;
}

Vector2 Player::GetPosition() const {
	return m_pos;
}

champion::Id Player::GetChampionId() const {
	return m_champId;
}

void Player::SetTargetPosition(const Vector2 targetPos) {
	m_pos_goal = targetPos;
}

void Player::SetPeer(const net::PeerId id) {
	m_peer = id;
	m_connected = ConnectionState::Connected;
}


net::PeerId Player::GetPeer() const {
	return m_peer;
}

bool Player::IsConnected() const {
	return m_connected == ConnectionState::Connected;
}

void Player::Update(const float dt) {
	updatePosition(dt);
}
