#pragma once

#include <raylib.h>


class Player {
public:
	Player();
	~Player();
	void Draw();
	void SetPosition(Vector2 pos);
private:
	Vector2 m_position;
};
