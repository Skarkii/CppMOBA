#include "handlers.hpp"

#include <bitset>
#include <cmath>
#include <memory>
#include <print>

#include "ability.hpp"
#include "match.hpp"
#include "messages.hpp"
#include "player.hpp"
#include "projectiles.hpp"
#include "serialize.hpp"

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
				matchInfo(players[i].GetTeam());
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
					broadcastChatMessage(server, scope, msg, player);
				}
			}
		}
		break;
	case protocol::MessageType::CastAbility:
		{
			uint8_t slot{}, targetSlot{};
			float pointX{}, pointZ{};
			r(slot);
			r(targetSlot);
			r(pointX);
			r(pointZ);
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break; 
			}

			uint8_t casterSlot = playerCount;
			for(uint8_t i = 0; i < playerCount; i++) {
				if (players[i].IsConnected() && players[i].GetPeer() == peer) {
					casterSlot = i;
					break;
				}
			}

			if (casterSlot == playerCount || slot > 4)
				break;

			Player& caster = players[casterSlot];
			const AbilityDef* a = caster.GetAbility(slot);
			if (!caster.IsAlive() || !a || a->type != AbilityType::Skillshot)
				break;

			const Vector3 pos = caster.GetPosition();
			const float dx = pointX - pos.x, dz = pointZ - pos.z;
			const float len = std::sqrt(dx * dx + dz * dz);
			if (len < 0.001f || !caster.TryUseAbility(slot))
				break;

			const float fx = dx / len, fz = dz / len;
			auto alreadyHit = std::make_shared<std::bitset<10>>();
			const uint16_t firstId = nextProjectileId;

			for (int i = 0; i < a->count; i++) {
				const auto [dirX, dirZ] = SpreadDirection(*a, fx, fz, i);
				skillshots.push_back({ .id = nextProjectileId++, .position = { pos.x, 1.2f, pos.z },
					   .dirX = dirX, .dirZ = dirZ, .caster = casterSlot,
					   .ability = a, .alreadyHit = alreadyHit });
			}
			broadcastSkillshotCast(server, firstId, casterSlot, *a, pos.x, pos.z, fx, fz);
			notifyCooldown(server, caster, slot, a->cooldown);
		}
	break;

	default:
		break;
	}
}
