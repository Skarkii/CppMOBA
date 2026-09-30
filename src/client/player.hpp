#pragma once

#include <raylib.h>
#include <string_view>

#include "champions.hpp"

class Player {
public:
	Player();
	~Player();
	void Draw();
	void DrawOverlay(Camera& cam) const;
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

	void SetModel(const Model* model);

	void SetHealth(const float health);
private:
	Vector3 m_position;
	float m_rotation;
	Vector3 m_scale;

	float m_health;
	float m_maxHealth;

	champion::Id m_champId;
	bool m_self = false;
	const Model* m_model = nullptr;
};
