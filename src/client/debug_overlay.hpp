#pragma once

#ifdef MOBA_DEBUG_OVERLAY
#include "net.hpp"

#include <cstdint>

// Everything the debug overlay shows. Filled by Game each frame.
struct DebugView
{
	int fps = 0;
	float frameMs = 0.0f;

	net::Stats net;                      // refreshed once per second
	float bytesPerSecondIn = 0.0f;
	float bytesPerSecondOut = 0.0f;

	std::uint32_t serverTick = 0;        // last tick number received
	std::uint32_t snapshotsReceived = 0;
	std::uint32_t snapshotsMissed = 0;
};

class DebugOverlay
{
public:
	void Draw(const DebugView& view) const;
};
#endif
