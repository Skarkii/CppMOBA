// game.hpp

#pragma once

#include "champions.hpp"
#include "net.hpp"
#include "player.hpp"
#include "hud.hpp"
#include <array>
#include <raylib.h>
#include <unordered_map>
#include <optional>
#include "menu.hpp"
#include "chat.hpp"
#include "protocol.hpp"
#include "ability.hpp"
#include <raymath.h>

#ifdef MOBA_DEBUG_OVERLAY
#include "debug_overlay.hpp"
#endif

struct ClientProjectile
{
	uint16_t id = 0;
	Vector3 position;
	Vector3 direction;
	std::optional<uint8_t> target;
	const AbilityDef* ability;
	float travelled = 0.0f;
};


enum class ConnectionState { Disconnected, Connecting, Joined, Failed };

class Game {
public:
	Game(uint64_t token);
	~Game();
	bool Connect();
	void Prepare();
	void WaitForServer();
	void Run();
private:
	uint64_t m_token;
	Model m_map;
	const std::string m_hostServerName = "127.0.0.1";
	const uint16_t m_hostServerPort = 8000;
	bool m_running = true;

	net::Library m_netLibrary;
	net::Client  m_client;
	ConnectionState m_connectionState = ConnectionState::Disconnected;
	bool m_retrievedMatchInfo = false;

	void updateNetwork();
	void onMessage(const std::vector<std::byte>& data);
	void sendHello();
	void disconnect();

	void recallCommand();
	void moveCommand(const Vector3 pos);
	void attackCommand(std::uint8_t targetId);
	void castAbility(const std::uint8_t slot, const uint8_t targetSlot, const float pointX, const float pointZ);
	std::optional<Vector3> mouseToGround() const;
	Vector3 m_position = { 0 };
	float m_zoom;
	Camera m_camera;

	std::array<Player, 10> m_players = { };
	int m_playerCount;

	uint8_t m_playerId;
	bool m_showPlayerNames = false;

	void loadModels();
	std::unordered_map<champion::Id, Model> m_models;

	std::optional<std::uint8_t> enemyUnderMouse() const;

	float m_screenW = { 0.0f };
	float m_screenH = { 0.0f };

	HudView makeHudView() const;
	Hud m_hud;
	Menu m_menu;
	Settings m_settings;

	void updateSettings();

	Chat m_chat;

	void sendMessage(protocol::TextScope scope, std::string_view msg);

	sol::state m_lua;
	AbilityLibrary m_abilities{ m_lua };
	std::array<float, 4> m_cooldownLeft{};
	std::array<float, 4> m_cooldownTotal{};

	uint32_t m_gold{};

	std::vector<ClientProjectile> m_projectiles;

	std::unordered_map<std::string, Model> m_projectileModels;
	std::unordered_map<std::string, Texture2D> m_icons;

	void loadAbilities();
	void updateProjectiles(float dt);
	void drawProjectiles() const;


#ifdef MOBA_DEBUG_OVERLAY
	bool m_showDebug = true;
	DebugOverlay m_debugOverlay;
	DebugView m_debugView;
	float m_statsTimer = 0.0f;
	std::uint32_t m_lastTick = 0;
	bool m_haveTick = false;
#endif

};