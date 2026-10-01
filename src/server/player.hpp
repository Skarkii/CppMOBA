#pragma once
#include <cstdint>
#include "serialize.hpp"
#include "protocol.hpp"
#include "net.hpp"
#include "champions.hpp"
#include <span>
#include "ability.hpp"

struct Vector3 {
	float x;
	float y;
	float z;
};

enum class ConnectionState { Disconnected, Connected};
enum class Order { Idle, MoveTo, Attack, Recalling, Dead };

enum class Team : uint8_t { Blue, Red } ;

class Player {
public:
	Player(uint64_t token, std::string name, champion::Id champId, Team team);
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

	[[nodiscard]]
	float GetHealth() const;

	[[nodiscard]]
	float GetMana() const;

	[[nodiscard]]
	net::PeerId GetPeer() const;

	[[nodiscard]]
	bool IsConnected() const;

	[[nodiscard]]
	bool Update(const float dt, std::span<Player> players);

	[[nodiscard]]
	Team GetTeam() const;

	void TakeDamage(float amount);

	[[nodiscard]]
	uint8_t GetAttackTarget() const;

	void SetForcePosition(const Vector3 pos);

	void Recall();

	[[nodiscard]]
	std::string_view GetName() const;

	[[nodiscard]]
	bool IsAlive() const;

	void SetBasicAttack(const AbilityDef* ability) { m_basicAttack = ability; }
	const AbilityDef* GetBasicAttack() const { return m_basicAttack; }
	float AttackDamage() const { return m_stats.attackDamage; }

	void SetAbility(uint8_t slot, const AbilityDef* ability) { m_abilities[slot] = ability; };

	[[nodiscard]]
	const AbilityDef* GetAbility(uint8_t slot) const { return m_abilities[slot]; }

	[[nodiscard]]
	float GetCollisionRadius() const { return m_stats.collisionRadius; }

	[[nodiscard]]
	bool TryUseAbility(uint8_t slot);

private:
	std::string m_name;
	Vector3 m_pos = { 0 };
	Vector3 m_pos_goal = { 0 };

	float m_health;
	float m_maxHealth;

	float m_mana;
	float m_maxMana;

	float m_rotation = 0.0f;
	champion::Id m_champId;
	champion::Stats m_stats;
	uint64_t m_token;
	net::PeerId m_peer;
	Order m_order = Order::Idle;
	float m_attackCooldown = 0.0f;
	float m_recallTimer = 3.0f;
	float m_respawnTimer = 3.0f;
	Team m_team;
	
	ConnectionState m_connected = ConnectionState::Disconnected;

	uint8_t m_attackTarget;

	bool moveTo(const Vector3 goal, const float dt);

	[[nodiscard]]
	bool inRangeOfPlayer(const Player& other) const;

	const AbilityDef* m_basicAttack = nullptr;
	std::array<const AbilityDef*, 4> m_abilities{};
	std::array<float, 4> m_cooldowns{};
};
