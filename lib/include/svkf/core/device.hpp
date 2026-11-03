#pragma once
#include "svkf/core/gpu.hpp"

namespace svkf::Device {
auto create(Gpu const&) -> vk::UniqueDevice;
}