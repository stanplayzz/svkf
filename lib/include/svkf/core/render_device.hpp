#pragma once
#include "svkf/constants.hpp"
#include "svkf/core/buffer_allocator.hpp"
#include "svkf/core/command_block.hpp"
#include "svkf/core/descriptor_allocator.hpp"
#include "svkf/core/device_waiter.hpp"
#include "svkf/core/gpu.hpp"
#include "svkf/core/shader.hpp"
#include "svkf/core/swapchain.hpp"
#include "svkf/detail/vma.hpp"
#include <memory>

namespace svkf {
struct RenderDeviceCreateInfo {
	static constexpr auto sets_per_pool_v{64};

	std::span<vk::DescriptorPoolSize const> pool_sizes{};
	std::uint32_t sets_per_pool{};
	vk::PresentModeKHR present_mode{vk::PresentModeKHR::eFifo};
};

struct ShaderObjectCreateInfo {
	std::span<std::uint32_t const> vertex_code{};
	std::span<std::uint32_t const> fragment_code{};
	std::span<vk::DescriptorSetLayout const> set_layouts{};
};

class RenderDevice {
  public:
	explicit RenderDevice(RenderDeviceCreateInfo const& create_info, vk::Instance instance, vk::SurfaceKHR surface,
						  glm::ivec2 framebuffer_size);

	[[nodiscard]] auto surface() const -> vk::SurfaceKHR { return m_surface; }
	[[nodiscard]] auto device() const -> vk::Device { return *m_device; }
	[[nodiscard]] auto gpu() const -> Gpu const& { return m_gpu; }
	[[nodiscard]] auto allocator() const -> VmaAllocator { return m_allocator.get(); }
	[[nodiscard]] auto buffer_allocator() -> BufferAllocator& { return m_buffer_allocator; }
	[[nodiscard]] auto framebuffer_size() const -> glm::ivec2 {
		return {m_render_target->extent.width, m_render_target->extent.height};
	}

	[[nodiscard]] auto create_shader(ShaderObjectCreateInfo const& create_info) -> Shader;
	[[nodiscard]] auto create_command_block() -> CommandBlock { return CommandBlock{*m_device, m_queue, *m_cmd_pool}; }
	[[nodiscard]] auto allocate_descriptor_set(std::span<vk::DescriptorSetLayout const> layouts)
		-> std::vector<vk::DescriptorSet> {
		auto ret = std::vector<vk::DescriptorSet>{layouts.size()};
		if (!m_descriptor_allocator->allocate(layouts, ret)) {
			throw std::runtime_error{"[svkf] Failed to allocate descriptor sets"};
		}
		return ret;
	}

	void set_present_mode(vk::PresentModeKHR present_mode) { m_swapchain->set_present_mode(present_mode); }
	void set_clear_color(vk::ClearColorValue color) { m_clear_color = color; }

	auto next_frame(glm::ivec2 framebuffer_size) -> std::optional<vk::CommandBuffer>;
	auto render(vk::CommandBuffer command_buffer) -> bool;

  private:
	auto get_render_target() -> bool;

	struct Sync {
		vk::UniqueSemaphore draw{};
		vk::UniqueFence drawn{};
		vk::CommandBuffer command_buffer{};
	};

	vk::SurfaceKHR m_surface{};
	Gpu m_gpu{};
	vk::UniqueDevice m_device{};
	vk::Queue m_queue{};
	vma::UniqueAllocator m_allocator{};

	glm::ivec2 m_framebuffer_size{};

	std::unique_ptr<Swapchain> m_swapchain{};

	vk::UniqueCommandPool m_cmd_pool{};
	Buffered<Sync> m_render_sync{};
	std::size_t m_frame_index{};
	std::optional<RenderTarget> m_render_target{};

	std::unique_ptr<DescriptorAllocator> m_descriptor_allocator;
	BufferAllocator m_buffer_allocator{};

	vk::ClearColorValue m_clear_color{};

	DeviceWaiter m_waiter{};
};
} // namespace svkf
