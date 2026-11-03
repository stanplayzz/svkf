#pragma once
#include "svkf/core/device_waiter.hpp"
#include <glm/vec2.hpp>
#include <vulkan/vulkan.hpp>

namespace svkf {
struct ShaderVertexInput {
	std::span<vk::VertexInputAttributeDescription2EXT const> attributes{};
	std::span<vk::VertexInputBindingDescription2EXT const> bindings{};
};

struct ShaderCreateInfo {
	vk::Device device;
	std::span<std::uint32_t const> vertex_code;
	std::span<std::uint32_t const> fragment_code;
	ShaderVertexInput vertex_input{};
	std::span<vk::DescriptorSetLayout const> set_layouts;
};

class Shader {
  public:
	using CreateInfo = ShaderCreateInfo;
	explicit Shader(CreateInfo const& create_info);

	void bind(vk::CommandBuffer command_buffer, glm::ivec2 framebuffer_size) const;

	static constexpr auto color_blend_equation_v = [] {
		auto ret = vk::ColorBlendEquationEXT{};
		ret.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
		return ret;
	}();

	enum : std::uint8_t {
		None = 0,
		AlphaBlend = 1 << 0,
		DepthTest = 1 << 1,
	};
	static constexpr auto flags_v = AlphaBlend | DepthTest;

	std::uint8_t flags{flags_v};
	vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};
	vk::PolygonMode polygon_mode{vk::PolygonMode::eFill};
	float line_width{1.0f};
	vk::ColorBlendEquationEXT color_blend_equation{color_blend_equation_v};
	vk::CompareOp depth_compare_op{vk::CompareOp::eLessOrEqual};

  private:
	static void set_viewport_scissor(vk::CommandBuffer command_buffer, glm::ivec2 size);
	static void set_static_states(vk::CommandBuffer command_buffer);
	void set_common_states(vk::CommandBuffer command_buffer) const;
	void set_vertex_states(vk::CommandBuffer command_buffer) const;
	void set_fragment_states(vk::CommandBuffer command_buffer) const;
	void bind_shaders(vk::CommandBuffer command_buffer) const;

	ShaderVertexInput m_vertex_input{};
	std::vector<vk::UniqueShaderEXT> m_shaders{};
	DeviceWaiter m_waiter{};
};
} // namespace svkf