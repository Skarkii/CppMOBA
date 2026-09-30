#ifdef MOBA_DEBUG_OVERLAY
#include "debug_overlay.hpp"

#include <raylib.h>

namespace
{
	// Green when good, yellow when so-so, red when bad.
	Color rate(float value, float okBelow, float badAbove) {
		if (value < okBelow)  return GREEN;
		if (value < badAbove) return YELLOW;
		return RED;
	}
}

void DebugOverlay::Draw(const DebugView& view) const {
	constexpr int fontSize = 16;
	constexpr int lineHeight = fontSize + 4;
	constexpr int lines = 7;
	constexpr int x = 10;
	int y = 10;

	DrawRectangle(x - 6, y - 6, 250, lines * lineHeight + 8, Fade(BLACK, 0.6f));

	// One line: label in white, value in its own color.
	auto line = [&](const char* label, const char* value, Color color)
		{
			DrawText(label, x, y, fontSize, RAYWHITE);
			DrawText(value, x + 110, y, fontSize, color);
			y += lineHeight;
		};

	const float total = static_cast<float>(view.snapshotsReceived + view.snapshotsMissed);
	const float snapshotLoss = total > 0.0f ? static_cast<float>(view.snapshotsMissed) / total : 0.0f;

	const Color fpsColor = view.fps >= 55 ? GREEN : (view.fps >= 30 ? YELLOW : RED);
	line("FPS", TextFormat("%d (%.1f ms)", view.fps, view.frameMs), fpsColor);
	line("Ping", TextFormat("%u ms (+-%u)", view.net.pingMs, view.net.pingVarianceMs),
		rate(static_cast<float>(view.net.pingMs), 60.0f, 120.0f));
	line("Loss (rel)", TextFormat("%.1f %%", view.net.packetLoss * 100.0f),
		rate(view.net.packetLoss, 0.01f, 0.05f));
	line("Snapshots", TextFormat("%u missed", view.snapshotsMissed),
		rate(snapshotLoss, 0.01f, 0.05f));
	line("Tick", TextFormat("%u", view.serverTick), RAYWHITE);
	line("In", TextFormat("%.1f KB/s", view.bytesPerSecondIn / 1024.0f), RAYWHITE);
	line("Out", TextFormat("%.1f KB/s", view.bytesPerSecondOut / 1024.0f), RAYWHITE);
}
#endif
