#include "game.hpp"

#include <print>
#include "serialize.hpp"
#include "protocol.hpp"
#include <champion_assets.hpp>
#include <algorithm>


Game::Game(uint64_t token) {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
	InitWindow(1280, 720, "MOBA");
	HideCursor();
	SetExitKey(KEY_NULL);

	m_screenW = static_cast<float>(GetScreenWidth());
	m_screenH = static_cast<float>(GetScreenHeight());
	m_hud.SetScreenSize(m_screenW, m_screenH);
	m_menu.SetScreenSize(m_screenW, m_screenH);

	m_zoom = 10.0f;

	m_camera.position = { 0.0f, m_zoom, 10.0f };
	m_camera.target = { 0.0f, 0.0f, 0.0f };
	m_camera.up = { 0.0f, 1.0f, 0.0f };
	m_camera.fovy = 45.0f;
	m_camera.projection = CAMERA_PERSPECTIVE;

	m_token = token;
}

Game::~Game() {
	for (auto m : m_models) {
		UnloadModel(m.second);
	}
	m_models.clear();

	m_hud.Unload();

	CloseWindow();
}

void Game::Connect() {
	//m_client.connect(...), then m_client.send(...) and m_client.flush()
	if (!m_client.connect(m_hostServerName, m_hostServerPort)) {
		std::println("Failed to connect to server {}:{}", m_hostServerName, m_hostServerPort);
		// TODO: Handle failure to connect
	}
	std::println("Connecting to server {}:{}", m_hostServerName, m_hostServerPort);
	m_connectionState = ConnectionState::Connecting;

	sendHello();

	while (m_connectionState != ConnectionState::Joined || !m_retrievedMatchInfo) {
		updateNetwork();
	}

	std::println("Connected and retrieved match info");
}

void Game::loadModels() {
	for (int i = 0; i < m_playerCount; i++) {
		champion::Id id = m_players[i].GetChampId();
		if (!m_models.contains(id)) {
			m_models[id] = LoadModel(ModelPath(id));

			if (m_models[id].meshCount == 0) {
				std::println("Warning failed to load model for {}", m_players[i].GetChampName());
			}
		}

		m_players[i].SetModel(&m_models[id]);
	}
}

void Game::Prepare() {
	loadModels();
	m_hud.Load();
}

void Game::WaitForServer() {
}

std::optional<Vector3> Game::mouseToGround() const {
	const Ray ray = GetScreenToWorldRay(GetMousePosition(), m_camera);

	if (ray.direction.y >= -0.0001f)
		return std::nullopt;

	const float t = -ray.position.y / ray.direction.y;
	return Vector3{ ray.position.x + ray.direction.x * t,
					0.0f,
					ray.position.z + ray.direction.z * t };
}

// Returns the slot of the enemy under the mouse, if any.
std::optional<std::uint8_t> Game::enemyUnderMouse() const {
	const Ray ray = GetScreenToWorldRay(GetMousePosition(), m_camera);

	std::optional<std::uint8_t> best;
	float bestDistance = std::numeric_limits<float>::max();   // #include <limits>

	for (int i = 0; i < m_playerCount; i++) {
		if (i == m_playerId)
			continue;

		const Vector3 pos = m_players[i].GetPosition();
		const Vector3 center = { pos.x, pos.y + 1.2f, pos.z };

		const RayCollision hit = GetRayCollisionSphere(ray, center, 1.0f);
		if (hit.hit && hit.distance < bestDistance) {
			bestDistance = hit.distance;
			best = static_cast<std::uint8_t>(i);
		}
	}
	return best;
}

HudView Game::makeHudView() const {
	const Player& self = m_players[m_playerId];
	HudView view{};
	view.championName = self.GetChampName();
	view.health = self.GetHealth();
	view.maxHealth = self.GetMaxHealth();
	view.stats = champion::GetStats(self.GetChampId());
	view.mana = self.GetMana();
	view.maxMana = self.GetMaxMana();
	return view;
}

void Game::UpdateSettings() {
	if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE) != m_settings.fullscreen) {
		ToggleBorderlessWindowed();
		m_screenW = static_cast<float>(GetScreenWidth());
		m_screenH = static_cast<float>(GetScreenHeight());
		m_hud.SetScreenSize(m_screenW, m_screenH);
		m_menu.SetScreenSize(m_screenW, m_screenH);
	}

	if (m_settings.vsync) {
		SetWindowState(FLAG_VSYNC_HINT);
	}
	else {
		ClearWindowState(FLAG_VSYNC_HINT);
	}

	SetTargetFPS(m_settings.fpsCap);

	SetMasterVolume(m_settings.masterVolume);

	m_showPlayerNames = m_settings.showPlayerNames;

	//Keybinds remaining
}

void Game::Run() {
	while (m_running && !WindowShouldClose()) {
		updateNetwork();

#ifdef MOBA_DEBUG_OVERLAY
		if (IsKeyPressed(KEY_F3))
			m_showDebug = !m_showDebug;

		m_debugView.fps = GetFPS();
		m_debugView.frameMs = GetFrameTime() * 1000.0f;

		m_statsTimer += GetFrameTime();
		if (m_statsTimer >= 1.0f) {
			m_debugView.net = m_client.GetStats();
			m_debugView.bytesPerSecondIn = static_cast<float>(m_debugView.net.bytesReceived) / m_statsTimer;
			m_debugView.bytesPerSecondOut = static_cast<float>(m_debugView.net.bytesSent) / m_statsTimer;
			m_statsTimer = 0.0f;
		}
#endif

		const std::optional<std::uint8_t> hovered = enemyUnderMouse();
		float wheel = 0.0f;

		if (IsKeyPressed(KEY_ESCAPE) && !m_menu.IsCapturingKey())
			m_menu.Toggle();

		if (!m_menu.IsOpen()) {
			if (IsKeyPressed(KEY_B)) {
				recallCommand();
			}

			if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
				if (hovered)
					attackCommand(*hovered);
				else if (auto point = mouseToGround())
					moveCommand(*point);
			}


			wheel = GetMouseWheelMove();
		}

		if (wheel != 0.0f) {
			m_zoom -= wheel * 0.1f;
			m_zoom = std::clamp(m_zoom, 5.0f, 20.0f);
		}

		BeginDrawing();
		ClearBackground(RAYWHITE);
		const Vector3 playerPos = m_players[m_playerId].GetPosition();
		const Vector3 offset = { 0.0f, m_zoom, 6.0f };

		m_camera.target = playerPos;
		m_camera.position = { playerPos.x + offset.x,
							  playerPos.y + offset.y,
							  playerPos.z + offset.z };

		BeginMode3D(m_camera);	

		DrawGrid(40, 1.0f);
		if (!m_menu.IsOpen()) {
			if (IsKeyDown(KEY_X)) {
				const Vector3 pos = m_players[m_playerId].GetPosition();
				const float range = champion::GetStats(m_players[m_playerId].GetChampId()).attackRange;

				DrawCircle3D({ pos.x, 0.02f, pos.z }, range, { 1.0f, 0.0f, 0.0f }, 90.0f, DARKGRAY);
			}
		}

		for (uint8_t i = 0; i < m_playerCount; i++) {
			m_players[i].Draw();
		}
		

		EndMode3D();

		for (uint8_t i = 0; i < m_playerCount; i++) {
			m_players[i].DrawOverlay(m_camera, m_showPlayerNames);
		}

		const Vector2 m = GetMousePosition();
		const Color color = hovered ? RED : WHITE;

		#ifdef MOBA_DEBUG_OVERLAY
		if (m_showDebug)
			m_debugOverlay.Draw(m_debugView);
		#endif

		m_hud.Draw(makeHudView());

		if (m_menu.IsOpen()) {
			MenuAction act = m_menu.Update(m_settings);

			if (m_menu.HasSettingsChanged()) {
				UpdateSettings();
				m_menu.ResetSettingsChanged();
			}

			switch (act) {
			case MenuAction::Exit:
				m_running = false;
				break;
			case MenuAction::Resume:
				m_menu.Toggle();
				break;
			}

		}

		DrawTriangle(m, { m.x, m.y + 18.0f }, { m.x + 12.0f, m.y + 13.0f }, color);
		DrawTriangleLines(m, { m.x, m.y + 18.0f }, { m.x + 12.0f, m.y + 13.0f }, BLACK);

		EndDrawing();
	}
}

void Game::updateNetwork() {
	for (auto& event : m_client.poll())
		{
			std::visit(net::overloaded{
				[&]([[maybe_unused]] const net::Connected& e) {},

				[&]([[maybe_unused]] const net::Received& e) {
					//std::println("received {} bytes from on channel {}", e.data.size(), static_cast<int>(e.channel));
					onMessage(e.data);
				},

				[&]([[maybe_unused]] const net::Disconnected& e) {
					std::println("server disconnected client");
					m_client.disconnect();
				},

				}, event);
		}

	m_client.flush();
}

void Game::onMessage(const std::vector<std::byte>& data) {
	/*
	for (std::byte b : data)
		std::print("{:02X} ", static_cast<unsigned>(b));
	std::println("");
	*/

	net::Reader r(data);

	protocol::MessageType type{};
	r(type);
	if (r.failed)
		return;

	switch (type)
	{
	case protocol::MessageType::Welcome:
		{
			uint8_t playerId;
			r(playerId);
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break; 
			}
			m_playerId = playerId;
			m_connectionState = ConnectionState::Joined;
		}
		break;
	case protocol::MessageType::MatchInfo:
		{
			if (m_retrievedMatchInfo)
				break;

			uint8_t playerCount;
			r(playerCount);
			std::array<champion::Id, 10> champIds{};
			std::array<std::string, 10> names;

			for (uint8_t i = 0; i < playerCount; i++) {
				r(champIds[i]);

				uint16_t playerNameLength;
				r(playerNameLength);
				if (playerNameLength > protocol::maximumNameLength) {
					std::println("Retrieved player with name over limit, got: {} - maximum: {}", playerNameLength, protocol::maximumNameLength);
					break;
				}
				for (size_t j = 0; j < playerNameLength; j++) {
					char c;
					r(c);
					names[i] += c;
				}
			}
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break; 
			}
			for (uint8_t i = 0; i < playerCount; i++) {
				m_players[i].SetChampId(champIds[i]);
				m_players[i].SetPlayerName(names[i]);
			}
			m_players[m_playerId].SetSelf();
			m_playerCount = playerCount;
			m_retrievedMatchInfo = true;
		}
		break;
	case protocol::MessageType::PlayerState:
		{
			std::array<Vector3, 10> positions{};
			std::array<float, 10> health{};
			std::array<float, 10> mana{};
			std::uint32_t tick{};
			r(tick);
			for (uint8_t i = 0; i < m_playerCount; i++) {
				Vector3 pos;
				r(pos.x);
				r(pos.y);
				r(pos.z);
				positions[i] = pos;
				r(health[i]);
				r(mana[i]);
			}
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break; 
			}
#ifdef MOBA_DEBUG_OVERLAY
			if (m_haveTick && tick > m_lastTick + 1)
				m_debugView.snapshotsMissed += tick - m_lastTick - 1;
			m_debugView.snapshotsReceived++;
			m_debugView.serverTick = tick;
			m_lastTick = tick;
			m_haveTick = true;
#endif
			for (uint8_t i = 0; i < m_playerCount; i++) {
				m_players[i].SetPosition(positions[i]);
				m_players[i].SetHealth(health[i]);
				m_players[i].SetMana(mana[i]);
			}
		}
		break;

	default:
		break;
	}
}

void Game::sendHello() {
	net::Writer w;

	w(protocol::MessageType::Hello);
	w(protocol::kVersion);
	w(m_token);

	m_client.send(w.buffer, net::Channel::Reliable);
	m_client.flush();
}

void Game::disconnect() {
}

void Game::recallCommand() {
	net::Writer w;
	w(protocol::MessageType::RecallCommand);
	m_client.send(w.buffer, net::Channel::Reliable);
	m_client.flush();
}

void Game::moveCommand(const Vector3 pos) {
	net::Writer w;
	w(protocol::MessageType::MoveCommand);
	w(pos.x);
	w(pos.y);
	w(pos.z);
	m_client.send(w.buffer, net::Channel::Reliable);
	m_client.flush();
}

void Game::attackCommand(std::uint8_t targetId) {
	net::Writer w;
	w(protocol::MessageType::AttackCommand);
	w(targetId);
	m_client.send(w.buffer, net::Channel::Reliable);
	m_client.flush();
}

