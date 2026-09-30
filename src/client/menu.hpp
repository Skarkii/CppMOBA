#pragma once

#include <raylib.h>

struct Settings {
	bool fullscreen = false;
	bool vsync = true;
	int  fpsCap = 144;
	bool showPlayerNames = false;
	float masterVolume = 1.0f;
	KeyboardKey keyRange = KEY_X;
};

enum class MenuCategory {
	Video,
	Keybinds,
	Audio,
	Interface,
};

enum class MenuAction {
	None,
	Resume,
	Exit,
};

class Menu {
public:
	[[nodiscard]]
	bool IsOpen() const;

	void Toggle();


	[[nodiscard]]
	MenuAction Update(Settings& settings);

	[[nodiscard]]
	bool HasSettingsChanged() const;
	
	void ResetSettingsChanged();

	void SetScreenSize(const float width, const float height);
	bool IsCapturingKey() const { return m_waitingForKey != nullptr; }
private:
	bool m_open = false;
	MenuCategory m_category = MenuCategory::Video;
	float m_screenW = { 0.0f };
	float m_screenH = { 0.0f };
	KeyboardKey* m_waitingForKey = nullptr;
	bool m_settingsChanged = false;
	void drawInterface(Rectangle page, Settings& s);
	void drawVideo(Rectangle page, Settings& s);

	void drawAudio(Rectangle page, Settings& s);

	void drawKeybinds(Rectangle page, Settings& s);
};
