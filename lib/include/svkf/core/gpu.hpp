#pragma once
#include <klib/base_types.hpp>
#include <vulkan/vulkan.hpp>
#include <ranges>

namespace svkf {
struct Gpu {
	vk::PhysicalDevice device{};
	vk::PhysicalDeviceProperties properties{};
	vk::PhysicalDeviceFeatures features{};
	std::uint32_t queue_family{};

	class Selector : klib::Polymorphic {
	  public:
		[[nodiscard]] virtual auto select(std::span<Gpu const> gpus) const -> Gpu const& {
			for (auto const& gpu : gpus) {
				if (gpu.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) { return gpu; }
			}
			return gpus.front();
		}
	};

	static auto select_gpu(vk::Instance instance, vk::SurfaceKHR surface, Selector const& selector = {}) -> Gpu;
};

struct GpuList {
	std::vector<Gpu> gpus{};

	static constexpr auto has_required_extensions(std::span<vk::ExtensionProperties const> avaible) {
		auto const has_extension = [avaible](char const* name) {
			auto const it = [name](vk::ExtensionProperties const& properties) {
				return std::string_view{properties.extensionName.data()} == name;
			};
			return std::ranges::find_if(avaible, it) != avaible.end();
		};
		return has_extension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	}

	[[nodiscard]] static auto get_viable(vk::Instance instance, vk::SurfaceKHR surface) {
		static constexpr auto queue_flags_v = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;
		auto const devices = instance.enumeratePhysicalDevices();
		auto ret = GpuList{};
		auto const get_queue_family = [](vk::PhysicalDevice device, std::uint32_t& out) {
			for (auto const& [index, family] : std::ranges::enumerate_view(device.getQueueFamilyProperties())) {
				if ((family.queueFlags & queue_flags_v) != queue_flags_v) { continue; }
				out = static_cast<std::uint32_t>(index);
				return true;
			}
			return false;
		};
		for (auto const& device : devices) {
			if (device.getProperties().apiVersion < VK_API_VERSION_1_3) { continue; }
			if (!has_required_extensions(device.enumerateDeviceExtensionProperties())) { continue; }
			auto queue_family = std::uint32_t{};
			if (!get_queue_family(device, queue_family)) { continue; }
			if (device.getSurfaceSupportKHR(queue_family, surface) == vk::False) { continue; }
			ret.gpus.push_back(Gpu{.device = device,
								   .properties = device.getProperties(),
								   .features = device.getFeatures(),
								   .queue_family = queue_family});
		}
		return ret;
	}
};

inline auto Gpu::select_gpu(vk::Instance instance, vk::SurfaceKHR surface, Selector const& selector) -> Gpu {
	auto const list = GpuList::get_viable(instance, surface);
	if (list.gpus.empty()) { throw std::runtime_error{"No viable GPUs"}; }
	auto const gpu = selector.select(list.gpus);
	return gpu;
}
} // namespace svkf