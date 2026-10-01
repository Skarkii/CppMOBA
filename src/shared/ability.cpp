#include "ability.hpp"

namespace
{
	std::optional<AbilityType> parseType(std::string_view s) {
		if (s == "targeted_projectile") return AbilityType::TargetedProjectile;
		if (s == "targeted")            return AbilityType::Targeted;
		if (s == "area")                return AbilityType::Area;
		if (s == "skillshot")           return AbilityType::Skillshot;
		return std::nullopt;
	}

	std::optional<Targeting> parseTargeting(std::string_view s) {
		if (s == "unit")      return Targeting::Unit;
		if (s == "point")     return Targeting::Point;
		if (s == "direction") return Targeting::Direction;
		if (s == "self")      return Targeting::Self;
		return std::nullopt;
	}

	std::optional<Shape> parseShape(std::string_view s) {
		if (s == "circle") return Shape::Circle;
		if (s == "rect")   return Shape::Rect;
		if (s == "cone")   return Shape::Cone;
		return std::nullopt;
	}
}

bool AbilityLibrary::LoadDirectory(const std::filesystem::path& directory) {
	std::error_code ec;
	if (!std::filesystem::is_directory(directory, ec))
	{
		std::println("Ability folder not found: {}", directory.string());
		return false;
	}

	bool allOk = true;
	for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".lua")
			continue;

		if (auto ability = loadFile(entry.path()))
		{
			std::println("Loaded ability '{}' ({})", ability->id, ability->name);
			std::string id = ability->id;
			m_abilities.insert_or_assign(std::move(id), std::move(*ability));
		}
		else
		{
			allOk = false;
		}
	}
	return allOk;
}

const AbilityDef* AbilityLibrary::Find(std::string_view id) const {
	const auto it = m_abilities.find(std::string(id));
	return it != m_abilities.end() ? &it->second : nullptr;
}

std::optional<AbilityDef> AbilityLibrary::loadFile(const std::filesystem::path& file) {
	const std::string id = file.stem().string();

	const sol::protected_function_result result = m_lua.safe_script_file(file.string(), sol::script_pass_on_error);
	if (!result.valid())
	{
		const sol::error error = result;
		std::println("Failed to load ability '{}': {}", id, error.what());
		return std::nullopt;
	}
	if (result.get_type() != sol::type::table)
	{
		std::println("Ability '{}' must return a table", id);
		return std::nullopt;
	}
	const sol::table t = result;

	AbilityDef a;
	a.id = id;
	a.name = t.get_or<std::string>("name", id);

	const auto type = parseType(t.get_or<std::string>("type", ""));
	if (!type)
	{
		std::println("Ability '{}': missing or unknown 'type'", id);
		return std::nullopt;
	}
	a.type = *type;

	const auto targeting = parseTargeting(t.get_or<std::string>("targeting", ""));
	if (!targeting)
	{
		std::println("Ability '{}': missing or unknown 'targeting'", id);
		return std::nullopt;
	}
	a.targeting = *targeting;

	a.cooldown = t.get_or("cooldown", 0.0f);
	a.manaCost = t.get_or("manaCost", 0.0f);
	a.range = t.get_or("range", 0.0f);
	a.castTime = t.get_or("castTime", 0.0f);

	a.speed = t.get_or("speed", 0.0f);
	a.radius = t.get_or("radius", 0.0f);
	a.width = t.get_or("width", 0.0f);
	a.length = t.get_or("length", 0.0f);
	a.angle = t.get_or("angle", 0.0f);
	a.delay = t.get_or("delay", 0.0f);
	a.pierce = t.get_or("pierce", false);

	if (const auto shape = parseShape(t.get_or<std::string>("shape", "circle")))
		a.shape = *shape;
	else
	{
		std::println("Ability '{}': unknown 'shape'", id);
		return std::nullopt;
	}

	if (a.type == AbilityType::TargetedProjectile && a.speed <= 0.0f)
	{
		std::println("Ability '{}': targeted_projectile needs a 'speed'", id);
		return std::nullopt;
	}

	a.onCast = t.get_or<sol::protected_function>("onCast", sol::lua_nil);
	a.onHit = t.get_or<sol::protected_function>("onHit", sol::lua_nil);

	if (auto v = t.get<sol::optional<sol::table>>("visual"))
	{
		a.visual.model = v->get_or<std::string>("model", "");
		a.visual.scale = v->get_or("scale", 1.0f);
		a.visual.radius = v->get_or("radius", 0.2f);

		if (auto c = v->get<sol::optional<sol::table>>("color"))
			for (int i = 0; i < 3; i++)
				a.visual.color[i] = static_cast<std::uint8_t>(c->get_or(i + 1, 255));
	}

	return a;
}
