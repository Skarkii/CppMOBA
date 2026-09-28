#pragma once


#include <cstdint>

namespace protocol
{
    constexpr std::uint16_t kVersion = 1;
    constexpr std::uint16_t kPort = 8000;

    enum class MessageType : std::uint8_t
    {
        Hello = 0,
        Welcome,
    };
}
