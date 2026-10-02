#pragma once

#include <raylib.h>
#include <string_view>
#include <string>

#include "champions.hpp"

enum class Team : uint8_t { Blue, Red } ;

class Player {
public:
	Player();
	~Player();
	void Draw();
	void DrawOverlay(Camera& cam, bool showPlayerNames) const;
	void SetPosition(const Vector3 pos);
	Vector3 GetPosition() const;
	void SetChampId(champion::Id id);

	[[nodiscard]]
	champion::Id GetChampId() const;

	[[nodiscard]]
	std::string_view GetChampName() const;

	[[nodiscard]]
	bool IsSelf() const;

	void SetSelf();

	[[nodiscard]]
	float GetHealth() const;

	[[nodiscard]]
	float GetMana() const;

	[[nodiscard]]
	float GetMaxHealth() const;

	[[nodiscard]]
	float GetMaxMana() const;

	void SetModel(const Model* model);
	void SetHealth(const float health);
	void SetMana(const float mana);

	void SetPlayerName(std::string name);

	[[nodiscard]]
	std::string_view GetPlayerName() const;

	void SetTeam(const Team team);

	[[nodiscard]]
	Team GetTeam() const;

private:
	std::string m_name;
	Vector3 m_position;
	float m_rotation;
	Vector3 m_scale;
	Team m_team;

	float m_health;
	float m_maxHealth;

	float m_mana;
	float m_maxMana;

	champion::Id m_champId;
	bool m_self = false;
	const Model* m_model = nullptr;
};
