#pragma once
#include "svkf/core/command_block.hpp"
#include "svkf/detail/vma.hpp"

namespace svkf {
struct Buffer {
	[[nodiscard]] auto span() const -> std::span<std::byte> { return std::span{static_cast<std::byte*>(mapped), size}; }
	auto operator==(Buffer const& rhs) const -> bool = default;

	VmaAllocator allocator{};
	VmaAllocation allocation{};
	vk::Buffer buffer{};
	vk::DeviceSize size{};
	void* mapped{};
};

struct BufferDeleter {
	void operator()(Buffer const& buffer) const noexcept {
		vmaDestroyBuffer(buffer.allocator, buffer.buffer, buffer.allocation);
	}
};

using UniqueBuffer = klib::Unique<Buffer, BufferDeleter>;

struct BufferCreateInfo {
	VmaAllocator allocator;
	vk::BufferUsageFlags usage;
	std::uint32_t queue_family;
};

enum class BufferMemoryType : std::int8_t { Host, Device };

[[nodiscard]] auto create_buffer(BufferCreateInfo const& create_info, BufferMemoryType memory_type, vk::DeviceSize size)
	-> UniqueBuffer;

using ByteSpans = std::span<std::span<std::byte const> const>;
[[nodiscard]] auto create_device_buffer(BufferCreateInfo const& create_info, CommandBlock command_block,
											ByteSpans const& byte_spans) -> UniqueBuffer;
} // namespace svkf
