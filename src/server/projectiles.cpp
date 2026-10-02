#include "projectiles.hpp"

#include <cmath>

#include "match.hpp"
#include "messages.hpp"

std::vector<Skillshot> skillshots;
std::uint16_t nextProjectileId = 0;
std::vector<Projectile> projectiles;

void updateProjectiles(net::Server& server, const float kTickSeconds) {
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

	for (Skillshot& s : skillshots) {
		const AbilityDef& a = *s.ability;
		Player& caster = players[s.caster];

		const float step = a.speed * kTickSeconds;
		s.position.x += s.dirX * step;
		s.position.z += s.dirZ * step;
		s.travelled += step;

		for (uint8_t j = 0; j < playerCount; j++) {
			Player& enemy = players[j];

			if (enemy.GetTeam() == caster.GetTeam() || !enemy.IsAlive() || s.alreadyHit->test(j))
				continue;

			const float dx = enemy.GetPosition().x - s.position.x;
			const float dz = enemy.GetPosition().z - s.position.z;
			const float hitRadius = a.width * 0.5f + enemy.GetCollisionRadius();
			if (dx * dx + dz * dz > hitRadius * hitRadius)
				continue;

			s.alreadyHit->set(j);
			CallHook(a, a.onHit, &caster, &enemy);

			if (!a.pierce) {
				s.done = true;
				broadcastProjectileEnd(server, s.id);
				break;
			}
		}
		if (s.travelled >= a.range)
			s.done = true;
	}
	
	std::erase_if(skillshots, [](const Skillshot& s) { return s.done; });
	std::erase_if(projectiles, [](const Projectile& p) { return p.done; });
}
