#include "svkf/core/shader.hpp"

namespace svkf {
namespace {
constexpr auto to_vkbool(bool const val) { return val ? vk::True : vk::False; }

} // namespace

Shader::Shader(CreateInfo const& create_info) : m_vertex_input(create_info.vertex_input) {
	auto const create_shader_ci = [&create_info](std::span<std::uint32_t const> spirv) {
		auto ret = vk::ShaderCreateInfoEXT{};
		ret.setCodeSize(spirv.size_bytes())
			.setPCode(spirv.data())
			.setSetLayouts(create_info.set_layouts)
			.setCodeType(vk::ShaderCodeTypeEXT::eSpirv)
			.setPName("main");
		return ret;
	};

	auto shader_cis = std::array{
		create_shader_ci(create_info.vertex_code),
		create_shader_ci(create_info.fragment_code),
	};
	shader_cis[0].setStage(vk::ShaderStageFlagBits::eVertex).setNextStage(vk::ShaderStageFlagBits::eFragment);
	shader_cis[1].setStage(vk::ShaderStageFlagBits::eFragment);

	auto res = create_info.device.createShadersEXTUnique(shader_cis);
	if (res.result != vk::Result::eSuccess) { throw std::runtime_error{"Failed to create Shader Objects"}; }
	m_shaders = std::move(res.value);
	m_waiter = create_info.device;
}

void Shader::bind(vk::CommandBuffer const command_buffer, glm::ivec2 const framebuffer_size) const {
	set_viewport_scissor(command_buffer, framebuffer_size);
	set_static_states(command_buffer);
	set_common_states(command_buffer);
	set_vertex_states(command_buffer);
	set_fragment_states(command_buffer);
	bind_shaders(command_buffer);
}

void Shader::set_viewport_scissor(vk::CommandBuffer const command_buffer, glm::ivec2 const size) {
	auto const fsize = glm::vec2{size};
	auto viewport = vk::Viewport{};
	viewport.setX(0.0f).setY(fsize.y).setWidth(fsize.x).setHeight(-fsize.y);
	command_buffer.setViewportWithCount(viewport);

	auto const usize = glm::uvec2{size};
	auto const scissor = vk::Rect2D{vk::Offset2D{}, vk::Extent2D{usize.x, usize.y}};
	command_buffer.setScissorWithCount(scissor);
}

void Shader::set_static_states(vk::CommandBuffer const command_buffer) {
	command_buffer.setRasterizerDiscardEnable(vk::False);
	command_buffer.setRasterizationSamplesEXT(vk::SampleCountFlagBits::e1);
	command_buffer.setSampleMaskEXT(vk::SampleCountFlagBits::e1, 0xff);
	command_buffer.setAlphaToCoverageEnableEXT(vk::False);
	command_buffer.setCullMode(vk::CullModeFlagBits::eNone);
	command_buffer.setFrontFace(vk::FrontFace::eCounterClockwise);
	command_buffer.setDepthBiasEnable(vk::False);
	command_buffer.setStencilTestEnable(vk::False);
	command_buffer.setPrimitiveRestartEnable(vk::False);
	command_buffer.setColorWriteMaskEXT(0, ~vk::ColorComponentFlags{});
}

void Shader::set_common_states(vk::CommandBuffer const command_buffer) const {
	auto const depth_test = to_vkbool((flags & DepthTest) == DepthTest);
	command_buffer.setDepthWriteEnable(depth_test);
	command_buffer.setDepthTestEnable(depth_test);
	command_buffer.setDepthCompareOp(depth_compare_op);
	command_buffer.setPolygonModeEXT(polygon_mode);
	command_buffer.setLineWidth(line_width);
}

void Shader::set_vertex_states(vk::CommandBuffer const command_buffer) const {
	command_buffer.setVertexInputEXT(m_vertex_input.bindings, m_vertex_input.attributes);
	command_buffer.setPrimitiveTopology(topology);
}

void Shader::set_fragment_states(vk::CommandBuffer const command_buffer) const {
	auto const alpha_blend = to_vkbool((flags & AlphaBlend) == AlphaBlend);
	command_buffer.setColorBlendEnableEXT(0, alpha_blend);
	command_buffer.setColorBlendEquationEXT(0, color_blend_equation);
}

void Shader::bind_shaders(vk::CommandBuffer const command_buffer) const {
	static constexpr auto stages_v = std::array{
		vk::ShaderStageFlagBits::eVertex,
		vk::ShaderStageFlagBits::eFragment,
	};
	auto const shaders = std::array{
		*m_shaders[0],
		*m_shaders[1],
	};
	command_buffer.bindShadersEXT(stages_v, shaders);
}
} // namespace svkf