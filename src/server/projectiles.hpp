#pragma once

#include <bitset>
#include <cstdint>
#include <memory>
#include <vector>

#include "ability.hpp"
#include "net.hpp"
#include "player.hpp"

struct Skillshot
{
	std::uint16_t id;
	Vector3 position;
	float dirX, dirZ;
	float travelled = 0.0f;
	std::uint8_t caster;
	const AbilityDef* ability;
	std::shared_ptr<std::bitset<10>> alreadyHit;
	bool done = false;
};

struct Projectile {
	Vector3 position;
	std::uint8_t caster;
	std::uint8_t target;
	float speed;
	const AbilityDef* ability;
	bool done = false;
};

extern std::vector<Skillshot> skillshots;
extern std::uint16_t nextProjectileId;
extern std::vector<Projectile> projectiles;

void updateProjectiles(net::Server& server, const float kTickSeconds);
