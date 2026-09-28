// game.hpp

#pragma once

#include "net.hpp"

enum class ConnectionState { Disconnected, Connecting, Joined };

class Game {
public:
	Game();
	~Game();
	void Connect();
	void Prepare();
	void WaitForServer();
	void Run();
private:
	bool m_running = true;

	net::Library m_netLibrary;
	net::Client  m_client;
	ConnectionState m_connectionState = ConnectionState::Disconnected;

	const std::string m_hostServerName = "127.0.0.1";
	const uint16_t m_hostServerPort = 8000;

	void updateNetwork();
	void onMessage(const std::vector<std::byte>& data);
	void sendHello();
	void disconnect();
};