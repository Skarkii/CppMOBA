#pragma once

#include <array>
#include <cstdint>

#include "net.hpp"
#include "player.hpp"

const uint8_t playerCount = 2;
extern std::array<Player, playerCount> players;

void updatePlayers(net::Server& server, const float kTickSeconds);
