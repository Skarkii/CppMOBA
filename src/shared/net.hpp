#pragma once

// Thin wrapper around ENet: one UDP connection with a reliable and an
// unreliable channel. It only moves bytes; what the bytes mean is up to you.
// The ENet headers stay inside net.cpp.

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <variant>
#include <vector>

struct _ENetHost;
struct _ENetPeer;

namespace net
{
    using PeerId = std::uint16_t;

    enum class Channel : std::uint8_t
    {
        Reliable = 0,   // arrives, in order (resent if lost)
        Unreliable = 1, // may be lost; late packets are dropped
        Count
    };

    struct Connected { PeerId peer; };
    struct Disconnected { PeerId peer; };
    struct Received { PeerId peer; Channel channel; std::vector<std::byte> data; };

    using Event = std::variant<Connected, Disconnected, Received>;

    // Helper for std::visit with several lambdas.
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

    // Create one at the start of main(). Initializes ENet and shuts it down
    // again when it goes out of scope.
    class Library
    {
    public:
        Library();
        ~Library();
        Library(const Library&) = delete;
        Library& operator=(const Library&) = delete;

        explicit operator bool() const { return m_ok; }

    private:
        bool m_ok = false;
    };

    class Server
    {
    public:
        Server() = default;
        ~Server();
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;

        bool start(std::uint16_t port, std::size_t maxClients);
        void stop();

        // Non-blocking. Returns everything that happened since the last call.
        std::vector<Event> poll();

        void send(PeerId peer, std::span<const std::byte> data, Channel channel);
        void broadcast(std::span<const std::byte> data, Channel channel);

        // Graceful: a Disconnected event for this peer follows in poll().
        void disconnect(PeerId peer);

        // Sends queued packets now instead of on the next poll().
        void flush();

    private:
        _ENetHost* m_host = nullptr;
    };

    class Client
    {
    public:
        Client() = default;
        ~Client();
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;

        // Blocks until connected or the timeout expires.
        bool connect(const std::string& host, std::uint16_t port, std::uint32_t timeoutMs = 5000);
        void disconnect();
        bool isConnected() const { return m_server != nullptr; }

        // Non-blocking. Peer ids in events are always 0 (the server).
        std::vector<Event> poll();

        void send(std::span<const std::byte> data, Channel channel);
        void flush();

    private:
        _ENetHost* m_host = nullptr;
        _ENetPeer* m_server = nullptr;
    };
}