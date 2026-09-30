#pragma once


#include <cstdint>
#include <chrono>

namespace protocol
{
    constexpr std::uint16_t kVersion = 1;
    constexpr std::uint16_t kPort = 8000;

	constexpr auto kTick = std::chrono::milliseconds(33); // ~30 tps
	constexpr float kTickSeconds = std::chrono::duration<float>(kTick).count();

    constexpr uint16_t maximumNameLength = 24;

    enum class MessageType : std::uint8_t
    {
        // Handshake
        Hello = 0, // C->
        Welcome, // S -> C

        // Client->Server
        MoveCommand,
        AttackCommand,
        RecallCommand,

        // Server->Client
        MatchInfo,
        PlayerState,

        Count,
    };

    inline constexpr std::array<std::string_view, static_cast<std::size_t>(MessageType::Count)> kMessageNames{
    "Hello",
    "Welcome",

    "MoveCommand",
    "AttackCommand",
    "RecallCommand",

    "MatchInfo",
    "PlayerState",
    };

    constexpr std::string_view ToString(MessageType type) {
        const auto i = static_cast<std::size_t>(type);
        return i < kMessageNames.size() ? kMessageNames[i] : "Unknown";
    }
}
