#pragma once
#include <vulkan/vulkan.hpp>

namespace svkf {
inline constexpr auto vk_version_v = VK_MAKE_VERSION(1, 3, 0);

inline constexpr auto resource_buffering_v = std::size_t{2};
template <typename Type>
using Buffered = std::array<Type, resource_buffering_v>;
} // namespace svkf