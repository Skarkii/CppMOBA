#pragma once


#include <cstdint>
#include <chrono>

namespace protocol
{
    constexpr std::uint16_t kVersion = 1;
    constexpr std::uint16_t kPort = 8000;

	constexpr auto kTick = std::chrono::milliseconds(33); // ~30 tps
	constexpr float kTickSeconds = std::chrono::duration<float>(kTick).count();

    enum class MessageType : std::uint8_t
    {
        // Handshake
        Hello = 0, // C->
        Welcome, // S -> C

        // Client->Server
        MoveCommand,
        AttackCommand,

        // Server->Client
        MatchInfo,
        PlayerState,
    };
}
