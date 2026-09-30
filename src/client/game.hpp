// game.hpp

#pragma once

#include "champions.hpp"
#include "net.hpp"
#include "player.hpp"
#include <array>
#include <raylib.h>
#include <unordered_map>
#include <optional>

enum class ConnectionState { Disconnected, Connecting, Joined };

class Game {
public:
	Game(uint64_t token);
	~Game();
	void Connect();
	void Prepare();
	void WaitForServer();
	void Run();
private:
	uint64_t m_token;
	const std::string m_hostServerName = "127.0.0.1";
	const uint16_t m_hostServerPort = 8000;
	bool m_running = true;

	net::Library m_netLibrary;
	net::Client  m_client;
	ConnectionState m_connectionState = ConnectionState::Disconnected;
	bool m_retrievedMatchInfo = false;

	void updateNetwork();
	void onMessage(const std::vector<std::byte>& data);
	void sendHello();
	void disconnect();

	void moveCommand(const Vector3 pos);
	void attackCommand(std::uint8_t targetId);
	std::optional<Vector3> mouseToGround() const;
	Vector3 m_position = { 0 };
	float m_zoom;
	Camera m_camera;

	std::array<Player, 10> m_players = { };
	int m_playerCount;

	uint8_t m_playerId;

	void loadModels();
	std::unordered_map<champion::Id, Model> m_models;

	std::optional<std::uint8_t> enemyUnderMouse() const;
};