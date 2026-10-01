#pragma once

#include <raylib.h>

#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <string_view>

#include "protocol.hpp"

struct ChatInput
{
	std::string text;
	protocol::TextScope scope = protocol::TextScope::Team;
};

class Chat
{
public:
	[[nodiscard]] 
	bool IsTyping() const { return m_typing; }

	[[nodiscard]] 
	std::optional<ChatInput> Update();

	void AddMessage(std::string_view senderName, std::string_view champName, protocol::TextScope allChat, std::string_view text);

	void AddSystemMessage(std::string_view text);

	void Draw(float bottomY) const;

private:
	struct Line
	{
		std::string sender;
		protocol::TextScope scope;
		std::string text;
		double receivedAt;
	};

	void addLine(Line line);

	std::deque<Line> m_history;
	std::string m_input;
	bool m_typing = false;

	static constexpr std::size_t kMaxHistory = 50;
	static constexpr std::size_t kMaxVisible = 8;
	static constexpr std::size_t kMaxInput = 200;
	static constexpr double kFadeAfter = 8.0;
	static constexpr double kFadeDuration = 1.0;
};
