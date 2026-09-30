// server-main.cpp

#include "net.hpp"

#include <print>
#include <thread>
#include <chrono>
#include <array>

#include "serialize.hpp"
#include "protocol.hpp"

#include "player.hpp"

#include "champions.hpp"

void onMessage(const std::vector<std::byte>& data, net::Server& server, const net::PeerId peer);

const uint8_t playerCount = 2;
std::array<Player, playerCount> players{ Player(100, champion::Id::Barbarian, Team::Blue), Player(101, champion::Id::Ranger, Team::Red) };

struct Projectile {
	Vector3 position;
	std::uint8_t caster;
	std::uint8_t target;
	float speed;
	float damage;
	bool done = false;
};

std::vector<Projectile> projectiles;

void broadcastState(net::Server& server) {
	net::Writer w;
	w(protocol::MessageType::PlayerState);
	for (Player& player : players) {
		Vector3 pos = player.GetPosition();
		w(pos.x);
		w(pos.y);
		w(pos.z);
		w(player.GetHealth());
	}
	server.broadcast(w.buffer, net::Channel::Unreliable);
}

int main()
{
	net::Library lib;

	if (!lib) {
		std::println("Net Library failed to start");
		return EXIT_FAILURE;
	}

	net::Server server;

	uint16_t port = 8000;


	if (!server.start(port, 10)) {
		std::println("Failed to listen at port {}", port);
		return EXIT_FAILURE;
	}
	std::println("Listening on port {}", port);

	using clock = std::chrono::steady_clock;

	constexpr auto kTick = protocol::kTick;
	constexpr float kTickSeconds = protocol::kTickSeconds;

	auto next = clock::now();

	while (true) {

		for (auto& event : server.poll())
		{
			std::visit(net::overloaded{
				[&]([[maybe_unused]] const net::Connected& e) {
					// std::println("peer {} connected", e.peer);
				},
				[&](const net::Received& e) {
					// std::println("received {} bytes from peer {} on channel {}",e.data.size(), e.peer, static_cast<int>(e.channel));
					// e.peer, e.channel, e.data
					onMessage(e.data, server, e.peer);
				},
				[&](const net::Disconnected& e) {
					std::println("peer {} disconnected", e.peer);
				},
				}, event);
		}

		for (std::uint8_t i = 0; i < playerCount; i++) {
			if (players[i].Update(kTickSeconds, players)) {
				const Player& p = players[i];
				projectiles.push_back({
					.position = { p.GetPosition().x, 1.2f, p.GetPosition().z },
					.caster = i,
					.target = p.GetAttackTarget(),
					.speed = 15.0f,
					.damage = 10
					});
			}
		}

		for (Projectile& proj : projectiles) {
			Player& target = players[proj.target];
			const Vector3 goal = { target.GetPosition().x, 1.2f, target.GetPosition().z };

			const float dx = goal.x - proj.position.x;
			const float dz = goal.z - proj.position.z;
			const float distance = std::sqrt(dx * dx + dz * dz);

			const float step = proj.speed * kTickSeconds;

			if (distance <= step) {
				target.TakeDamage(proj.damage);
				proj.done = true;
			}

			proj.position.x += dx / distance * step;
			proj.position.z += dz / distance * step;
		}

		std::erase_if(projectiles, [](const Projectile& p) { return p.done; });

		broadcastState(server);

		next += kTick;
		std::this_thread::sleep_until(next);
	}

	return EXIT_SUCCESS;
}

void onMessage(const std::vector<std::byte>& data, net::Server& server, const net::PeerId peer) {
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
	case protocol::MessageType::Hello:
		{
			uint16_t kver;
			uint64_t token;
			r(kver);
			r(token);
			if (!r.done())
				break; 

			if (kver != protocol::kVersion) {
				server.disconnect(peer);
				std::println("Player Connected with wrong protol version");
				break;
			}

			uint8_t amountOfPlayers = playerCount;
			uint8_t playerId = playerCount;
			for (uint8_t i = 0; i < playerCount; i++) {
				if (players[i].GetToken() == token) {
					playerId = i;
					break;
				}
			}

			if (playerId >= playerCount) {
				server.disconnect(peer);
				std::println("Player is not in this game");
				break;
			}

			players[playerId].SetPeer(peer);

			net::Writer welcome;
			welcome(protocol::MessageType::Welcome);
			welcome(playerId);
			net::Writer matchInfo;
			matchInfo(protocol::MessageType::MatchInfo);
			matchInfo(amountOfPlayers);
			for (Player p : players) {
				matchInfo(p.GetChampionId());
			}
			server.send(peer, welcome.buffer, net::Channel::Reliable);
			server.send(peer, matchInfo.buffer, net::Channel::Reliable);
			std::println("Player {} connected", playerId);
		}
		break;
	case protocol::MessageType::MoveCommand:
		{
			// std::println("Moving player");
			Vector3 newPos{};
			r(newPos.x);
			r(newPos.y);
			r(newPos.z);
			if (!r.done()) {
				std::println("MoveCommand invalid message");
				break;
			}
			for (Player& player : players) {
				if (player.IsConnected() && player.GetPeer() == peer) {
					player.SetTargetPosition(newPos);
					break;
				}
			}
		}
		break;
	case protocol::MessageType::AttackCommand:
		{
			uint8_t targetId;
			r(targetId);
			if (!r.done()) {
				std::println("AttackCommand invalid message");
				break;
			}
			for (Player& player : players) {
				if (player.IsConnected() && player.GetPeer() == peer) {
					if (targetId >= playerCount || player.GetTeam() == players[targetId].GetTeam())
						break;
					player.SetAttackTarget(targetId);
					break;
				}
			}
		}
		break;

	default:
		break;
	}
}

