#include "svkf/core/render_device.hpp"
#include "svkf/core/device.hpp"
#include <chrono>

using namespace std::chrono_literals;

namespace svkf {
namespace {
auto get_framebuffer_size(GLFWwindow* window) {
	auto ret = glm::ivec2{};
	glfwGetFramebufferSize(window, &ret.x, &ret.y);
	return ret;
};

constexpr auto descriptors_per_type_v{8};
constexpr auto pool_sizes_v = std::array{
	vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, descriptors_per_type_v},
	vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, descriptors_per_type_v},
	vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, descriptors_per_type_v},
};
} // namespace

RenderDevice::RenderDevice(RenderDeviceCreateInfo const& create_info, vk::Instance instance,
						   gsl::not_null<GLFWwindow*> window)
	: m_window(window) {
	auto* ret = VkSurfaceKHR{};
	auto const result = glfwCreateWindowSurface(instance, window, nullptr, &ret);
	if (result != VK_SUCCESS || ret == VkSurfaceKHR{}) { throw std::runtime_error{"Failed to create vulkan surface"}; }
	m_surface = vk::UniqueSurfaceKHR{ret, instance};
	m_gpu = Gpu::select_gpu(instance, *m_surface);
	m_device = Device::create(m_gpu);
	m_waiter = *m_device;

	auto cmd_pool_ci = vk::CommandPoolCreateInfo{};
	cmd_pool_ci.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer).setQueueFamilyIndex(m_gpu.queue_family);
	m_cmd_pool = m_device->createCommandPoolUnique(cmd_pool_ci);

	auto cmd_buffer_ai = vk::CommandBufferAllocateInfo{};
	cmd_buffer_ai.setCommandPool(*m_cmd_pool)
		.setCommandBufferCount(resource_buffering_v)
		.setLevel(vk::CommandBufferLevel::ePrimary);
	auto const command_buffers = m_device->allocateCommandBuffers(cmd_buffer_ai);
	assert(command_buffers.size() == m_render_sync.size()); // NOLINT

	static constexpr auto fence_ci_v = vk::FenceCreateInfo{vk::FenceCreateFlagBits::eSignaled};
	for (auto [sync, cmd_buffer] : std::views::zip(m_render_sync, command_buffers)) {
		sync.command_buffer = cmd_buffer;
		sync.draw = m_device->createSemaphoreUnique({});
		sync.drawn = m_device->createFenceUnique(fence_ci_v);
	}

	m_swapchain = std::make_unique<Swapchain>(*m_device, m_gpu, *m_surface, get_framebuffer_size(window),
											  create_info.present_mode);

	m_queue = m_device->getQueue(m_gpu.queue_family, 0);

	m_allocator = vma::create_allocator(instance, m_gpu.device, *m_device);

	// create DescriptorAllocator
	auto pool_sizes = create_info.pool_sizes;
	if (pool_sizes.empty()) { pool_sizes = pool_sizes_v; }
	auto sets_per_pool = create_info.sets_per_pool;
	if (sets_per_pool == 0) { sets_per_pool = svkf::RenderDeviceCreateInfo::sets_per_pool_v; }
	m_descriptor_allocator = std::make_unique<DescriptorAllocator>(*m_device, pool_sizes, sets_per_pool);
}

auto RenderDevice::create_shader(ShaderObjectCreateInfo const& create_info) -> Shader {
	auto shader_ci = Shader::CreateInfo{
		.device = *m_device,
		.vertex_code = create_info.vertex_code,
		.fragment_code = create_info.fragment_code,
		.vertex_input = {},
		.set_layouts = create_info.set_layouts,
	};
	return Shader{shader_ci};
}

auto RenderDevice::next_frame() -> std::optional<vk::CommandBuffer> {
	auto const& sync = m_render_sync.at(m_frame_index);

	if (!get_render_target()) { return std::nullopt; }

	m_descriptor_allocator->on_next_frame(m_frame_index);

	auto command_buffer_bi = vk::CommandBufferBeginInfo{};
	command_buffer_bi.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	sync.command_buffer.begin(command_buffer_bi);

	auto dependency_info = vk::DependencyInfo{};
	auto barrier = m_swapchain->base_barrier();
	barrier.setOldLayout(vk::ImageLayout::eUndefined)
		.setNewLayout(vk::ImageLayout::eAttachmentOptimal)
		.setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite)
		.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
		.setDstAccessMask(barrier.srcAccessMask)
		.setDstStageMask(barrier.srcStageMask);
	dependency_info.setImageMemoryBarriers(barrier);
	sync.command_buffer.pipelineBarrier2(dependency_info);

	auto color_attachment = vk::RenderingAttachmentInfo{};
	color_attachment.setImageView(m_render_target->image_view)
		.setImageLayout(vk::ImageLayout::eAttachmentOptimal)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setClearValue(m_clear_color);
	auto rendering_info = vk::RenderingInfo{};
	auto const render_area = vk::Rect2D{{}, m_render_target->extent};
	rendering_info.setRenderArea(render_area).setColorAttachments(color_attachment).setLayerCount(1);

	sync.command_buffer.beginRendering(rendering_info);

	return sync.command_buffer;
}

auto RenderDevice::render(vk::CommandBuffer command_buffer) -> bool {
	command_buffer.endRendering();

	auto dependency_info = vk::DependencyInfo{};
	auto barrier = m_swapchain->base_barrier();
	barrier.setOldLayout(vk::ImageLayout::eAttachmentOptimal)
		.setNewLayout(vk::ImageLayout::ePresentSrcKHR)
		.setSrcAccessMask(vk::AccessFlagBits2::eColorAttachmentRead | vk::AccessFlagBits2::eColorAttachmentWrite)
		.setSrcStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput)
		.setDstAccessMask(barrier.srcAccessMask)
		.setDstStageMask(barrier.srcStageMask);
	dependency_info.setImageMemoryBarriers(barrier);
	command_buffer.pipelineBarrier2(dependency_info);

	auto const& sync = m_render_sync.at(m_frame_index);
	sync.command_buffer.end();

	auto submit_info = vk::SubmitInfo2{};
	auto const command_buffer_info = vk::CommandBufferSubmitInfo{sync.command_buffer};
	auto wait_semaphore_info = vk::SemaphoreSubmitInfo{};
	wait_semaphore_info.setSemaphore(*sync.draw).setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	auto signal_semaphore_info = vk::SemaphoreSubmitInfo{};
	signal_semaphore_info.setSemaphore(m_swapchain->get_present_semaphore())
		.setStageMask(vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	submit_info.setCommandBufferInfos(command_buffer_info)
		.setWaitSemaphoreInfos(wait_semaphore_info)
		.setSignalSemaphoreInfos(signal_semaphore_info);
	m_queue.submit2(submit_info, *sync.drawn);

	m_frame_index = (m_frame_index + 1) % m_render_sync.size();
	m_render_target.reset();

	if (!m_swapchain->present(m_queue) || get_framebuffer_size(m_window) != m_swapchain->get_size()) {
		m_swapchain->recreate(get_framebuffer_size(m_window));
	}

	return true;
}

auto RenderDevice::get_render_target() -> bool {
	auto framebuffer_size = get_framebuffer_size(m_window);
	if (framebuffer_size.x <= 0 || framebuffer_size.y <= 0) { return false; }

	auto& sync = m_render_sync.at(m_frame_index);
	static constexpr std::uint64_t fence_timeout_v = std::chrono::nanoseconds{3s}.count();
	auto result = m_device->waitForFences(*sync.drawn, vk::True, fence_timeout_v);
	if (result != vk::Result::eSuccess) { throw std::runtime_error{"Failed to wait for Render Fence"}; }

	m_render_target = m_swapchain->acquire_next_image(*sync.draw);
	if (!m_render_target) {
		m_swapchain->recreate(framebuffer_size);
		return false;
	}

	m_device->resetFences(*sync.drawn);
	return true;
}
} // namespace svkf
