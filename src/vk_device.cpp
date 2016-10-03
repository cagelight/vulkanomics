#include "vulkanomics.hpp"
#include "vk_internal.hpp"

constexpr uint32_t vk::device::capability::transfer;
constexpr uint32_t vk::device::capability::compute;
constexpr uint32_t vk::device::capability::graphics;
constexpr uint32_t vk::device::capability::presentable;

vk::device::initializer::initializer(physical_device const & pdev, capability_set const & caps) : parent(pdev) {
	this->create_infos.resize(pdev.queue_families.size());
	this->queue_priorities.resize(pdev.queue_families.size());
	this->protoqueues.resize(caps.size());
	for (uint32_t i = 0; i < pdev.queue_families.size(); i++) {
		this->create_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		this->create_infos[i].pNext = nullptr;
		this->create_infos[i].flags = 0;
		this->create_infos[i].queueFamilyIndex = i;
		this->create_infos[i].queueCount = 0;
	}
	
	for (uint32_t c = 0; c < caps.size(); c++) {
		
		bool queue_found = false;
		
		for (uint32_t i = 0; i < pdev.queue_families.size(); i++) {
			if (caps[c] & capability::graphics && !(pdev.queue_families[i].queueFlags | VK_QUEUE_GRAPHICS_BIT)) continue;
			if (caps[c] & capability::compute && !(pdev.queue_families[i].queueFlags | VK_QUEUE_COMPUTE_BIT)) continue;
			if (caps[c] & capability::transfer && !(pdev.queue_families[i].queueFlags | VK_QUEUE_TRANSFER_BIT)) continue;
			if (caps[c] & capability::presentable && !pdev.queue_families_presentable[i]) continue;
			if (create_infos[i].queueCount == pdev.queue_families[i].queueCount) continue;
			queue_priorities[i].emplace_back(caps[c] == capability::transfer ? 0.5f : 1.0f);
			protoqueues[c].queue_index = create_infos[i].queueCount++;
			protoqueues[c].cap_flags = caps[c];
			protoqueues[c].queue_family = i;
			queue_found = true;
			overall_capability |= caps[c];
			break;
		}
		
		if (queue_found) continue;
		
		srcthrow("could not resolve a queue for requested capability: %u, index %i", caps[c], c);
		
	}
	
	for (uint32_t i = 0; i < pdev.queue_families.size(); i++) {
		this->create_infos[i].pQueuePriorities = this->queue_priorities[i].data();
	}
	
	if (overall_capability & capability::presentable) device_extensions.push_back("VK_KHR_swapchain");
	
	for (char const * ext : device_extensions) {
		bool sup = false;
		for (VkExtensionProperties const & ep : parent.extensions) {
			if (!strcmp(ext, ep.extensionName)) sup = true;
		}
		if (!sup) srcthrow("required device extension \"%s\" unsupported by selected physical device \"%s\"", ext, pdev.properties.deviceName);
	}
	
	for (char const * ext : device_layers) {
		bool sup = false;
		for (VkLayerProperties const & ep : parent.layers) {
			if (!strcmp(ext, ep.layerName)) sup = true;
		}
		if (!sup) srcthrow("required device layer \"%s\" unsupported by selected physical device \"%s\"", ext, pdev.properties.deviceName);
	}
	
	for (std::vector<VkDeviceQueueCreateInfo>::iterator iter = create_infos.begin(); iter != create_infos.end();) {
		if (iter->queueCount == 0) {
			iter = create_infos.erase(iter);
		} else {
			iter++;
		}
	}
}

vk::device::device(initializer & ldi) : parent(ldi.parent), overall_capability(ldi.overall_capability), device_extensions(std::move(ldi.device_extensions)), device_layers(std::move(ldi.device_layers)) {
	
	VkDeviceCreateInfo device_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = static_cast<uint32_t>(ldi.create_infos.size()),
		.pQueueCreateInfos = ldi.create_infos.data(),
		.enabledLayerCount = static_cast<uint32_t>(device_layers.size()),
		.ppEnabledLayerNames = device_layers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size()),
		.ppEnabledExtensionNames = device_extensions.data(),
		.pEnabledFeatures = NULL,
	};
	
	VKR(vk::CreateDevice(parent.handle, &device_create_info, NULL, &handle))
	
	#define VK_FN_SYM_DEVICE
	#include "vulkanomics_fn.inl"
	
	if (overall_capability & capability::presentable) {
		#define VK_FN_SYM_SWAPCHAIN
		#include "vulkanomics_fn.inl"
	}
	
	queues.resize(ldi.protoqueues.size());
	
	for (size_t i = 0; i < ldi.protoqueues.size(); i++) {
		vkGetDeviceQueue(handle, ldi.protoqueues[i].queue_family, ldi.protoqueues[i].queue_index, &queues[i].handle);
		queues[i].cap_flags = ldi.protoqueues[i].cap_flags;
		queues[i].queue_family = ldi.protoqueues[i].queue_family;
	}
}

vk::device::~device() {
	if (handle && vkDestroyDevice) {
		vkDestroyDevice(handle, nullptr);
	}
}
