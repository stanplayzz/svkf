#include "svkf/core/buffer.hpp"
#include <numeric>
#include <print>

namespace svkf {
auto create_buffer(BufferCreateInfo const& create_info, BufferMemoryType const memory_type, vk::DeviceSize size)
	-> UniqueBuffer {
	if (size == 0) {
		std::println(stderr, "Buffer cannot be 0-sized");
		return {};
	}

	auto allocation_ci = VmaAllocationCreateInfo{};
	allocation_ci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
	auto usage = create_info.usage;
	if (memory_type == BufferMemoryType::Host) {
		allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
		allocation_ci.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
	} else {
		allocation_ci.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
		usage |= vk::BufferUsageFlagBits::eTransferDst;
	}

	auto buffer_ci = vk::BufferCreateInfo{};
	buffer_ci.setQueueFamilyIndices(create_info.queue_family).setSize(size).setUsage(usage);
	auto vma_buffer_ci = static_cast<VkBufferCreateInfo>(buffer_ci);

	auto* allocation = VmaAllocation{};
	auto* buffer = VkBuffer{};
	auto allocation_info = VmaAllocationInfo{};
	auto const result =
		vmaCreateBuffer(create_info.allocator, &vma_buffer_ci, &allocation_ci, &buffer, &allocation, &allocation_info);
	if (result != VK_SUCCESS) {
		std::println(stderr, "Failed to create VMA Buffer");
		return {};
	}

	return Buffer{
		.allocator = create_info.allocator,
		.allocation = allocation,
		.buffer = buffer,
		.size = size,
		.mapped = allocation_info.pMappedData,
	};
}

auto create_device_buffer(BufferCreateInfo const& create_info, CommandBlock command_block, ByteSpans const& byte_spans)
	-> UniqueBuffer {
	auto const size = std::accumulate(byte_spans.begin(), byte_spans.end(), 0uz,
									  [](std::size_t const n, std::span<std::byte const> bytes) {
										  return n + bytes.size();
									  });

	auto staging_ci = create_info;
	staging_ci.usage = vk::BufferUsageFlagBits::eTransferSrc;

	auto staging_buffer = create_buffer(staging_ci, BufferMemoryType::Host, size);
	auto ret = create_buffer(create_info, BufferMemoryType::Device, size);
	if (!staging_buffer.get().buffer || !ret.get().buffer) { return {}; }

	auto dst = staging_buffer.get().span();
	for (auto const bytes : byte_spans) {
		std::memcpy(dst.data(), bytes.data(), bytes.size());
		dst = dst.subspan(bytes.size());
	}

	auto buffer_copy = vk::BufferCopy2{};
	buffer_copy.setSize(size);
	auto copy_buffer_info = vk::CopyBufferInfo2{};
	copy_buffer_info.setSrcBuffer(staging_buffer.get().buffer).setDstBuffer(ret.get().buffer).setRegions(buffer_copy);
	command_block.command_buffer().copyBuffer2(copy_buffer_info);
	command_block.submit_and_wait();

	return ret;
}
} // namespace svkf
