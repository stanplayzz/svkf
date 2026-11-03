#include "svkf/core/buffer.hpp"
#include "svkf/core/command_block.hpp"
#include "svkf/core/instance.hpp"
#include "svkf/core/render_device.hpp"
#include "svkf/core/texture.hpp"
#include "svkf/detail/spirv.hpp"
#include <GLFW/glfw3.h>
#include <array>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <print>
#include <stdexcept>

namespace {
struct WindowDeleter {
	void operator()(GLFWwindow* window) const noexcept { glfwDestroyWindow(window); }
};
using Window = std::unique_ptr<GLFWwindow, WindowDeleter>;

auto create_window() -> Window {
	glfwSetErrorCallback([](int code, char const* description) {
		std::println(stderr, "[GLFW] Error {}: {}", code, description);
	});
	if (glfwInit() != GLFW_TRUE) { throw std::runtime_error{"Failed to initialize GLFW"}; }
	if (glfwVulkanSupported() != GLFW_TRUE) { throw std::runtime_error{"GLFW could not find Vulkan"}; }
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	Window window{glfwCreateWindow(960, 540, "SVKF textured quad", nullptr, nullptr)};
	if (!window) { throw std::runtime_error{"Failed to create GLFW window"}; }
	return window;
}

[[nodiscard]] auto locate_assets_dir() -> std::filesystem::path {
	for (auto path = std::filesystem::current_path(); !path.empty() && path.has_parent_path();
		 path = path.parent_path()) {
		auto const assets = path / "assets";
		if (std::filesystem::is_directory(assets)) { return assets; }
	}
	throw std::runtime_error{"Could not locate the assets directory"};
}

struct Vertex {
	std::array<float, 2> position{};
	std::array<float, 2> uv{};
};

constexpr auto vertices_v = std::array{
	Vertex{.position = {-0.65f, -0.65f}, .uv = {0.0f, 1.0f}},
	Vertex{.position = {0.65f, -0.65f}, .uv = {1.0f, 1.0f}},
	Vertex{.position = {0.65f, 0.65f}, .uv = {1.0f, 0.0f}},
	Vertex{.position = {-0.65f, 0.65f}, .uv = {0.0f, 0.0f}},
};
constexpr auto indices_v = std::array<std::uint32_t, 6>{0, 1, 2, 2, 3, 0};

constexpr auto checker_pixels_v = std::array{
	std::byte{0xff}, std::byte{0x60}, std::byte{0x30}, std::byte{0xff}, std::byte{0xff}, std::byte{0xe0},
	std::byte{0x40}, std::byte{0xff}, std::byte{0x30}, std::byte{0x70}, std::byte{0xff}, std::byte{0xff},
	std::byte{0x30}, std::byte{0xd0}, std::byte{0xff}, std::byte{0xff},
};

} // namespace

auto main() -> int {
	try {
		auto window = create_window();
		auto instance = svkf::Instance::create();
		auto render_device = svkf::RenderDevice{{}, *instance, window.get()};
		auto const device = render_device.device();
		auto const queue = device.getQueue(render_device.gpu().queue_family, 0);

		auto upload_pool_ci = vk::CommandPoolCreateInfo{};
		upload_pool_ci.setFlags(vk::CommandPoolCreateFlagBits::eTransient)
			.setQueueFamilyIndex(render_device.gpu().queue_family);
		auto upload_pool = device.createCommandPoolUnique(upload_pool_ci);

		auto const vertex_bytes = std::span<std::byte const>{std::as_bytes(std::span{vertices_v})};
		auto const index_bytes = std::span<std::byte const>{std::as_bytes(std::span{indices_v})};
		auto const quad_bytes = std::array{vertex_bytes, index_bytes};
		auto const buffer_ci = svkf::BufferCreateInfo{
			.allocator = render_device.allocator(),
			.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
			.queue_family = render_device.gpu().queue_family,
		};
		auto quad_buffer =
			svkf::create_device_buffer(buffer_ci, svkf::CommandBlock{device, queue, *upload_pool}, quad_bytes);

		auto const bitmap = svkf::Bitmap{.bytes = checker_pixels_v, .size = {2, 2}};
		auto texture = svkf::Texture{svkf::TextureCreateInfo{
			.device = device,
			.allocator = render_device.allocator(),
			.queue_family = render_device.gpu().queue_family,
			.command_block = svkf::CommandBlock{device, queue, *upload_pool},
			.bitmap = bitmap,
			.sampler = svkf::create_sampler_ci(vk::SamplerAddressMode{}, vk::Filter::eNearest),
		}};

		auto texture_binding = vk::DescriptorSetLayoutBinding{};
		texture_binding.setBinding(0)
			.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
			.setDescriptorCount(1)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);
		auto layout_ci = vk::DescriptorSetLayoutCreateInfo{};
		layout_ci.setBindings(texture_binding);
		auto texture_layout = device.createDescriptorSetLayoutUnique(layout_ci);
		auto const set_layouts = std::array{*texture_layout};

		auto const assets = locate_assets_dir();
		auto const vertex_spirv = svkf::to_spir_v(assets / "shaders/sprite.vert.spv");
		auto const fragment_spirv = svkf::to_spir_v(assets / "shaders/sprite.frag.spv");
		auto const vertex_attributes = std::array{
			vk::VertexInputAttributeDescription2EXT{0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, position)},
			vk::VertexInputAttributeDescription2EXT{1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)},
		};
		auto const vertex_bindings =
			std::array{vk::VertexInputBindingDescription2EXT{0, sizeof(Vertex), vk::VertexInputRate::eVertex, 1}};
		auto const shader_ci = svkf::Shader::CreateInfo{
			.device = device,
			.vertex_code = vertex_spirv,
			.fragment_code = fragment_spirv,
			.vertex_input = {.attributes = vertex_attributes, .bindings = vertex_bindings},
			.set_layouts = set_layouts,
		};
		auto shader = svkf::Shader{shader_ci};
		auto const pipeline_layout_ci = vk::PipelineLayoutCreateInfo{}.setSetLayouts(set_layouts);
		auto pipeline_layout = device.createPipelineLayoutUnique(pipeline_layout_ci);

		while (glfwWindowShouldClose(window.get()) == GLFW_FALSE) {
			glfwPollEvents();
			if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				glfwSetWindowShouldClose(window.get(), GLFW_TRUE);
			}

			auto const command_buffer = render_device.next_frame();
			if (!command_buffer) { continue; }

			auto const descriptor_layouts = std::array{*texture_layout};
			auto const descriptor_sets = render_device.allocate_descriptor_set(descriptor_layouts);
			auto const image_info = texture.descriptor_info();
			auto write = vk::WriteDescriptorSet{};
			write.setDstSet(descriptor_sets.front())
				.setDstBinding(0)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setImageInfo(image_info);
			device.updateDescriptorSets(write, {});

			shader.bind(*command_buffer, render_device.framebuffer_size());
			command_buffer->bindVertexBuffers(0, quad_buffer.get().buffer, vk::DeviceSize{});
			command_buffer->bindIndexBuffer(quad_buffer.get().buffer, sizeof(vertices_v), vk::IndexType::eUint32);
			command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0, descriptor_sets,
											   {});
			command_buffer->drawIndexed(static_cast<std::uint32_t>(indices_v.size()), 1, 0, 0, 0);
			render_device.render(*command_buffer);
		}
		return 0;
	} catch (std::exception const& error) {
		std::println(stderr, "[SVKF example] {}", error.what());
		return 1;
	}
}
