#pragma once
#include <glm/vec2.hpp>
#include <span>

namespace svkf {
struct Bitmap {
	std::span<std::byte const> bytes{};
	glm::ivec2 size{};
};
} // namespace svkf