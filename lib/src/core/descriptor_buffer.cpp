#include "svkf/core/descriptor_buffer.hpp"

namespace svkf {
DescriptorBuffer::DescriptorBuffer(VmaAllocator allocator, std::uint32_t const queue_family, vk::BufferUsageFlags usage)
	: m_allocator(allocator), m_queue_family(queue_family), m_usage(usage) {
	for (auto& buffer : m_buffers) { write_to(buffer, {}); }
}

void DescriptorBuffer::write_at(std::size_t const frame_index, std::span<std::byte const> bytes) {
	write_to(m_buffers.at(frame_index), bytes);
}

auto DescriptorBuffer::descriptor_info_at(std::size_t const frame_index) const -> vk::DescriptorBufferInfo {
	auto const& buffer = m_buffers.at(frame_index);
	auto ret = vk::DescriptorBufferInfo{};
	ret.setBuffer(buffer.buffer.get().buffer).setRange(buffer.size);
	return ret;
}

void DescriptorBuffer::write_to(Buffer& out, std::span<std::byte const> bytes) const {
	static constexpr auto blank_bytes_v = std::array{std::byte{}};
	if (bytes.empty()) { bytes = blank_bytes_v; }
	out.size = bytes.size();
	if (out.buffer.get().size < bytes.size()) {
		auto const buffer_ci = BufferCreateInfo{
			.allocator = m_allocator,
			.usage = m_usage,
			.queue_family = m_queue_family,
		};
		out.buffer = create_buffer(buffer_ci, BufferMemoryType::Host, out.size);
	}
	std::memcpy(out.buffer.get().mapped, bytes.data(), bytes.size());
}
} // namespace svkf