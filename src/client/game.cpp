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

void Game::Run() {
	while (m_running && !WindowShouldClose()) {
		updateNetwork();

		const std::optional<std::uint8_t> hovered = enemyUnderMouse();

		if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
			if (hovered)
				attackCommand(*hovered);
			else if (auto point = mouseToGround())
				moveCommand(*point);
		}


		const float wheel = GetMouseWheelMove();

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
		if (IsKeyDown(KEY_X)) {
			const Vector3 pos = m_players[m_playerId].GetPosition();
			const float range = champion::GetStats(m_players[m_playerId].GetChampId()).attackRange;

			DrawCircle3D({ pos.x, 0.02f, pos.z }, range, { 1.0f, 0.0f, 0.0f }, 90.0f, DARKGRAY);
		}

		for (uint8_t i = 0; i < m_playerCount; i++) {
			m_players[i].Draw();
		}
		

		EndMode3D();

		for (uint8_t i = 0; i < m_playerCount; i++) {
			m_players[i].DrawOverlay(m_camera);
		}

		const Vector2 m = GetMousePosition();
		const Color color = hovered ? RED : WHITE;

		DrawTriangle(m, { m.x, m.y + 18.0f }, { m.x + 12.0f, m.y + 13.0f }, color);
		DrawTriangleLines(m, { m.x, m.y + 18.0f }, { m.x + 12.0f, m.y + 13.0f }, BLACK);

		DrawFPS(5, 5);
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
			if (!r.done())
				break; 
			m_playerId = playerId;
			//std::println("Handshake completed, playerID: {}", playerId);
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
			for (uint8_t i = 0; i < playerCount; i++) {
				r(champIds[i]);
			}
			if (!r.done())
				break; 
			for (uint8_t i = 0; i < playerCount; i++) {
				m_players[i].SetChampId(champIds[i]);
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
			for (uint8_t i = 0; i < m_playerCount; i++) {
				Vector3 pos;
				r(pos.x);
				r(pos.y);
				r(pos.z);
				positions[i] = pos;
				r(health[i]);
			}
			if (!r.done())
				break;
			for (uint8_t i = 0; i < m_playerCount; i++) {
				m_players[i].SetPosition(positions[i]);
				m_players[i].SetHealth(health[i]);
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

