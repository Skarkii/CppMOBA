#include "player.hpp"

#include <cmath>
#include <print>

Player::Player(uint64_t token, champion::Id champId, Team team) {
	m_token = token;

	m_champId = champId;
	m_stats = champion::Get(champId).stats;

	m_team = team;

	m_health = m_maxHealth = m_stats.maxHealth;
}

Player::~Player() {

}

bool Player::moveTo(const Vector3 goal, const float dt) {	
	const float dx = goal.x - m_pos.x;
	const float dz = goal.z - m_pos.z;
	const float distance = std::sqrt(dx * dx + dz * dz);

	const float step = m_stats.moveSpeed * dt;

	if (distance <= step) {
		m_pos.x = goal.x;
		m_pos.z = goal.z;
		return true;
	}

	m_pos.x += dx / distance * step;
	m_pos.z += dz / distance * step;

	return false;
}

uint64_t Player::GetToken() const {
	return m_token;
}

Vector3 Player::GetPosition() const {
	return m_pos;
}

champion::Id Player::GetChampionId() const {
	return m_champId;
}

void Player::SetTargetPosition(const Vector3 targetPos) {
	m_pos_goal = targetPos;
	m_order = Order::MoveTo;
}

void Player::SetAttackTarget(const std::uint8_t target) {
	m_attackTarget = target;
	m_order = Order::Attack;
}

void Player::SetPeer(const net::PeerId id) {
	m_peer = id;
	m_connected = ConnectionState::Connected;
}

float Player::GetHealth() const {
	return m_health;
}


net::PeerId Player::GetPeer() const {
	return m_peer;
}

bool Player::IsConnected() const {
	return m_connected == ConnectionState::Connected;
}

bool Player::inRangeOfPlayer(Player other) const {
	Vector3 otherPos = other.GetPosition();
	const float dx = otherPos.x - m_pos.x;
	const float dz = otherPos.z - m_pos.z;

	return dx * dx + dz * dz <= m_stats.attackRange * m_stats.attackRange;
}

bool Player::Update(const float dt, std::span<Player> players) {
	bool attacked = false;
	m_attackCooldown = std::max(0.0f, m_attackCooldown - dt);

	switch (m_order) {
	case Order::Idle:
		break;
	case Order::MoveTo:
		if (moveTo(m_pos_goal, dt))
			m_order = Order::Idle;
		break;
	case Order::Attack:
		if (inRangeOfPlayer(players[m_attackTarget]) && m_attackCooldown == 0.0f) {
			m_attackCooldown = m_stats.attackSpeed;
			attacked = true;
			m_order = Order::Idle;
		}
		else {
			moveTo(players[m_attackTarget].GetPosition(), dt);
		}
		break;
	}
	return attacked;
}

Team Player::GetTeam() const {
	return m_team;
}

void Player::TakeDamage(float amount) {
	m_health = std::max(0.0f, m_health - amount);
	std::println("Player {} health: {}", champion::Get(m_champId).name, m_health);
}

uint8_t Player::GetAttackTarget() const {
	return m_attackTarget;
}
