// server-main.cpp

#include "net.hpp"

#include <cmath>
#include <print>
#include <thread>
#include <chrono>
#include <array>

#include "serialize.hpp"
#include <memory>
#include <bitset>
#include "protocol.hpp"

#include "player.hpp"

#include "champions.hpp"

#include "handlers.hpp"
#include "match.hpp"
#include "messages.hpp"
#include "projectiles.hpp"

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
		const auto* basicAbility = abilities.Find(champion::Get(p.GetChampionId()).basicAttack);
		if (!basicAbility) {
			std::println("Missing basic attack for {}", champion::Get(p.GetChampionId()).name);
			return EXIT_FAILURE;
		}
		p.SetBasicAttack(basicAbility);

		const champion::Definition& def = champion::Get(p.GetChampionId());

		for (std::uint8_t slot = 0; slot < def.abilities.size(); slot++) {
			const std::string_view id = def.abilities[slot];
			if (id.empty())
				continue;

			const AbilityDef* ability = abilities.Find(id);
			if (!ability) {
				std::println("{}: ability '{}' (slot {}) not found", def.name, id, slot);
				return EXIT_FAILURE;
			}
			p.SetAbility(slot, ability);
		}
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


		updatePlayers(server, kTickSeconds);
		updateProjectiles(server, kTickSeconds);

		broadcastState(server, tick);

		if (tick % protocol::kTicksPerSecond == 0) {
			broadcastPlayerGold(server);
		}

		next += kTick;
		std::this_thread::sleep_until(next);
	}

	return EXIT_SUCCESS;
}
