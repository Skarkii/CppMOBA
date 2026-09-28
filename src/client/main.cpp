// client-main.cpp

#include "game.hpp"
#include <cstdlib>

int main( [[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
	Game game = Game();

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
