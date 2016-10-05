#include "vulkanomics.hpp"
#include "vk_internal.hpp"

#include <dlfcn.h>

//================================================================
#define VK_FN_IDECL
#include "vulkanomics_fn.inl"
//================================================================

static void * vk_handle = nullptr;
VkInstance vk_instance = VK_NULL_HANDLE;
static std::vector<vk::physical_device> physical_devices {};

//================================================================
VkSurfaceKHR vk::surface::handle = VK_NULL_HANDLE;
static VkSurfaceCapabilitiesKHR surface_capabilities;
static std::vector<VkSurfaceFormatKHR> surface_formats;
static std::vector<VkPresentModeKHR> surface_present_modes;
VkSurfaceCapabilitiesKHR const & vk::surface::capabilities {surface_capabilities};
std::vector<VkSurfaceFormatKHR> const & vk::surface::formats {surface_formats};
std::vector<VkPresentModeKHR> const & vk::surface::present_modes {surface_present_modes};
//================================================================

static void vk_instance_init(std::vector<char const *> & instance_extensions, std::vector<char const *> & instance_layers) {
	vk_handle = dlopen("libvulkan.so", RTLD_LOCAL | RTLD_NOW);
	if (!vk_handle) srcthrow("could not open vulkan library");
	
	#define VK_FN_SYM_GLOBAL
	#include "vulkanomics_fn.inl"

	uint32_t supext_cnt;
	VKR(vk::EnumerateInstanceExtensionProperties(nullptr, &supext_cnt, nullptr))
	std::vector<VkExtensionProperties> supext {supext_cnt};
	VKR(vk::EnumerateInstanceExtensionProperties(nullptr, &supext_cnt, supext.data()))
	
	for (char const * ext : instance_extensions) {
		bool c = false;
		for (VkExtensionProperties const & ep : supext) {
			if (!strcmp(ext, ep.extensionName)) c = true;
		}
		if (!c) srcthrow("required extension \"%s\" unsupported", ext);
	}
	
	uint32_t suplay_cnt;
	VKR(vk::EnumerateInstanceLayerProperties(&suplay_cnt, nullptr))
	std::vector<VkLayerProperties> suplay {suplay_cnt};
	VKR(vk::EnumerateInstanceLayerProperties(&suplay_cnt, suplay.data()))
	
	for (char const * ext : instance_layers) {
		bool c = false;
		for (VkLayerProperties const & ep : suplay) {
			if (!strcmp(ext, ep.layerName)) c = true;
		}
		if (!c) srcthrow("required layer \"%s\" unsupported", ext);
	}
	
	VkApplicationInfo application_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = "Progeny",
		.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
		.pEngineName = "Progeny",
		.engineVersion = VK_MAKE_VERSION(0, 0, 1),
		.apiVersion = VK_MAKE_VERSION(1, 0, 21),
	};
	
	VkInstanceCreateInfo instance_create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &application_info,
		.enabledLayerCount = static_cast<uint32_t>(instance_layers.size()),
		.ppEnabledLayerNames = instance_layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size()),
		.ppEnabledExtensionNames = instance_extensions.data(),
	};
	
	VKR(vk::CreateInstance(&instance_create_info, nullptr, &vk_instance))
	
	#define VK_FN_SYM_INSTANCE
	#include "vulkanomics_fn.inl"
}

static void vk_physical_devices_init() {
	uint32_t pdev_cnt;
	VKR(vk::EnumeratePhysicalDevices(vk_instance, &pdev_cnt, NULL))
	std::vector<VkPhysicalDevice> pdevs {pdev_cnt};
	VKR(vk::EnumeratePhysicalDevices(vk_instance, &pdev_cnt, pdevs.data()))

	for (VkPhysicalDevice & pdev : pdevs) {
		try {
			physical_devices.emplace_back(pdev);
		} catch (vk::exception & e) {
			srcprintf_debug("WARNING: a physical device could not be resolved: \"%s\"", e.what());
			continue;
		}
	}
}

void vk::instance::init(xcb_connection_t * con, xcb_window_t & win) {
	std::vector<char const *> instance_extensions = {
		"VK_KHR_surface", "VK_KHR_xcb_surface",
		#ifdef VULKANOMICS_VK_DEBUG
			"VK_EXT_debug_report"
		#endif
	};
	std::vector<char const *> instance_layers = {
		#ifdef VULKANOMICS_VK_DEBUG
			"VK_LAYER_LUNARG_standard_validation",
		#endif
	};
	vk_instance_init(instance_extensions, instance_layers);
	#define VK_FN_SYM_SURFACE
	#include "vulkanomics_fn.inl"
	VkXcbSurfaceCreateInfoKHR surf_create = {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.pNext = nullptr,
		.flags = 0,
		.connection = con,
		.window = win,
	};
	VKR(vk::CreateXcbSurfaceKHR(vk_instance, &surf_create, NULL, &vk::surface::handle))
	vk_physical_devices_init();
}

void vk::instance::init() {
	std::vector<char const *> instance_extensions = {
		#ifdef VULKANOMICS_VK_DEBUG
			"VK_EXT_debug_report"
		#endif
	};
	std::vector<char const *> instance_layers = {
		#ifdef VULKANOMICS_VK_DEBUG
			"VK_LAYER_LUNARG_standard_validation",
		#endif
	};
	vk_instance_init(instance_extensions, instance_layers);
	vk_physical_devices_init();
}

void vk::instance::term() noexcept {
	if (vk::surface::handle) {
		DestroySurfaceKHR(vk_instance, vk::surface::handle, nullptr);
		vk::surface::handle = VK_NULL_HANDLE;
	}
	if (vk_instance) {
		DestroyInstance(vk_instance, nullptr);
		vk_instance = VK_NULL_HANDLE;
	}
	if (vk_handle) {
		dlclose(vk_handle);
		vk_handle = nullptr;
	}
}

vk::physical_device::physical_device(VkPhysicalDevice & handle) : handle(handle) {
	GetPhysicalDeviceProperties(handle, &properties);
	uint32_t num;
	VKR(EnumerateDeviceExtensionProperties(handle, nullptr, &num, nullptr))
	this->extensions.resize(num);
	VKR(EnumerateDeviceExtensionProperties(handle, nullptr, &num, this->extensions.data()))
	VKR(EnumerateDeviceLayerProperties(handle, &num, nullptr))
	this->layers.resize(num);
	VKR(EnumerateDeviceLayerProperties(handle, &num, this->layers.data()))
	GetPhysicalDeviceQueueFamilyProperties(handle, &num, nullptr);
	this->queue_families.resize(num);
	GetPhysicalDeviceQueueFamilyProperties(handle, &num, this->queue_families.data());
	GetPhysicalDeviceMemoryProperties(handle, &this->memory_properties);
	
	this->queue_families_presentable.resize(num);
	if (surface::handle) {
		for (uint32_t i = 0; i < this->queue_families.size(); i++) {
			GetPhysicalDeviceSurfaceSupportKHR(handle, i, vk::surface::handle, &this->queue_families_presentable[i]);
		}
	}
}

uint32_t vk::physical_device::find_staging_memory(uint32_t restrict_mask) const {
	uint32_t index = UINT32_MAX;
	
	for (uint32_t mi = 0, ri = 0; mi < memory_properties.memoryTypeCount; mi++) {
		ri = 1 << mi;
		if (!(ri & restrict_mask) || !(memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) continue;
		else if (index == UINT32_MAX) index = mi;
		
		do {
			if (memory_properties.memoryHeaps[memory_properties.memoryTypes[mi].heapIndex].size != memory_properties.memoryHeaps[memory_properties.memoryTypes[index].heapIndex].size) {
				if (memory_properties.memoryHeaps[memory_properties.memoryTypes[mi].heapIndex].size > memory_properties.memoryHeaps[memory_properties.memoryTypes[index].heapIndex].size) index = mi;
				break;
			}
			if ((memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != (memory_properties.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				if (memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) index = mi;
				break;
			}
			if ((memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != (memory_properties.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
				if (memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) index = mi;
				break;
			}
			if ((memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != (memory_properties.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				if (memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) index = mi;
				break;
			}
			if ((memory_properties.memoryHeaps[memory_properties.memoryTypes[mi].heapIndex].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != (memory_properties.memoryHeaps[memory_properties.memoryTypes[index].heapIndex].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)) {
				if (memory_properties.memoryHeaps[memory_properties.memoryTypes[mi].heapIndex].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) index = mi;
				break;
			}
		} while(0);
	}
	if (index == UINT32_MAX) srcthrow("memory index requirements could not be satisfied");
	return index;
}

uint32_t vk::physical_device::find_device_memory(uint32_t restrict_mask) const {
	uint32_t index = UINT32_MAX;
	
	for (uint32_t mi = 0, ri = 0; mi < memory_properties.memoryTypeCount; mi++) {
		ri = 1 << mi;
		if (!(ri & restrict_mask)) continue;
		else if (index == UINT32_MAX) index = mi;
		
		do {
			if ((memory_properties.memoryHeaps[memory_properties.memoryTypes[mi].heapIndex].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != (memory_properties.memoryHeaps[memory_properties.memoryTypes[index].heapIndex].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)) {
				if (memory_properties.memoryHeaps[memory_properties.memoryTypes[mi].heapIndex].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) index = mi;
				break;
			}
			if ((memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != (memory_properties.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				if (memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) index = mi;
				break;
			}
			if (memory_properties.memoryHeaps[memory_properties.memoryTypes[mi].heapIndex].size != memory_properties.memoryHeaps[memory_properties.memoryTypes[index].heapIndex].size) {
				if (memory_properties.memoryHeaps[memory_properties.memoryTypes[mi].heapIndex].size > memory_properties.memoryHeaps[memory_properties.memoryTypes[index].heapIndex].size) index = mi;
				break;
			}
			if ((memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != (memory_properties.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
				if (memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) index = mi;
				break;
			}
			if ((memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != (memory_properties.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
				if (memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) index = mi;
				break;
			}
			if ((memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != (memory_properties.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				if (memory_properties.memoryTypes[mi].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) index = mi;
				break;
			}
		} while(0);
	}
	if (index == UINT32_MAX) srcthrow("memory index requirements could not be satisfied");
	return index;
}

std::vector<vk::physical_device> const & vk::get_physical_devices() {
	return physical_devices;
}

void vk::surface::setup(physical_device const & pdev) {
	uint32_t num;
	VKR(GetPhysicalDeviceSurfaceCapabilitiesKHR(pdev.handle, handle, &surface_capabilities))
	VKR(GetPhysicalDeviceSurfaceFormatsKHR(pdev.handle, handle, &num, NULL))
	surface_formats.resize(num);
	VKR(GetPhysicalDeviceSurfaceFormatsKHR(pdev.handle, handle, &num, surface_formats.data()))
	VKR(GetPhysicalDeviceSurfacePresentModesKHR(pdev.handle, handle, &num, NULL))
	surface_present_modes.resize(num);
	VKR(GetPhysicalDeviceSurfacePresentModesKHR(pdev.handle, handle, &num, surface_present_modes.data()))
}
