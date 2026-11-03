#pragma once
#include <filesystem>
#include <fstream>
#include <vector>

namespace svkf {
[[nodiscard]] inline auto to_spir_v(std::filesystem::path const& path) -> std::vector<std::uint32_t> {
	auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
	if (!file.is_open()) { throw std::runtime_error{std::format("Failed to open file: {}", path.generic_string())}; }

	auto const size = file.tellg();
	auto const usize = static_cast<std::uint64_t>(size);
	if (usize % sizeof(std::uint32_t) != 0) { throw std::runtime_error{std::format("Invalid SPIR-V size: {}", usize)}; }

	file.seekg({}, std::ios::beg);
	auto ret = std::vector<std::uint32_t>{};
	ret.resize(usize / sizeof(std::uint32_t));
	void* data = ret.data();
	file.read(static_cast<char*>(data), size);
	return ret;
}
} // namespace svkf