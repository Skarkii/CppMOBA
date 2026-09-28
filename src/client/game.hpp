// game.hpp

#pragma once

#include "net.hpp"
#include <raylib.h>
#include "player.hpp"
#include <array>

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

	void updateNetwork();
	void onMessage(const std::vector<std::byte>& data);
	void sendHello();
	void disconnect();

	void moveCommand(const Vector2 pos);
	Vector2 m_position = { 0 };

	std::array<Player, 10> m_players = { };
	int m_playerCount;

	uint8_t m_playerId;
};