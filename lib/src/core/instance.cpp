#include "svkf/core/instance.hpp"
#include "svkf/constants.hpp"
#include <GLFW/glfw3.h>
#include <print>

namespace svkf::Instance {
namespace {
auto instance_extensions() -> std::span<char const* const> {
	auto count = std::uint32_t{};
	auto const* extensions = glfwGetRequiredInstanceExtensions(&count);
	return {extensions, count};
}

[[nodiscard]] auto get_layers(std::span<char const* const> desired) -> std::vector<char const*> {
	auto ret = std::vector<char const*>{};
	ret.reserve(desired.size());
	auto const available = vk::enumerateInstanceLayerProperties();
	for (char const* layer : desired) {
		auto const pred = [layer = std::string_view{layer}](vk::LayerProperties const& properties) {
			return properties.layerName == layer;
		};
		if (std::ranges::find_if(available, pred) == available.end()) {
			std::println("[svkf] [WARNING] Vulkan Layer '{}' not found", layer);
			continue;
		}
		ret.push_back(layer);
	}
	return ret;
}

} // namespace

auto create() -> vk::UniqueInstance {
	VULKAN_HPP_DEFAULT_DISPATCHER.init();
	auto const loader_version = vk::enumerateInstanceVersion();
	if (loader_version < vk_version_v) { throw std::runtime_error{"Loader does not support Vulkan 1.3"}; }

	auto app_info = vk::ApplicationInfo{};
	app_info.setApiVersion(vk_version_v);

	auto instance_ci = vk::InstanceCreateInfo{};
	auto const extensions = instance_extensions();
	instance_ci.setPApplicationInfo(&app_info).setPEnabledExtensionNames(extensions);

	static constexpr auto layers_v = std::array{
		"VK_LAYER_KHRONOS_shader_object",
	};
	auto const layers = get_layers(layers_v);
	instance_ci.setPEnabledLayerNames(layers);

	auto instance = vk::createInstanceUnique(instance_ci);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance);

	return instance;
}
} // namespace svkf::Instance