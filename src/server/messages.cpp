#include "messages.hpp"

#include "match.hpp"
#include "serialize.hpp"

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

void broadcastChatMessage(net::Server& server, protocol::TextScope scope, std::string_view msg, Player& sender) {
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
	server.broadcast(w.buffer, net::Channel::Reliable);
}

void broadcastProjectileEnd(net::Server& server, uint16_t id) {
	net::Writer w;
	w(protocol::MessageType::ProjectileEnd);
	w(id);
	server.broadcast(w.buffer, net::Channel::Reliable);
}

void broadcastSkillshotCast(net::Server& server, uint16_t firstId, uint8_t casterSlot, const AbilityDef& a, float x, float z, float fx, float fz) {
	net::Writer w;
	w(protocol::MessageType::SkillshotCast);
	w(firstId);
	w(casterSlot);
	w.writeString(a.id);
	w(x);
	w(z);
	w(fx);
	w(fz);
	server.broadcast(w.buffer, net::Channel::Reliable);
}

void notifyCooldown(net::Server& server, Player& p, uint8_t slot, float cd) {
	net::Writer w;
	w(protocol::MessageType::CooldownStart);
	w(slot);
	w(cd);
	
	server.send(p.GetPeer(), w.buffer, net::Channel::Reliable);
}

void sendPlayerGold(net::Server& server, Player& p) {
	if (!p.IsConnected())
		return;

	net::Writer w;
	w(protocol::MessageType::GoldUpdate);
	w(static_cast<std::uint32_t>(p.GetGold()));
	server.send(p.GetPeer(), w.buffer, net::Channel::Reliable);
}

void broadcastPlayerGold(net::Server& server) {
	for (Player& p : players)
		sendPlayerGold(server, p);
}

void broadcastKill(net::Server& server, Player& attacker, Player& deadPlayer) {
	net::Writer w;
	w(protocol::MessageType::PlayerKill);
	w(slotOf(attacker));
	w(slotOf(deadPlayer));
	server.broadcast(w.buffer, net::Channel::Reliable);
}
