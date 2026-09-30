#pragma once

// Shared champion data: used by both server and client.
// Keep file paths, models and animations out of here (those are client-only).

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace champion
{
    enum class Id : std::uint8_t
    {
        Barbarian,
        Knight,
        Mage,
        Ranger,
        Rogue,

        Count,
    };

    struct Stats
    {
        float moveSpeed;       // units per second
        float maxHealth;
        float attackRange;     // units
        float attackDamage;
        float collisionRadius; // units
        float attackSpeed;
    };

    // Everything that describes a champion.
    struct Definition
    {
        std::string_view name;
        Stats stats;
    };

    inline constexpr auto kDefinitions = []
        {
            std::array<Definition, static_cast<std::size_t>(Id::Count)> table{};

            auto set = [&](Id id, Definition definition)
                {
                    table[static_cast<std::size_t>(id)] = definition;
                };

            set(Id::Barbarian, {
                .name = "Barbarian",
                .stats = {.moveSpeed = 4.5f, .maxHealth = 650.f, .attackRange = 1.5f,
                           .attackDamage = 60.f, .collisionRadius = 0.6f, .attackSpeed = 1.0f },
                });

            set(Id::Knight, {
                .name = "Knight",
                .stats = {.moveSpeed = 4.5f, .maxHealth = 650.f, .attackRange = 1.5f,
                           .attackDamage = 60.f, .collisionRadius = 0.6f, .attackSpeed = 1.0f },
                });

            set(Id::Mage, {
                .name = "Mage",
                .stats = {.moveSpeed = 4.2f, .maxHealth = 480.f, .attackRange = 6.0f,
                           .attackDamage = 45.f, .collisionRadius = 0.5f, .attackSpeed = 1.0f },
                });

            set(Id::Ranger, {
                .name = "Ranger",
                .stats = {.moveSpeed = 10.5f, .maxHealth = 650.f, .attackRange = 7.5f,
                           .attackDamage = 60.f, .collisionRadius = 0.6f, .attackSpeed = 1.0f },
                });

            set(Id::Rogue, {
                .name = "Rogue",
                .stats = {.moveSpeed = 4.5f, .maxHealth = 650.f, .attackRange = 1.5f,
                           .attackDamage = 60.f, .collisionRadius = 0.6f, .attackSpeed = 1.0f },
                });

            return table;
        }();

    static_assert([]
        {
            for (const auto& definition : kDefinitions)
                if (definition.name.empty() || definition.stats.moveSpeed <= 0.f)
                    return false;
            return true;
        }(), "A champion in champion::Id has no entry in kDefinitions");

    inline constexpr const Definition& Get(Id id) {
        return kDefinitions[static_cast<std::size_t>(id)];
    }

    inline constexpr const Stats& GetStats(Id id) {
        return Get(id).stats;
    }
}

