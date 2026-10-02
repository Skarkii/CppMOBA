#include "match.hpp"

#include "ability.hpp"
#include "champions.hpp"
#include "messages.hpp"
#include "projectiles.hpp"

std::array<Player, playerCount> players{ 
	Player(100, "Abc", champion::Id::Barbarian, Team::Blue), 
	Player(101, "def", champion::Id::Ranger, Team::Red),
	/*
	Player(102, "ghi", champion::Id::Rogue, Team::Blue),
	Player(103, "jkl", champion::Id::Mage, Team::Red),
	*/
};

std::uint8_t slotOf(const Player& p) {
	return static_cast<std::uint8_t>(&p - players.data());
}

void playerKilled(net::Server& server, Player& attacker, Player& deadPlayer) {
	attacker.AwardKill();
	sendPlayerGold(server, attacker);
	broadcastKill(server, attacker, deadPlayer);
}

void applyHit(net::Server& server, const AbilityDef& ability, Player& caster, Player& target) {
	if (!target.IsAlive())
		return;

	CallHook(ability, ability.onHit, &caster, &target);

	if (!target.IsAlive()) {
		playerKilled(server, caster, target);
	}
}

void updatePlayers(net::Server& server, const float kTickSeconds) {
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
				applyHit(server, ability, p, target);
				break;
			default:
				break;
			}

		}
	}
}
