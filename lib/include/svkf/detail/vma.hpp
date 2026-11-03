#pragma once
#include <klib/unique.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace svkf::vma {
struct AllocatorDeleter {
	void operator()(VmaAllocator allocator) const noexcept { vmaDestroyAllocator(allocator); }
};

using UniqueAllocator = klib::Unique<VmaAllocator, AllocatorDeleter>;

[[nodiscard]] auto create_allocator(vk::Instance instance, vk::PhysicalDevice physical_device, vk::Device device)
	-> UniqueAllocator;
} // namespace svkf::vma