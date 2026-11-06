#pragma once
#include "svkf/constants.hpp"
#include "svkf/core/buffer.hpp"

namespace svkf {
class BufferAllocator {
  public:
	void init(BufferCreateInfo create_info) { m_create_info = create_info; }

	[[nodiscard]] auto allocate(vk::BufferUsageFlags usage, vk::DeviceSize size) -> Buffer& {
		if (size == 0) { throw std::invalid_argument{"Buffer size must be greater than zero"}; }

		auto& pool = m_pools.at(m_frame_index);
		if (pool.index == pool.buffers.size()) { pool.buffers.emplace_back(); }

		auto& slot = pool.buffers.at(pool.index++);
		if (!slot.buffer.get().buffer || slot.usage != usage || slot.buffer.get().size < size) {
			auto create_info = m_create_info;
			create_info.usage = usage;
			slot.buffer = create_buffer(create_info, BufferMemoryType::Host, size);
			if (!slot.buffer.get().buffer) { throw std::runtime_error{"Failed to allocate frame buffer"}; }
			slot.usage = usage;
		}
		return slot.buffer.get();
	}

	void on_next_frame(std::size_t frame_index) {
		m_frame_index = frame_index;
		m_pools.at(m_frame_index).index = 0;
	}

  private:
	struct Slot {
		UniqueBuffer buffer{};
		vk::BufferUsageFlags usage{};
	};
	struct Pool {
		std::vector<Slot> buffers{};
		std::size_t index{};
	};

	BufferCreateInfo m_create_info{};
	std::array<Pool, resource_buffering_v> m_pools{};
	std::size_t m_frame_index{};
};
} // namespace svkf
