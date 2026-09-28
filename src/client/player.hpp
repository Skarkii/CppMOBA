#pragma once

#include <raylib.h>
#include <string_view>

#include "champions.hpp"

class Player {
public:
	Player();
	~Player();
	void Draw();
	void SetPosition(Vector2 pos);
	void SetChampId(champion::Id id);

	[[nodiscard]]
	champion::Id GetChampId() const;

	[[nodiscard]]
	std::string_view GetChampName() const;
private:
	Vector2 m_position;
	champion::Id m_champId;
};
