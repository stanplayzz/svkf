#pragma once
#include "svkf/core/gpu.hpp"
#include "svkf/core/render_target.hpp"
#include <glm/vec2.hpp>

namespace svkf {
class Swapchain {
  public:
	explicit Swapchain(vk::Device, Gpu const& gpu, vk::SurfaceKHR surface, glm::ivec2 size,
					   vk::PresentModeKHR present_mode);

	void set_present_mode(vk::PresentModeKHR present_mode);

	auto recreate(glm::ivec2 size) -> bool;

	[[nodiscard]] auto acquire_next_image(vk::Semaphore to_signal) -> std::optional<RenderTarget>;
	[[nodiscard]] auto present(vk::Queue queue) -> bool;

	[[nodiscard]] auto get_size() const -> glm::ivec2 { return {m_ci.imageExtent.width, m_ci.imageExtent.height}; }
	[[nodiscard]] auto get_present_semaphore() const -> vk::Semaphore {
		return *m_present_semaphores.at(m_image_index.value());
	}
	[[nodiscard]] auto base_barrier() const -> vk::ImageMemoryBarrier2;

  private:
	vk::Device m_device{};
	Gpu m_gpu{};
	vk::SurfaceKHR m_surface{};

	vk::SwapchainCreateInfoKHR m_ci{};
	vk::UniqueSwapchainKHR m_swapchain{};
	std::vector<vk::Image> m_images{};
	std::vector<vk::UniqueImageView> m_image_views{};
	std::vector<vk::UniqueSemaphore> m_present_semaphores{};

	std::optional<std::size_t> m_image_index{};
};
} // namespace svkf