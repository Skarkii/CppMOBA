#pragma once
#include <cstdint>
#include "serialize.hpp"
#include "protocol.hpp"
#include "net.hpp"
#include "champions.hpp"
#include <span>

struct Vector3 {
	float x;
	float y;
	float z;
};

enum class ConnectionState { Disconnected, Connected};
enum class Order { Idle, MoveTo, Attack };

enum class Team : uint8_t { Blue, Red } ;

class Player {
public:
	Player(uint64_t token, champion::Id champId, Team team);
	~Player();

	[[nodiscard]]
	uint64_t GetToken() const;

	[[nodiscard]]
	Vector3 GetPosition() const;

	[[nodiscard]]
	champion::Id GetChampionId() const;

	void SetTargetPosition(const Vector3 targetPos);

	void SetAttackTarget(const std::uint8_t);

	void SetPeer(const net::PeerId id);

	float GetHealth() const;

	[[nodiscard]]
	net::PeerId GetPeer() const;

	[[nodiscard]]
	bool IsConnected() const;


	bool Update(const float dt, std::span<Player> players);

	Team GetTeam() const;

	void TakeDamage(float amount);

	uint8_t GetAttackTarget() const;
private:
	Vector3 m_pos = { 0 };
	Vector3 m_pos_goal = { 0 };

	float m_health;
	float m_maxHealth;

	float m_rotation = 0.0f;
	champion::Id m_champId;
	champion::Stats m_stats;
	uint64_t m_token;
	net::PeerId m_peer;
	Order m_order = Order::Idle;
	float m_attackCooldown = 0.0f;
	Team m_team;
	
	ConnectionState m_connected = ConnectionState::Disconnected;

	uint8_t m_attackTarget;

	bool moveTo(const Vector3 goal, const float dt);
	bool inRangeOfPlayer(Player other) const;
};
