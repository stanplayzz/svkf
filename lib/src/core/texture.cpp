#include "svkf/core/texture.hpp"
#include <limits>

namespace svkf {
namespace {
constexpr auto invalid_pixels_v = std::array{
	std::byte{0xff}, std::byte{},	  std::byte{0xff}, std::byte{0xff}, std::byte{}, std::byte{},
	std::byte{},	 std::byte{0xff}, std::byte{},	   std::byte{},		std::byte{}, std::byte{0xff},
	std::byte{0xff}, std::byte{},	  std::byte{0xff}, std::byte{0xff},
};
constexpr auto invalid_bitmap_v = Bitmap{.bytes = invalid_pixels_v, .size = {2, 2}};

[[nodiscard]] auto is_valid_bitmap(Bitmap const& bitmap) -> bool {
	if (bitmap.size.x <= 0 || bitmap.size.y <= 0) { return false; }
	auto const width = static_cast<std::size_t>(bitmap.size.x);
	auto const height = static_cast<std::size_t>(bitmap.size.y);
	if (width > std::numeric_limits<std::size_t>::max() / height) { return false; }
	auto const pixels = width * height;
	if (pixels > std::numeric_limits<std::size_t>::max() / 4) { return false; }
	return bitmap.bytes.size_bytes() == pixels * 4;
}
} // namespace

Texture::Texture(CreateInfo create_info) {
	if (!is_valid_bitmap(create_info.bitmap)) { create_info.bitmap = invalid_bitmap_v; }

	auto const image_ci = ImageCreateInfo{
		.allocator = create_info.allocator,
		.queue_family = create_info.queue_family,
	};
	m_image = create_sampled_image(image_ci, std::move(create_info.command_block), create_info.bitmap);

	auto image_view_ci = vk::ImageViewCreateInfo{};
	auto subresource_range = vk::ImageSubresourceRange{};
	subresource_range.setAspectMask(vk::ImageAspectFlagBits::eColor)
		.setLayerCount(1)
		.setLevelCount(m_image.get().levels);

	image_view_ci.setImage(m_image.get().image)
		.setViewType(vk::ImageViewType::e2D)
		.setFormat(m_image.get().format)
		.setSubresourceRange(subresource_range);
	m_view = create_info.device.createImageViewUnique(image_view_ci);

	m_sampler = create_info.device.createSamplerUnique(create_info.sampler);
}

auto Texture::descriptor_info() const -> vk::DescriptorImageInfo {
	auto ret = vk::DescriptorImageInfo{};
	ret.setImageView(*m_view).setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal).setSampler(*m_sampler);
	return ret;
}
} // namespace svkf
