#include "game.hpp"

#include <print>
#include "serialize.hpp"
#include "protocol.hpp"


Game::Game(uint64_t token) {
	InitWindow(1280, 720, "MOBA");
	m_token = token;
}

Game::~Game() {
	CloseWindow();
}

void Game::Connect() {
	//m_client.connect(...), then m_client.send(...) and m_client.flush()
	if (!m_client.connect(m_hostServerName, m_hostServerPort)) {
		std::println("Failed to connect to server {}:{}", m_hostServerName, m_hostServerPort);
		// TODO: Handle failure to connect
	}
	std::println("Connecting to server {}:{}", m_hostServerName, m_hostServerPort);
	m_connectionState = ConnectionState::Connecting;

	sendHello();

	while (m_connectionState != ConnectionState::Joined || !m_retrievedMatchInfo) {
		updateNetwork();
	}

	std::println("Connected and retrieved match info");
}

void Game::Prepare() {
}

void Game::WaitForServer() {
}

void Game::Run() {
	while (m_running && !WindowShouldClose()) {
		updateNetwork();

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			Vector2 mousePos = GetMousePosition();
			moveCommand(mousePos);
		}

		BeginDrawing();
		ClearBackground(RAYWHITE);
		for (uint8_t i = 0; i < m_playerCount; i++) {
			m_players[i].Draw();
		}
		DrawFPS(5, 5);
		EndDrawing();
	}
}

void Game::updateNetwork() {
	for (auto& event : m_client.poll())
		{
			std::visit(net::overloaded{
				[&]([[maybe_unused]] const net::Connected& e) {},

				[&]([[maybe_unused]] const net::Received& e) {
					//std::println("received {} bytes from on channel {}", e.data.size(), static_cast<int>(e.channel));
					onMessage(e.data);
				},

				[&]([[maybe_unused]] const net::Disconnected& e) {
					std::println("server disconnected client");
					m_client.disconnect();
				},

				}, event);
		}

	m_client.flush();
}

void Game::onMessage(const std::vector<std::byte>& data) {
	/*
	for (std::byte b : data)
		std::print("{:02X} ", static_cast<unsigned>(b));
	std::println("");
	*/

	net::Reader r(data);

	protocol::MessageType type{};
	r(type);
	if (r.failed)
		return;

	switch (type)
	{
	case protocol::MessageType::Welcome:
		{
			uint8_t playerId;
			r(playerId);
			if (!r.done())
				break; 
			m_playerId = playerId;
			//std::println("Handshake completed, playerID: {}", playerId);
			m_connectionState = ConnectionState::Joined;
		}
		break;
	case protocol::MessageType::MatchInfo:
		{
			if (m_retrievedMatchInfo)
				break;

			uint8_t playerCount;
			r(playerCount);
			std::array<champion::Id, 10> champIds{};
			for (uint8_t i = 0; i < playerCount; i++) {
				r(champIds[i]);
			}
			if (!r.done())
				break; 
			for (uint8_t i = 0; i < playerCount; i++) {
				m_players[i].SetChampId(champIds[i]);
			}
			m_playerCount = playerCount;
			m_retrievedMatchInfo = true;
		}
		break;
	case protocol::MessageType::PlayerState:
		{
			std::array<Vector2, 10> positions{};
			for (uint8_t i = 0; i < m_playerCount; i++) {
				Vector2 pos;
				r(pos.x);
				r(pos.y);
				positions[i] = pos;
			}
			if (!r.done())
				break;
			for (uint8_t i = 0; i < m_playerCount; i++) {
				m_players[i].SetPosition(positions[i]);
			}
		}
		break;

	default:
		break;
	}
}

void Game::sendHello() {
	net::Writer w;

	w(protocol::MessageType::Hello);
	w(protocol::kVersion);
	w(m_token);

	m_client.send(w.buffer, net::Channel::Reliable);
	m_client.flush();
}

void Game::disconnect() {
}

void Game::moveCommand(const Vector2 pos) {
	net::Writer w;
	w(protocol::MessageType::MoveCommand);
	w(pos.x);
	w(pos.y);
	m_client.send(w.buffer, net::Channel::Reliable);
	m_client.flush();
}


