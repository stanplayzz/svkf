#include "svkf/core/device.hpp"

namespace svkf::Device {
auto create(Gpu const& gpu) -> vk::UniqueDevice {
	auto queue_ci = vk::DeviceQueueCreateInfo{};
	static constexpr auto queue_priorities_v = std::array{1.0f};
	queue_ci.setQueueFamilyIndex(gpu.queue_family).setQueueCount(1).setQueuePriorities(queue_priorities_v);

	auto enabled_features = vk::PhysicalDeviceFeatures{};
	enabled_features.fillModeNonSolid = gpu.features.fillModeNonSolid;
	enabled_features.wideLines = gpu.features.wideLines;
	enabled_features.samplerAnisotropy = gpu.features.samplerAnisotropy;
	enabled_features.sampleRateShading = gpu.features.sampleRateShading;

	auto sync_feature = vk::PhysicalDeviceSynchronization2Features{vk::True};
	auto dynamic_rendering_feature = vk::PhysicalDeviceDynamicRenderingFeatures{vk::True};
	auto shader_object_feature = vk::PhysicalDeviceShaderObjectFeaturesEXT{vk::True};
	dynamic_rendering_feature.setPNext(&shader_object_feature);
	sync_feature.setPNext(&dynamic_rendering_feature);

	auto device_ci = vk::DeviceCreateInfo{};
	static constexpr auto extensions_v = std::array{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		"VK_EXT_shader_object",
	};
	device_ci.setPEnabledExtensionNames(extensions_v)
		.setQueueCreateInfos(queue_ci)
		.setPEnabledFeatures(&enabled_features)
		.setPNext(&sync_feature);

	auto device = gpu.device.createDeviceUnique(device_ci);
	VULKAN_HPP_DEFAULT_DISPATCHER.init();
	return device;
};
} // namespace svkf::Device