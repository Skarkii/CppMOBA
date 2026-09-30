#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#endif
#include <enet/enet.h>

#include "net.hpp"

namespace net
{
    namespace
    {
        constexpr auto kChannelCount = static_cast<std::size_t>(Channel::Count);

        ENetPacket* makePacket(std::span<const std::byte> data, Channel channel) {
            const enet_uint32 flags = channel == Channel::Reliable
                ? static_cast<enet_uint32>(ENET_PACKET_FLAG_RELIABLE)
                : 0u; // unreliable + sequenced
            return enet_packet_create(data.data(), data.size(), flags);
        }

        void sendPacket(ENetPeer* peer, std::span<const std::byte> data, Channel channel) {
            ENetPacket* packet = makePacket(data, channel);
            if (enet_peer_send(peer, static_cast<enet_uint8>(channel), packet) < 0)
                enet_packet_destroy(packet); // ENet only takes ownership on success
        }

        std::vector<Event> collectEvents(ENetHost* host) {
            std::vector<Event> events;
            if (!host)
                return events;

            ENetEvent ev;
            while (enet_host_service(host, &ev, 0) > 0)
            {
                const PeerId id = ev.peer->incomingPeerID;

                switch (ev.type)
                {
                case ENET_EVENT_TYPE_CONNECT:
                    events.emplace_back(Connected{ id });
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:
                    events.emplace_back(Disconnected{ id });
                    break;

                case ENET_EVENT_TYPE_RECEIVE:
                    if (ev.channelID < kChannelCount)
                    {
                        const auto* begin = reinterpret_cast<const std::byte*>(ev.packet->data);
                        events.emplace_back(Received{ id, static_cast<Channel>(ev.channelID),
                                                     std::vector<std::byte>(begin, begin + ev.packet->dataLength) });
                    }
                    enet_packet_destroy(ev.packet);
                    break;

                default:
                    break;
                }
            }
            return events;
        }
    }

    // ---- Library ----------------------------------------------------------
    Library::Library() : m_ok(enet_initialize() == 0) {}

    Library::~Library() {
        if (m_ok)
            enet_deinitialize();
    }

    // ---- Server -----------------------------------------------------------
    Server::~Server() { stop(); }

    bool Server::start(std::uint16_t port, std::size_t maxClients) {
        stop();

        ENetAddress address{};
        address.host = ENET_HOST_ANY;
        address.port = port;

        m_host = enet_host_create(&address, maxClients, kChannelCount, 0, 0);
        return m_host != nullptr;
    }

    void Server::stop() {
        if (!m_host)
            return;

        for (std::size_t i = 0; i < m_host->peerCount; ++i)
        {
            ENetPeer& peer = m_host->peers[i];
            if (peer.state != ENET_PEER_STATE_DISCONNECTED)
                enet_peer_disconnect_now(&peer, 0);
        }
        enet_host_destroy(m_host);
        m_host = nullptr;
    }

    std::vector<Event> Server::poll() { return collectEvents(m_host); }

    void Server::send(PeerId peer, std::span<const std::byte> data, Channel channel) {
        if (!m_host || peer >= m_host->peerCount)
            return;

        ENetPeer& p = m_host->peers[peer];
        if (p.state == ENET_PEER_STATE_CONNECTED)
            sendPacket(&p, data, channel);
    }

    void Server::broadcast(std::span<const std::byte> data, Channel channel) {
        if (m_host)
            enet_host_broadcast(m_host, static_cast<enet_uint8>(channel), makePacket(data, channel));
    }

    void Server::disconnect(PeerId peer) {
        if (m_host && peer < m_host->peerCount)
            enet_peer_disconnect(&m_host->peers[peer], 0);
    }

    void Server::flush() {
        if (m_host)
            enet_host_flush(m_host);
    }

    // ---- Client -----------------------------------------------------------
    Client::~Client() { disconnect(); }

    bool Client::connect(const std::string& host, std::uint16_t port, std::uint32_t timeoutMs) {
        disconnect();

        m_host = enet_host_create(nullptr, 1, kChannelCount, 0, 0);
        if (!m_host)
            return false;

        ENetAddress address{};
        if (enet_address_set_host(&address, host.c_str()) != 0)
        {
            disconnect();
            return false;
        }
        address.port = port;

        m_server = enet_host_connect(m_host, &address, kChannelCount, 0);
        if (!m_server)
        {
            disconnect();
            return false;
        }

        ENetEvent ev;
        if (enet_host_service(m_host, &ev, timeoutMs) > 0 && ev.type == ENET_EVENT_TYPE_CONNECT)
            return true;

        enet_peer_reset(m_server);
        m_server = nullptr;
        disconnect();
        return false;
    }

    void Client::disconnect() {
        if (m_server)
        {
            // Tell the server, then wait briefly for the acknowledgement.
            enet_peer_disconnect(m_server, 0);

            ENetEvent ev;
            while (enet_host_service(m_host, &ev, 1000) > 0)
            {
                if (ev.type == ENET_EVENT_TYPE_RECEIVE)
                    enet_packet_destroy(ev.packet);
                else if (ev.type == ENET_EVENT_TYPE_DISCONNECT)
                {
                    m_server = nullptr;
                    break;
                }
            }
            if (m_server)
                enet_peer_reset(m_server);
            m_server = nullptr;
        }

        if (m_host)
        {
            enet_host_destroy(m_host);
            m_host = nullptr;
        }
    }

    std::vector<Event> Client::poll() {
        auto events = collectEvents(m_host);
        for (const auto& e : events)
        {
            if (std::holds_alternative<Disconnected>(e))
                m_server = nullptr; // kicked or timed out
        }
        return events;
    }

    void Client::send(std::span<const std::byte> data, Channel channel) {
        if (m_server)
            sendPacket(m_server, data, channel);
    }

    void Client::flush() {
        if (m_host)
            enet_host_flush(m_host);
    }

    Stats Client::GetStats() {
        Stats stats;
        if (m_server)
        {
            stats.pingMs = m_server->roundTripTime;
            stats.pingVarianceMs = m_server->roundTripTimeVariance;
            stats.packetLoss = static_cast<float>(m_server->packetLoss) /
                static_cast<float>(ENET_PEER_PACKET_LOSS_SCALE);
        }
        if (m_host)
        {
            stats.bytesSent = m_host->totalSentData;
            stats.bytesReceived = m_host->totalReceivedData;
            m_host->totalSentData = 0;      // ENet expects the user to reset these
            m_host->totalReceivedData = 0;
        }
        return stats;
    }
}
