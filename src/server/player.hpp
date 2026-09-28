#pragma once
#include <cstdint>
#include "serialize.hpp"
#include "protocol.hpp"
#include "net.hpp"
#include "champions.hpp"

struct Vector2 {
	float x;
	float y;
};

enum class ConnectionState { Disconnected, Connected};

class Player {
public:
	Player(uint64_t token, champion::Id champId);
	~Player();

	[[nodiscard]]
	uint64_t GetToken() const;

	[[nodiscard]]
	Vector2 GetPosition() const;

	[[nodiscard]]
	champion::Id GetChampionId() const;

	void SetTargetPosition(const Vector2 targetPos);

	void SetPeer(const net::PeerId id);

	[[nodiscard]]
	net::PeerId GetPeer() const;

	[[nodiscard]]
	bool IsConnected() const;

	void Update(const float dt);
private:
	Vector2 m_pos = { 0 };
	Vector2 m_pos_goal = { 0 };
	champion::Id m_champId;
	champion::Stats m_stats;
	uint64_t m_token;
	net::PeerId m_peer;
	ConnectionState m_connected = ConnectionState::Disconnected;

	void updatePosition(const float dt);
};
