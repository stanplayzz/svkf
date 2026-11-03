#pragma once
#include <vulkan/vulkan.hpp>

namespace svkf::Instance {
auto create() -> vk::UniqueInstance;
} // namespace svkf::Instance