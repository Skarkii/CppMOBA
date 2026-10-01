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

#ifdef MOBA_DEBUG_OVERLAY
#include "debug_overlay.hpp"
#endif

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

	void UpdateSettings();

#ifdef MOBA_DEBUG_OVERLAY
	bool m_showDebug = true;
	DebugOverlay m_debugOverlay;
	DebugView m_debugView;
	float m_statsTimer = 0.0f;
	std::uint32_t m_lastTick = 0;
	bool m_haveTick = false;
#endif

};