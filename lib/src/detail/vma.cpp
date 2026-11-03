#define VMA_IMPLEMENTATION
#include "svkf/detail/vma.hpp"

namespace svkf::vma {
auto create_allocator(vk::Instance const instance, vk::PhysicalDevice const physical_device, vk::Device const device)
	-> UniqueAllocator {
	auto const& dispatcher = VULKAN_HPP_DEFAULT_DISPATCHER;
	auto vma_funcs = VmaVulkanFunctions{};
	vma_funcs.vkGetInstanceProcAddr = dispatcher.vkGetInstanceProcAddr;
	vma_funcs.vkGetDeviceProcAddr = dispatcher.vkGetDeviceProcAddr;

	auto allocator_ci = VmaAllocatorCreateInfo{};
	allocator_ci.physicalDevice = physical_device;
	allocator_ci.device = device;
	allocator_ci.pVulkanFunctions = &vma_funcs;
	allocator_ci.instance = instance;

	auto* ret = VmaAllocator{};
	auto const res = vmaCreateAllocator(&allocator_ci, &ret);
	if (res != VK_SUCCESS) { throw std::runtime_error{"Failed to create Vulkan Memory Allocator"}; }

	return ret;
}
} // namespace svkf::vma