#pragma once

#include <sol/sol.hpp>

#include <filesystem>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <cmath>


enum class AbilityType : std::uint8_t
{
	TargetedProjectile,
	Targeted,
	Area,
	Skillshot,
};

enum class Targeting : std::uint8_t
{
	Unit,
	Point,
	Direction,
	Self,
};

struct AbilityVisual
{
	std::string model;
	float scale = 1.0f;
	float radius = 0.2f;
	std::array<std::uint8_t, 4> color{ 255, 255, 255, 255 };
	std::string icon;
};

enum class Shape : std::uint8_t { Circle, Rect, Cone };

struct AbilityDef
{
	std::string id;
	std::string name;
	AbilityType type = AbilityType::Targeted;
	Targeting targeting = Targeting::Self;

	float cooldown = 0.0f;
	float manaCost = 0.0f;
	float range = 0.0f;
	float castTime = 0.0f;

	float speed = 0.0f;
	Shape shape = Shape::Circle;
	float radius = 0.0f;
	float width = 0.0f;
	float length = 0.0f;
	float angle = 0.0f;
	float delay = 0.0f;
	float spread = 0.0f;
	int count = 1;
	bool pierce = false;

	sol::protected_function onCast;
	sol::protected_function onHit;

	AbilityVisual visual;
};

class AbilityLibrary
{
public:
	explicit AbilityLibrary(sol::state& lua) : m_lua(lua) {}

	bool LoadDirectory(const std::filesystem::path& directory);

	[[nodiscard]]
	const AbilityDef* Find(std::string_view id) const;

	[[nodiscard]] 
	const auto& All() const { return m_abilities; }

private:
	std::optional<AbilityDef> loadFile(const std::filesystem::path& file);

	sol::state& m_lua;
	std::unordered_map<std::string, AbilityDef> m_abilities;
};

template <typename... Args>
void CallHook(const AbilityDef& ability, const sol::protected_function& hook, Args&&... args) {
	if (!hook.valid())
		return;

	const sol::protected_function_result result = hook(std::forward<Args>(args)...);
	if (!result.valid())
	{
		const sol::error error = result;
		std::println("Lua error in ability '{}': {}", ability.id, error.what());
	}
}

inline std::pair<float, float> SpreadDirection(const AbilityDef& a, float fx, float fz, int i) {
	const float t = a.count > 1 ? static_cast<float>(i) / static_cast<float>(a.count - 1) : 0.5f;
	const float angle = (-a.spread / 2.0f + a.spread * t) * (3.14159265f / 180.0f);
	const float c = std::cos(angle), s = std::sin(angle);
	return { fx * c - fz * s, fx * s + fz * c };
}
