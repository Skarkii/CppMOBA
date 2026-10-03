#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

enum class Tile : std::uint8_t { Wall, Jungle, Lane, River, Bush, BlueBase, RedBase };

class MapGrid {
public:
	[[nodiscard]]
	bool Load(const std::filesystem::path& file) {
		std::ifstream in(file);
		if (!in)
			return false;

		m_tiles.clear();
		m_width = 0;
		m_height = 0;

		std::string line;
		while (std::getline(in, line)) {
			if (!line.empty() && line.back() == '\r')
				line.pop_back();
			if (line.empty())
				continue;

			if (m_width == 0)
				m_width = static_cast<int>(line.size());
			else if (static_cast<int>(line.size()) != m_width)
				return false;

			for (const char c : line)
				m_tiles.push_back(fromChar(c));
			m_height++;
		}
		return m_height > 0;
	}

	[[nodiscard]]
	int Width() const { return m_width; }

	[[nodiscard]]
	int Height() const { return m_height; }

	[[nodiscard]]
	bool InBounds(int cellX, int cellZ) const {
		return cellX >= 0 && cellZ >= 0 && cellX < m_width && cellZ < m_height;
	}

	[[nodiscard]]
	Tile At(int cellX, int cellZ) const {
		if (!InBounds(cellX, cellZ))
			return Tile::Wall;
		return m_tiles[static_cast<std::size_t>(cellZ) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(cellX)];
	}

	[[nodiscard]]
	bool IsWalkable(int cellX, int cellZ) const {
		return At(cellX, cellZ) != Tile::Wall;
	}

	[[nodiscard]]
	Tile AtWorld(float x, float z) const {
		return At(toCell(x), toCell(z));
	}

	[[nodiscard]]
	bool IsWalkableWorld(float x, float z) const {
		return AtWorld(x, z) != Tile::Wall;
	}

	[[nodiscard]]
	static int toCell(float world) {
		return world < 0.0f ? -1 : static_cast<int>(world);
	}

private:
	[[nodiscard]]
	static Tile fromChar(char c) {
		switch (c) {
		case '.': return Tile::Jungle;
		case '=': return Tile::Lane;
		case '~': return Tile::River;
		case '*': return Tile::Bush;
		case 'b': return Tile::BlueBase;
		case 'r': return Tile::RedBase;
		default:  return Tile::Wall;
		}
	}

	std::vector<Tile> m_tiles;
	int m_width = 0;
	int m_height = 0;
};
