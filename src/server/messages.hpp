#pragma once

#include <cstdint>
#include <string_view>

#include "ability.hpp"
#include "net.hpp"
#include "player.hpp"
#include "protocol.hpp"

void broadcastState(net::Server& server, std::uint32_t tick);
void broadcastChatMessage(net::Server& server, protocol::TextScope scope, std::string_view msg, Player& sender);
void broadCastProjectileSpawn(net::Server& server, uint8_t casterSlot, uint8_t targetSlot, const AbilityDef& ability);
void broadcastProjectileEnd(net::Server& server, uint16_t id);
void broadcastSkillshotCast(net::Server& server, uint16_t firstId, uint8_t casterSlot, const AbilityDef& a, float x, float z, float fx, float fz);
void notifyCooldown(net::Server& server, Player& p, uint8_t slot, float cd);
void sendPlayerGold(net::Server& server, Player& p);
void broadcastPlayerGold(net::Server& server); 
void broadcastKill(net::Server& server, Player& attacker, Player& deadPlayer);
