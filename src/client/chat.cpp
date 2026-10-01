#include "chat.hpp"

#include <algorithm>
#include <utility>
#include <format>

namespace
{
	constexpr int kFontSize = 18;
	constexpr int kLineHeight = kFontSize + 4;
	constexpr int kLeft = 12;
	constexpr int kWidth = 420;

	constexpr std::string_view kAllPrefix = "/all ";
}

std::optional<ChatInput> Chat::Update()
{
	if (!m_typing)
	{
		if (IsKeyPressed(KEY_ENTER))
		{
			m_typing = true;
			while (GetCharPressed() != 0) {}
		}
		return std::nullopt;
	}

	for (int c = GetCharPressed(); c != 0; c = GetCharPressed())
	{
		if (c >= 32 && c < 127 && m_input.size() < kMaxInput)
			m_input.push_back(static_cast<char>(c));
	}

	if ((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) && !m_input.empty())
		m_input.pop_back();

	if (IsKeyPressed(KEY_ESCAPE))
	{
		m_input.clear();
		m_typing = false;
		return std::nullopt;
	}

	if (IsKeyPressed(KEY_ENTER))
	{
		m_typing = false;

		ChatInput input;
		input.text = std::exchange(m_input, {});

		if (input.text.starts_with(kAllPrefix))
		{
			input.scope = protocol::TextScope::All;
			input.text.erase(0, kAllPrefix.size());
		}

		if (input.text.find_first_not_of(' ') == std::string::npos)
			return std::nullopt;

		return input;
	}

	return std::nullopt;
}

void Chat::AddMessage(std::string_view senderName, std::string_view champName, protocol::TextScope scope, std::string_view text)
{
	std::string senderFull = std::format("{} ({})", senderName, champName);
	addLine({ senderFull, scope, std::string(text), GetTime()});
}

void Chat::AddSystemMessage(std::string_view text)
{
	addLine({ {}, protocol::TextScope::All, std::string(text), GetTime() });
}

void Chat::addLine(Line line)
{
	m_history.push_back(std::move(line));
	while (m_history.size() > kMaxHistory)
		m_history.pop_front();
}

void Chat::Draw(float bottomY) const
{
	const double now = GetTime();
	int y = static_cast<int>(bottomY);

	if (m_typing)
	{
		y -= kLineHeight + 6;
		DrawRectangle(kLeft - 6, y - 3, kWidth, kLineHeight + 6, Fade(BLACK, 0.7f));
		DrawRectangleLines(kLeft - 6, y - 3, kWidth, kLineHeight + 6, GRAY);

		const bool all = std::string_view(m_input).starts_with(kAllPrefix);
		const char* label = all ? "[All] " : "[Team] ";
		DrawText(label, kLeft, y, kFontSize, all ? ORANGE : SKYBLUE);
		int x = kLeft + MeasureText(label, kFontSize);

		DrawText(m_input.c_str(), x, y, kFontSize, RAYWHITE);
		x += MeasureText(m_input.c_str(), kFontSize);

		if (static_cast<int>(now * 2.0) % 2 == 0)
			DrawRectangle(x + 1, y, 2, kFontSize, RAYWHITE);

		y -= 6;
	}

	const std::size_t count = std::min(m_history.size(), kMaxVisible);
	for (std::size_t i = 0; i < count; i++)
	{
		const Line& line = m_history[m_history.size() - 1 - i];

		float alpha = 1.0f;
		if (!m_typing)
		{
			const double age = now - line.receivedAt;
			if (age > kFadeAfter + kFadeDuration)
				continue;
			if (age > kFadeAfter)
				alpha = static_cast<float>(1.0 - (age - kFadeAfter) / kFadeDuration);
		}

		y -= kLineHeight;
		int x = kLeft;

		DrawRectangle(kLeft - 6, y - 2, kWidth, kLineHeight, Fade(BLACK, 0.35f * alpha));

		if (line.sender.empty())
		{
			DrawText(line.text.c_str(), x, y, kFontSize, Fade(BLACK, alpha));
			continue;
		}

		if (line.scope == protocol::TextScope::All)
		{
			DrawText("[All] ", x, y, kFontSize, Fade(ORANGE, alpha));
			x += MeasureText("[All] ", kFontSize);
		}

		const char* name = TextFormat("%s: ", line.sender.c_str());
		DrawText(name, x, y, kFontSize, Fade(BLACK, alpha));
		x += MeasureText(name, kFontSize);

		DrawText(line.text.c_str(), x, y, kFontSize, Fade(RAYWHITE, alpha));
	}
}
