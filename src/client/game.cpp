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
	for (auto m : m_models)
		UnloadModel(m.second);

	m_models.clear();

	for (auto& [path, model] : m_projectileModels)
		UnloadModel(model);

	m_projectileModels.clear();

	for (auto& [path, icon] : m_icons)
		UnloadTexture(icon);

	m_icons.clear();

	m_projectileModels.clear();

	m_hud.Unload();

	CloseWindow();
}

bool Game::Connect() {
	if (!m_client.connect(m_hostServerName, m_hostServerPort)) {
		std::println("Failed to connect to server {}:{}", m_hostServerName, m_hostServerPort);
		return false;
	}
	std::println("Connecting to server {}:{}", m_hostServerName, m_hostServerPort);
	m_connectionState = ConnectionState::Connecting;

	sendHello();

	while (m_connectionState != ConnectionState::Joined || !m_retrievedMatchInfo) {
		updateNetwork();

		if (m_connectionState == ConnectionState::Failed)
			return false;
	}

	std::println("Connected and retrieved match info");
	return true;
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
	loadAbilities();
	m_map = LoadModel("assets/maps/threelane/map.glb");
	if (m_map.meshCount == 0)
		std::println("Warning: failed to load map model");

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
	view.gold = m_gold;

	const champion::Definition& def = champion::Get(self.GetChampId());
	for (size_t i = 0; i < 4; i++) {
		SlotView& s = view.slots[i];
		s.cooldownLeft = m_cooldownLeft[i];
		s.cooldownTotal = m_cooldownTotal[i];

		const AbilityDef* a = def.abilities[i].empty() ? nullptr : m_abilities.Find(def.abilities[i]);
		if (!a)
			continue;
		s.name = a->name;
		s.notEnoughMana = self.GetMana() < a->manaCost;
		if (m_icons.contains(a->visual.icon))
			s.icon = &m_icons.at(a->visual.icon);
	}

	return view;
}

void Game::updateSettings() {
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
		const float dt = GetFrameTime();

		updateNetwork();

		updateProjectiles(dt);

		for (float& cd : m_cooldownLeft)
			cd = std::max(0.0f, cd - dt);

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

		const bool wasTyping = m_chat.IsTyping();
		if (!m_menu.IsOpen()) {
			if (auto input = m_chat.Update()) {
				//m_chat.AddMessage(m_players[m_playerId].GetPlayerName(), SKYBLUE, input->allChat, input->text);
				sendMessage(input->scope, input->text);
			}
		}

		if (!m_menu.IsCapturingKey() && !wasTyping) {
			if (IsKeyPressed(KEY_ESCAPE) )
				m_menu.Toggle();
		}

		if (!m_menu.IsOpen() && !wasTyping) {
			if (IsKeyPressed(KEY_B)) {
				recallCommand();
			}

			constexpr std::array<KeyboardKey, 4> kAbilityKeys = { KEY_Q, KEY_W, KEY_E, KEY_R };
			for (std::uint8_t slot = 0; slot < kAbilityKeys.size(); slot++)
				if (IsKeyPressed(kAbilityKeys[slot]))
					if (auto point = mouseToGround())
						castAbility(slot, 255, point->x, point->z);


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
		ClearBackground({20, 22, 26, 255});
		const Vector3 playerPos = m_players[m_playerId].GetPosition();
		const Vector3 offset = { 0.0f, m_zoom, 6.0f };

		m_camera.target = playerPos;
		m_camera.position = { playerPos.x + offset.x,
							  playerPos.y + offset.y,
							  playerPos.z + offset.z };

		BeginMode3D(m_camera);	

		DrawModel(m_map, { 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

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
		

		drawProjectiles();

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

		m_chat.Draw(m_screenH - 140.0f);

		if (m_menu.IsOpen()) {
			MenuAction act = m_menu.Update(m_settings);

			if (m_menu.HasSettingsChanged()) {
				updateSettings();
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
	case protocol::MessageType::DeclineInvalidMessage:
	case protocol::MessageType::DeclineProtocolVersion:
	case protocol::MessageType::DeclineToken:
		{
			std::println("Handshake failed: {}", protocol::ToString(type));
			m_connectionState = ConnectionState::Failed;
		}
		break;
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

			if (playerCount > 10) {
				std::println("{} rejected: invalid playercount, got {}, maximum: {}", protocol::ToString(type), playerCount, 10);
				break;
			}

			std::array<champion::Id, 10> champIds{};
			std::array<std::string, 10> names;
			std::array<Team, 10> teams;

			for (uint8_t i = 0; i < playerCount; i++) {
				r(champIds[i]);
				r.readString(names[i], 32);
				r(teams[i]);
			}

			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break; 
			}

			for (uint8_t i = 0; i < playerCount; i++) {
				m_players[i].SetChampId(champIds[i]);
				m_players[i].SetPlayerName(names[i]);
				m_players[i].SetTeam(teams[i]);
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
	case protocol::MessageType::ChatMessage:
		{
			protocol::TextScope scope;
			champion::Id champId;
			std::string msg;
			std::string sender;
			r(scope);
			r(champId);
			r.readString(msg, protocol::maximumChatLength);
			r.readString(sender, protocol::maximumNameLength);
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break; 
			}
			
			std::string_view champName = champion::Get(champId).name;
			m_chat.AddMessage(sender, champName, scope, msg);
		}
		break;
	case protocol::MessageType::ProjectileSpawn:
		{
			std::uint8_t caster{}, target{};
			std::string abilityId;
			r(caster);
			r(target);
			r.readString(abilityId, 64);
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break;
			}
			if (caster >= m_playerCount || target >= m_playerCount)
				break;

			const AbilityDef* ability = m_abilities.Find(abilityId);
			if (!ability) {
				std::println("ProjectileSpawn: unkown ability '{}'", abilityId);
				break;
			}

			Vector3 playerPos = m_players[caster].GetPosition();
			Vector3 targetPos = m_players[target].GetPosition();
			const Vector3 start = Vector3(playerPos.x, playerPos.y + 1.2f, playerPos.z);
			const Vector3 end = Vector3(targetPos.x, targetPos.y + 1.2f, targetPos.z);
			m_projectiles.push_back({ 
				.position = start,
				.direction = Vector3Normalize(Vector3Subtract(end, start)),
				.target = target,
				.ability = ability,
				});
		}
		break;
	case protocol::MessageType::SkillshotCast:
		{
			uint16_t firstId;
			uint8_t casterSlot;
			std::string abilityId;
			float x{}, z{}, fx{}, fz{};

			r(firstId);
			r(casterSlot);
			r.readString(abilityId, 64);
			r(x);
			r(z);
			r(fx);
			r(fz);
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break;
			}

			if (casterSlot >= m_playerCount)
				break;

			const AbilityDef* ability = m_abilities.Find(abilityId);
			if (!ability) {
				std::println("ProjectileSpawn: unkown ability '{}'", abilityId);
				break;
			}

			for (int i = 0; i < ability->count; i++) {
				const auto [dirX, dirZ] = SpreadDirection(*ability, fx, fz, i);
				m_projectiles.push_back({
					.id = static_cast<uint16_t>(firstId + i),
					.position = Vector3(x, 1.2f, z),
					.direction = Vector3(dirX, 0, dirZ),
					.ability = ability,
					});
			}
		}
		break;
	case protocol::MessageType::ProjectileEnd:
		{
			uint16_t id;
			r(id);
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break;
			}
			std::erase_if(m_projectiles, [&](const ClientProjectile& p) {
				return p.id == id;
				});

		}
		break;
	case protocol::MessageType::CooldownStart:
		{
			uint8_t slot;
			float cd;
			r(slot);
			r(cd);
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break;
			}

			if (slot >= 4)
				break;

			m_cooldownLeft[slot] = cd;
			m_cooldownTotal[slot] = cd;
		}
		break;
	case protocol::MessageType::GoldUpdate:
		{
			uint32_t gold;
			r(gold);
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break;
			}

			m_gold = gold;
		}
		break;
	case protocol::MessageType::PlayerKill:
		{
			uint8_t attacker{}, killedPlayer{};
			r(attacker);
			r(killedPlayer);
			if (!r.done()) {
				std::println("{} rejected: read {} of {} bytes", protocol::ToString(type), r.pos, data.size());
				break;
			}
			if (attacker > m_playerCount || killedPlayer > m_playerCount)
				break;

			m_chat.AddSystemMessage(std::format("{} ({}) killed {} ({})", 
				m_players[attacker].GetPlayerName(),
				m_players[attacker].GetChampName(),
				m_players[killedPlayer].GetPlayerName(),
				m_players[killedPlayer].GetChampName()
			));
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

void Game::castAbility(const std::uint8_t slot, const uint8_t targetSlot, const float pointX, const float pointZ) {
	net::Writer w;
	w(protocol::MessageType::CastAbility);
	w(slot);
	w(targetSlot);
	w(pointX);
	w(pointZ);
	m_client.send(w.buffer, net::Channel::Reliable);
	m_client.flush();
}

void Game::sendMessage(protocol::TextScope scope, std::string_view msg){
	net::Writer w;
	w(protocol::MessageType::ChatSend);
	w(scope);
	w.writeString(msg);
	m_client.send(w.buffer, net::Channel::Reliable);
	m_client.flush();
}

void Game::loadAbilities() {
	m_lua.open_libraries(sol::lib::base, sol::lib::math);

	if(!m_abilities.LoadDirectory("assets/abilities"))
		std::println("Warning: some abilities failed to load");

	for (const auto& [id, ability] : m_abilities.All()) {
		const std::string& path = ability.visual.model;

		if (path.empty() || m_projectileModels.contains(path))
			continue;

		const std::string fullPath = "assets/" + path;
		Model model = LoadModel(fullPath.c_str());

		if (model.meshCount == 0) {
			std::println("Warning: failed to load model '{}' for ability '{}'", fullPath, id);
			continue;
		}
		m_projectileModels.emplace(path, model);
	}

	for (const auto& [id, ability] : m_abilities.All()) {
		const std::string& path = ability.visual.icon;

		if (path.empty() || m_icons.contains(path))
			continue;

		const std::string fullPath = "assets/" + path;
		Texture2D icon = LoadTexture(fullPath.c_str());

		if (icon.id == 0) {
			std::println("Warning: failed to load icon '{}' for ability '{}'", fullPath, id);
			continue;
		}

		m_icons.emplace(path, icon);
	}
}

void Game::updateProjectiles(float dt) {
	std::erase_if(m_projectiles, [&](ClientProjectile& p) {
		const float step = p.ability->speed * dt;

		if (!p.target) {
			p.position = Vector3Add(p.position, Vector3Scale(p.direction, step));
			p.travelled += step;
			return p.travelled >= p.ability->range;
		}

		const Vector3 aim = Vector3Add(m_players[*p.target].GetPosition(), { 0.0f, 1.2f, 0.0f });
		const Vector3 toTarget = Vector3Subtract(aim, p.position);
		const float distance = Vector3Length(toTarget);

		if (distance <= step)
			return true;

		p.direction = Vector3Scale(toTarget, 1.0f / distance);
		p.position = Vector3Add(p.position, Vector3Scale(p.direction, step));
		return false;
	});
}

void Game::drawProjectiles() const {
	for (const ClientProjectile& p : m_projectiles)
	{
		const AbilityVisual& v = p.ability->visual;
		const Color color = { v.color[0], v.color[1], v.color[2], v.color[3] };

		const auto it = m_projectileModels.find(v.model);
		if (it != m_projectileModels.end())
		{
			const float yaw = atan2f(p.direction.x, p.direction.z) * RAD2DEG;
			DrawModelEx(it->second, p.position, { 0.0f, 1.0f, 0.0f }, yaw,
				{ v.scale, v.scale, v.scale }, color);
		}
		else
		{
			DrawSphere(p.position, v.radius, color);
		}
	}
}
