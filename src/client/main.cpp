// client-main.cpp

#include "game.hpp"
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <print>

int main(  int argc,  char** argv) {
	std::uint64_t token = 1001;   // default when no argument is given

	if (argc > 1)
	{
		const char* text = argv[1];
		auto [end, error] = std::from_chars(text, text + std::strlen(text), token);
		if (error != std::errc{} || *end != '\0')
		{
			std::println("Invalid token: {}", text);
			return EXIT_FAILURE;
		}
	}

	Game game = Game(token);

	// Connect to Game Server and fetch required champions, maps etc
	// Set up data streams
	game.Connect();

	// Load textures, map, etc
	game.Prepare();

	// Wait for all clients to be ready
	game.WaitForServer();

	// Start game
	game.Run();

	return EXIT_SUCCESS;
}
