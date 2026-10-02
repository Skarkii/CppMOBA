#pragma once

#include <cstddef>
#include <vector>

#include "net.hpp"
#include "protocol.hpp"

void OnHandshakeFailed(net::Server& server, protocol::MessageType type);
void onMessage(const std::vector<std::byte>& data, net::Server& server, const net::PeerId peer);
