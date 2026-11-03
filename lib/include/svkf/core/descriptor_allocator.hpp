#pragma once
#include "svkf/constants.hpp"
#include <vulkan/vulkan.hpp>
#include <print>

namespace svkf {
class DescriptorPool {
  public:
	void init(vk::Device device, std::span<vk::DescriptorPoolSize const> pool_sizes, std::uint32_t sets_per_pool) {
		m_device = device;
		m_pool_sizes = pool_sizes;
		m_sets_per_pool = sets_per_pool;
	}

	void reset() {
		for (auto& pool : m_pools) { m_device.resetDescriptorPool(*pool); }
		m_index = 0;
	}

	auto allocate(std::span<vk::DescriptorSetLayout const> layouts, std::span<vk::DescriptorSet> out_sets) -> bool {
		assert(m_device && !m_pool_sizes.empty() && m_sets_per_pool > 0 && out_sets.size() == layouts.size()); // NOLINT

		auto const pool = get_pool();
		auto allocate_info = vk::DescriptorSetAllocateInfo{};
		allocate_info.setDescriptorPool(pool).setSetLayouts(layouts);
		auto result = m_device.allocateDescriptorSets(&allocate_info, out_sets.data());
		switch (result) {
		case vk::Result::eSuccess: return true;
		case vk::Result::eErrorOutOfPoolMemory:
			++m_index;
			allocate_info.setDescriptorPool(get_pool());
			return m_device.allocateDescriptorSets(&allocate_info, out_sets.data()) == vk::Result::eSuccess;
		default: return false;
		}
	}

  private:
	auto get_pool() -> vk::DescriptorPool {
		if (m_index > 4096) { std::println("[SVKF] Warning: {} DescriptorPools allocated this frame", m_index); }
		if (m_index >= m_pools.size()) {
			m_index = m_pools.size();
			auto pool_ci = vk::DescriptorPoolCreateInfo{};
			pool_ci.setPoolSizes(m_pool_sizes).setMaxSets(m_sets_per_pool);
			m_pools.push_back(m_device.createDescriptorPoolUnique(pool_ci));
		}
		return *m_pools.at(m_index);
	}

	vk::Device m_device{};
	std::span<vk::DescriptorPoolSize const> m_pool_sizes{};
	std::uint32_t m_sets_per_pool{};
	std::vector<vk::UniqueDescriptorPool> m_pools{};

	std::size_t m_index{};
};

class DescriptorAllocator {
  public:
	explicit DescriptorAllocator(vk::Device device, std::span<vk::DescriptorPoolSize const> pool_sizes,
								 std::uint32_t const sets_per_pool) {
		for (auto& pool : m_pools) { pool.init(device, pool_sizes, sets_per_pool); }
	}

	auto allocate(std::span<vk::DescriptorSetLayout const> layouts, std::span<vk::DescriptorSet> out_sets) -> bool {
		return m_pools.at(m_frame_index).allocate(layouts, out_sets);
	}

	void on_next_frame(std::size_t const frame_index) {
		m_frame_index = frame_index;
		m_pools.at(m_frame_index).reset();
	}

  private:
	std::array<DescriptorPool, resource_buffering_v> m_pools{};
	std::size_t m_frame_index{};
};
} // namespace svkf