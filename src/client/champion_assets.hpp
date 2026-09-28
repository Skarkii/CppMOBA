#pragma once

#include <champions.hpp>

inline constexpr const char* ModelPath(champion::Id id) {
    switch (id)
    {
    case champion::Id::Barbarian: return "assets/champions/Barbarian/Barbarian.glb";
    case champion::Id::Knight:    return "assets/champions/Knight/Knight.glb";
    case champion::Id::Ranger:    return "assets/champions/Ranger/Ranger.glb";
    case champion::Id::Rogue:     return "assets/champions/Rogue/Rogue.glb";
    case champion::Id::Mage:      return "assets/champions/Mage/Mage.glb";
        // ...
    default:                      return nullptr;
    }
}
