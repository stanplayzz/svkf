#pragma once
#include <klib/unique.hpp>
#include <vulkan/vulkan.hpp>

namespace svkf {
struct DeviceWaiterDeleter {
	void operator()(vk::Device const device) const noexcept { device.waitIdle(); }
};

using DeviceWaiter = klib::Unique<vk::Device, DeviceWaiterDeleter>;
} // namespace svkf