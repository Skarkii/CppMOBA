#pragma once


#include <cstdint>
#include <chrono>

#include <array>
#include <cstddef>
#include <string_view>

namespace protocol
{
    constexpr std::uint16_t kVersion = 1;
    constexpr std::uint16_t kPort = 8000;

    constexpr std::uint32_t kTicksPerSecond = 30;
    constexpr auto kTick = std::chrono::microseconds(1000000 / kTicksPerSecond);
    constexpr float kTickSeconds = 1.0f / static_cast<float>(kTicksPerSecond);

	//constexpr auto kTick = std::chrono::milliseconds(33); // ~30 tps
	//constexpr float kTickSeconds = std::chrono::duration<float>(kTick).count();

    constexpr uint16_t maximumNameLength = 24;
    constexpr uint16_t maximumChatLength = 50;

    enum class TextScope : std::uint8_t {
        Team,
        All,
    };

    enum class MessageType : std::uint8_t
    {
        // Handshake
        Hello = 0, // C->
        Welcome, // S -> C
        DeclineProtocolVersion,
        DeclineToken,
        DeclineInvalidMessage,

        // Client->Server
        MoveCommand,
        AttackCommand,
        RecallCommand,
        ChatSend,
        CastAbility,

        // Server->Client
        MatchInfo,
        PlayerState,
        ChatMessage,
        ProjectileSpawn,
        ProjectileEnd,
        SkillshotCast,
        CooldownStart,
        GoldUpdate,
        PlayerKill,

        Count,
    };

    inline constexpr std::array<std::string_view, static_cast<std::size_t>(MessageType::Count)> kMessageNames{
    "Hello",
    "Welcome",
	"DeclineProtocolVersion",
	"DeclineToken",
	"DeclineInvalidMessage",

    "MoveCommand",
    "AttackCommand",
    "RecallCommand",
    "ChatSend",
    "CastAbility",

    "MatchInfo",
    "PlayerState",
    "ChatMessage",
    "ProjectileSpawn",
    "ProjectileEnd",
    "SkillshotCast",
    "CooldownStart",
    "GoldUpdate",
    "PlayerKill",
    };

    constexpr std::string_view ToString(MessageType type) {
        const auto i = static_cast<std::size_t>(type);
        return i < kMessageNames.size() ? kMessageNames[i] : "Unknown";
    }
}
