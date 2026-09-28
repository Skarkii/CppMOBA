#include "game.hpp"

#include <print>
#include "serialize.hpp"
#include "protocol.hpp"

Game::Game() {
}

Game::~Game() {
}

void Game::Connect() {
	//m_client.connect(...), then m_client.send(...) and m_client.flush()
	if (!m_client.connect(m_hostServerName, m_hostServerPort)) {
		std::println("Failed to connect to server {}:{}", m_hostServerName, m_hostServerPort);
		// TODO: Handle failure to connect
	}
	std::println("Connected to server {}:{}", m_hostServerName, m_hostServerPort);
	m_connectionState = ConnectionState::Connecting;

	sendHello();
}

void Game::Prepare() {
}

void Game::WaitForServer() {
}

void Game::Run() {
	while (m_running) {
		updateNetwork();
	}
}

void Game::updateNetwork() {
	for (auto& event : m_client.poll())
		{
			std::visit(net::overloaded{
				[&]([[maybe_unused]] const net::Connected& e) {},

				[&]([[maybe_unused]] const net::Received& e) {
					std::println("received {} bytes from on channel {}",
						e.data.size(), static_cast<int>(e.channel));
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
		if (!r.done())
			break; 
		std::println("Welcome Retrieved");
		m_connectionState = ConnectionState::Joined;
		break;

	default:
		break;
	}
}

void Game::sendHello() {
	net::Writer w;
	const std::uint64_t token = 1001;

	w(protocol::MessageType::Hello);
	w(protocol::kVersion);
	w(token);

	m_client.send(w.buffer, net::Channel::Reliable);
	m_client.flush();
}

void Game::disconnect() {
}

