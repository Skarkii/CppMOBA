#pragma once

#include <array>
#include <cstdint>

#include "net.hpp"
#include "player.hpp"

const uint8_t playerCount = 2;
extern std::array<Player, playerCount> players;

void updatePlayers(net::Server& server, const float kTickSeconds, MapGrid& mapGrid);
void applyHit(net::Server& server, const AbilityDef& ability, Player& caster, Player& target);
void playerKilled(net::Server& server, Player& attacker, Player& deadPlayer);

[[nodiscard]]
std::uint8_t slotOf(const Player& p);
