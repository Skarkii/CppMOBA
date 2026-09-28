// server-main.cpp

#include "net.hpp"

#include <print>
#include <thread>
#include <chrono>

#include "serialize.hpp"
#include "protocol.hpp"

int main()
{
	net::Library lib;

	if (!lib) {
		std::println("Library failed to start");
		return EXIT_FAILURE;
	}

	net::Server server;

	uint16_t port = 8000;


	if (!server.start(port, 10)) {
		std::println("Failed to listen at port {}", port);
		return EXIT_FAILURE;
	}
	std::println("Listening on port {}", port);

	while (true) {
		for (auto& event : server.poll())
		{
			std::visit(net::overloaded{
				[&](const net::Connected& e) {
					std::println("peer {} connected", e.peer);

					net::Writer w;
					w(protocol::MessageType::Welcome);
					server.send(e.peer, w.buffer, net::Channel::Reliable);
				},
				[&](const net::Received& e) {
					std::println("received {} bytes from peer {} on channel {}",
						e.data.size(), e.peer, static_cast<int>(e.channel));
					// e.peer, e.channel, e.data
				},
				[&](const net::Disconnected& e) {
					std::println("peer {} disconnected", e.peer);
				},
				}, event);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	return EXIT_SUCCESS;
}
