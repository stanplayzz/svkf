#pragma once
#include "svkf/core/bitmap.hpp"
#include "svkf/core/command_block.hpp"
#include <klib/unique.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace svkf {
struct Image {
	auto operator==(Image const& rhs) const -> bool = default;

	VmaAllocator allocator{};
	VmaAllocation allocation{};
	vk::Image image{};
	vk::Extent2D extent{};
	vk::Format format{};
	std::uint32_t levels{};
};

struct ImageDeleter {
	void operator()(Image const& image) const noexcept {
		vmaDestroyImage(image.allocator, image.image, image.allocation);
	}
};

using UniqueImage = klib::Unique<Image, ImageDeleter>;

struct ImageCreateInfo {
	VmaAllocator allocator;
	std::uint32_t queue_family;
};

[[nodiscard]] auto create_image(ImageCreateInfo const& create_info, vk::ImageUsageFlags usage, std::uint32_t levels,
								vk::Format format, vk::Extent2D extent) -> UniqueImage;

[[nodiscard]] auto create_sampled_image(ImageCreateInfo const& create_info, CommandBlock command_block,
											Bitmap const& bitmap) -> UniqueImage;
} // namespace svkf
