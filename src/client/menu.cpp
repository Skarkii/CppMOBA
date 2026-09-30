#include "menu.hpp"
#include <algorithm>
#include <array>

bool Menu::IsOpen() const {
	return m_open;
}

void Menu::Toggle() {
	m_open = !m_open;
}

bool Menu::HasSettingsChanged() const {
    return m_settingsChanged;
}

void Menu::ResetSettingsChanged() {
    m_settingsChanged = false;
}

void Menu::SetScreenSize(const float width, const float height) {
	m_screenW = width;
	m_screenH = height;
}

bool Button(Rectangle r, const char* text) {
	const bool hovered = CheckCollisionPointRec(GetMousePosition(), r);
	DrawRectangleRounded(r, 0.2f, 6, hovered ? Color{ 70, 70, 90, 255 } : Color{ 45, 45, 60, 255 });
	DrawRectangleRoundedLinesEx(r, 0.2f, 6, 2.0f, hovered ? GOLD : GRAY);

	const int w = MeasureText(text, 20);
	DrawText(text, static_cast<int>(r.x + (r.width - w) / 2), static_cast<int>(r.y + (r.height - 20) / 2), 20, RAYWHITE);

	return hovered && IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

void Heading(float x, float& y, const char* text) {
    DrawText(text, static_cast<int>(x), static_cast<int>(y), 28, RAYWHITE);
    y += 44.0f;
}

void Label(float x, float y, const char* text) {
    DrawText(text, static_cast<int>(x), static_cast<int>(y + 8), 20, LIGHTGRAY);
}

bool ToggleRow(Rectangle page, float& y, const char* label, bool& value) {
    Label(page.x, y, label);
    const bool clicked = Button({ page.x + page.width - 120, y, 120, 36 }, value ? "On" : "Off");
    if (clicked)
        value = !value;
    y += 48.0f;
    return clicked;
}

bool SliderRow(Rectangle page, float& y, const char* label, float& value) {
    Label(page.x, y, label);
    const Rectangle bar = { page.x + page.width - 220, y + 10, 220, 16 };

    DrawRectangleRounded(bar, 0.5f, 6, Color{ 45, 45, 60, 255 });
    DrawRectangleRounded({ bar.x, bar.y, bar.width * value, bar.height }, 0.5f, 6, GOLD);
    DrawText(TextFormat("%d%%", static_cast<int>(value * 100.0f)),
        static_cast<int>(bar.x - 60), static_cast<int>(y + 8), 20, RAYWHITE);

    bool changed = false;
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), bar)) {
        value = std::clamp((GetMousePosition().x - bar.x) / bar.width, 0.0f, 1.0f);
        changed = true;
    }
    y += 48.0f;
    return changed;
}

const char* KeyName(KeyboardKey key) {
    if (key >= KEY_A && key <= KEY_Z)       return TextFormat("%c", static_cast<char>(key));
    if (key >= KEY_ZERO && key <= KEY_NINE) return TextFormat("%c", static_cast<char>(key));
    switch (key) {
    case KEY_SPACE:        return "Space";
    case KEY_TAB:          return "Tab";
    case KEY_LEFT_SHIFT:   return "Shift";
    case KEY_LEFT_CONTROL: return "Ctrl";
    case KEY_LEFT_ALT:     return "Alt";
    default:               return TextFormat("Key %d", static_cast<int>(key));
    }
}

void Menu::drawInterface(Rectangle page, Settings& s) {
    float y = page.y;
    Heading(page.x, y, "Interface");

    if (ToggleRow(page, y, "Show Player Names", s.showPlayerNames))
        m_settingsChanged = true;
}

void Menu::drawVideo(Rectangle page, Settings& s) {
    float y = page.y;
    Heading(page.x, y, "Video");

    if (ToggleRow(page, y, "Fullscreen", s.fullscreen))
        m_settingsChanged = true;

    if (ToggleRow(page, y, "VSync", s.vsync))
        m_settingsChanged = true;

    constexpr std::array<int, 4> caps = { 60, 144, 240, 0 };   // 0 = unlimited
    Label(page.x, y, "FPS limit");
    const char* text = s.fpsCap == 0 ? "Unlimited" : TextFormat("%d", s.fpsCap);
    if (Button({ page.x + page.width - 120, y, 120, 36 }, text)) {
        auto it = std::find(caps.begin(), caps.end(), s.fpsCap);
        s.fpsCap = (it == caps.end() || it + 1 == caps.end()) ? caps[0] : *(it + 1);
        m_settingsChanged = true;
    }
    y += 48.0f;
}

void Menu::drawAudio(Rectangle page, Settings& s) {
    float y = page.y;
    Heading(page.x, y, "Audio");

    if (SliderRow(page, y, "Master volume", s.masterVolume))
        m_settingsChanged = true;
}

void Menu::drawKeybinds(Rectangle page, Settings& s) {
    float y = page.y;
    Heading(page.x, y, "Keybinds");

    // While waiting, the next key pressed becomes the new binding.
    if (m_waitingForKey) {
        const int key = GetKeyPressed();
        if (key == KEY_ESCAPE)
            m_waitingForKey = nullptr;
        else if (key != 0) {
            *m_waitingForKey = static_cast<KeyboardKey>(key);
            m_waitingForKey = nullptr;
        }
    }

    auto bindRow = [&](const char* label, KeyboardKey& key) {
        Label(page.x, y, label);
        const bool waiting = (m_waitingForKey == &key);
        if (Button({ page.x + page.width - 120, y, 120, 36 }, waiting ? "Press key..." : KeyName(key)))
            m_waitingForKey = &key;
        y += 48.0f;
        };

    bindRow("Show attack range", s.keyRange);
    // later: bindRow("Ability Q", s.keyQ); ...
}

MenuAction Menu::Update(Settings& settings) {
	DrawRectangleRec({ 0, 0, m_screenW, m_screenH }, Fade(BLACK, 0.5f));

	const Rectangle box = { (m_screenW - 700) / 2.0f, (m_screenH - 450) / 2.0f, 700, 450 };
	DrawRectangleRounded(box, 0.05f, 8, Color{ 25, 25, 35, 240 });

	if (Button({ box.x + 20, box.y + 20,  150, 40 }, "Video"))    
		m_category = MenuCategory::Video;

	if (Button({ box.x + 20, box.y + 70,  150, 40 }, "Keybinds")) 
		m_category = MenuCategory::Keybinds;

	if (Button({ box.x + 20, box.y + 120, 150, 40 }, "Audio"))
		m_category = MenuCategory::Audio;

	if (Button({ box.x + 20, box.y + 170, 150, 40 }, "Interface"))
		m_category = MenuCategory::Interface;

	if (Button({ box.x + 20, box.y + box.height - 110, 150, 40 }, "Resume")) 
		return MenuAction::Resume;

	if (Button({ box.x + 20, box.y + box.height - 60,  150, 40 }, "Exit"))
		return MenuAction::Exit;

	const Rectangle page = { box.x + 190, box.y + 20, box.width - 210, box.height - 40 };
	switch (m_category) {
	case MenuCategory::Video:     drawVideo(page, settings);       break;
	case MenuCategory::Keybinds:  drawKeybinds(page, settings);    break;
	case MenuCategory::Audio:     drawAudio(page, settings);       break;
	case MenuCategory::Interface: drawInterface(page, settings);   break;
	}
	return MenuAction::None;
}

