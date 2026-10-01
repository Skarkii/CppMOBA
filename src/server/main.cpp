// server-main.cpp

#include "net.hpp"

#include <cmath>
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
std::array<Player, playerCount> players{ 
	Player(100, "Abc", champion::Id::Barbarian, Team::Blue), 
	Player(101, "def", champion::Id::Ranger, Team::Red),
	/*
	Player(102, "ghi", champion::Id::Rogue, Team::Blue),
	Player(103, "jkl", champion::Id::Mage, Team::Red),
	*/
};

struct Projectile {
	Vector3 position;
	std::uint8_t caster;
	std::uint8_t target;
	float speed;
	const AbilityDef* ability;
	bool done = false;
};

std::vector<Projectile> projectiles;

void broadcastState(net::Server& server, std::uint32_t tick) {
	net::Writer w;
	w(protocol::MessageType::PlayerState);
	w(tick);
	for (Player& player : players) {
		Vector3 pos = player.GetPosition();
		w(pos.x);
		w(pos.y);
		w(pos.z);
		w(player.GetHealth());
		w(player.GetMana());
	}
	server.broadcast(w.buffer, net::Channel::Unreliable);
}

void broadcastMessage(net::Server& server, protocol::TextScope scope, std::string_view msg, Player& sender) {
	net::Writer w;
	w(protocol::MessageType::ChatMessage);
	w(scope);
	w(sender.GetChampionId());
	w.writeString(msg);
	w.writeString(sender.GetName());

	if (scope == protocol::TextScope::All) {
		server.broadcast(w.buffer, net::Channel::Reliable);
		return;
	}

	for (Player& p : players) {
		if (p.GetTeam() == sender.GetTeam() && p.IsConnected()) {
			server.send(p.GetPeer(), w.buffer, net::Channel::Reliable);
		}
	}
}

void broadCastProjectileSpawn(net::Server& server, uint8_t casterSlot, uint8_t targetSlot, const AbilityDef& ability) {
	net::Writer w;
	w(protocol::MessageType::ProjectileSpawn);
	w(casterSlot);
	w(targetSlot);
	w.writeString(ability.id);
	server.broadcast(w.buffer, net::Channel::Unreliable);
}

int main(int argc, char** argv)
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

	bool requireAllPlayers = false;
	bool started = true;

	if (argc > 1 && std::string_view(argv[1]) == "-REQUIREPLAYERS") {
		requireAllPlayers = true;
		started = false;
	}

	sol::state lua;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
	lua.new_usertype<Player>("Player", sol::no_constructor,
		"damage", &Player::TakeDamage,
		"attackDamage", &Player::AttackDamage);

	AbilityLibrary abilities(lua);
	if (!abilities.LoadDirectory("assets/abilities"))
		std::println("Warning: some abilities failed to load");

	for (Player& p : players) {
		const auto* ability = abilities.Find(champion::Get(p.GetChampionId()).basicAttack);
		if (!ability) {
			std::println("Missing basic attack for {}", champion::Get(p.GetChampionId()).name);
			return EXIT_FAILURE;
		}
		p.SetBasicAttack(ability);
	}


	using clock = std::chrono::steady_clock;

	constexpr auto kTick = protocol::kTick;
	constexpr float kTickSeconds = protocol::kTickSeconds;

	std::uint32_t tick = 0;

	auto next = clock::now();

	uint32_t maximumTickWaitTimeForPlayers = 30 * 10;

	while (true) {
		tick += 1;

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

		if (!started && requireAllPlayers) {
			bool shouldStart = true;
			for (uint8_t i = 0; i < playerCount; i++) {
				if (!players[i].IsConnected()) {
					shouldStart = false;
				}
				
			}
			if (shouldStart || tick > maximumTickWaitTimeForPlayers)
				started = true;

			next += kTick;
			std::this_thread::sleep_until(next);
			continue;
		}


		for (std::uint8_t i = 0; i < playerCount; i++) {
			if (players[i].Update(kTickSeconds, players)) {
				Player& p = players[i];
				Player& target = players[p.GetAttackTarget()];
				const AbilityDef& ability = *p.GetBasicAttack();

				switch (ability.type) {
				case AbilityType::TargetedProjectile:
					projectiles.push_back({
						.position = { p.GetPosition().x, 1.2f, p.GetPosition().z },
						.caster = i,
						.target = p.GetAttackTarget(),
						.speed = ability.speed,
						.ability = &ability,
						});
					broadCastProjectileSpawn(server, i, p.GetAttackTarget(), ability);
					break;
				case AbilityType::Targeted:
					CallHook(ability, ability.onHit, &p, &target);
					break;
				default:
					break;
				}

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
				if (target.IsAlive())
					CallHook(*proj.ability, proj.ability->onHit, &players[proj.caster], &target);
				proj.done = true;
				continue;
			}

			proj.position.x += dx / distance * step;
			proj.position.z += dz / distance * step;
		}

		std::erase_if(projectiles, [](const Projectile& p) { return p.done; });

		broadcastState(server, tick);

		next += kTick;
		std::this_thread::sleep_until(next);
	}

	return EXIT_SUCCESS;
}

void OnHandshakeFailed(net::Server& server, protocol::MessageType type) {
	std::println("Handshake failed: {}", protocol::ToString(type));
	net::Writer w;
	w(type);
	server.broadcast(w.buffer, net::Channel::Reliable);
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

			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				OnHandshakeFailed(server, protocol::MessageType::DeclineInvalidMessage);
				break; 
			}

			if (kver != protocol::kVersion) {
				OnHandshakeFailed(server, protocol::MessageType::DeclineProtocolVersion);
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
				OnHandshakeFailed(server, protocol::MessageType::DeclineToken);
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
			for (uint8_t i = 0; i < amountOfPlayers; i++) {
				matchInfo(players[i].GetChampionId());
				matchInfo.writeString(players[i].GetName());
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
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break; 
			}
			for (Player& player : players) {
				if (player.IsConnected() && player.GetPeer() == peer && player.IsAlive()) {
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
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break; 
			}
			for (Player& player : players) {
				if (player.IsConnected() && player.GetPeer() == peer && player.IsAlive()) {
					if (targetId >= playerCount || player.GetTeam() == players[targetId].GetTeam())
						break;
					player.SetAttackTarget(targetId);
					break;
				}
			}
		}
		break;
	case protocol::MessageType::RecallCommand:
		{
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break; 
			}
			for (Player& player : players) {
				if (player.IsConnected() && player.GetPeer() == peer && player.IsAlive()) {
					player.Recall();
				}
			}
		}
		break;
	case protocol::MessageType::ChatSend:
		{
			protocol::TextScope scope;
			std::string msg;
			r(scope);
			r.readString(msg, protocol::maximumChatLength);
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break; 
			}
			for (Player& player : players) {
				if (player.IsConnected() && player.GetPeer() == peer) {
					broadcastMessage(server, scope, msg, player);
				}
			}
		}
		break;

	default:
		break;
	}
}

