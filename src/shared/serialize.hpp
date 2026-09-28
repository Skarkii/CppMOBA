#pragma once

// Byte writer/reader for building your own messages.
// Reader never reads past the end: on short data it sets `failed` and returns zeros.

#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <span>
#include <type_traits>
#include <vector>

namespace net
{
    // Assumes little-endian (x86/ARM). Add std::byteswap here if that ever changes.
    static_assert(std::endian::native == std::endian::little);

    class Writer
    {
    public:
        static constexpr bool reading = false;

        template <typename T>
            requires std::is_arithmetic_v<T> || std::is_enum_v<T>
        void operator()(const T& v)
        {
            auto bytes = std::bit_cast<std::array<std::byte, sizeof(T)>>(v);
            buffer.insert(buffer.end(), bytes.begin(), bytes.end());
        }

        std::vector<std::byte> buffer;
    };

    class Reader
    {
    public:
        static constexpr bool reading = true;

        explicit Reader(std::span<const std::byte> d) : data(d) {}

        template <typename T>
            requires std::is_arithmetic_v<T> || std::is_enum_v<T>
        void operator()(T& v)
        {
            if (failed || data.size() - pos < sizeof(T))
            {
                failed = true;
                v = {};
                return;
            }
            std::memcpy(&v, data.data() + pos, sizeof(T));
            pos += sizeof(T);
        }

        // True if everything was read and nothing is left over.
        bool done() const { return !failed && pos == data.size(); }

        std::span<const std::byte> data;
        std::size_t pos = 0;
        bool failed = false;
    };
}