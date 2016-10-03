#include "vulkanomics.hpp"
#include "vk_internal.hpp"

#include <cstdarg>

static constexpr size_t strf_startlen = 256;
std::string strf(char const * fmt, ...) noexcept {
	va_list va;
	va_start(va, fmt);
	char * tmp_buf = reinterpret_cast<char *>(malloc(strf_startlen));
	tmp_buf[strf_startlen - 1] = 0;
	size_t req_size = vsnprintf(tmp_buf, strf_startlen, fmt, va);
	va_end(va);
	if (req_size >= strf_startlen) {
		tmp_buf = reinterpret_cast<char *>(realloc(tmp_buf, req_size+1));
		va_start(va, fmt);
		vsprintf(tmp_buf, fmt, va);
		va_end(va);
		tmp_buf[req_size] = 0;
	}
	std::string r = {tmp_buf};
	free(tmp_buf);
	return {r};
}

char const * vk_result_to_str(VkResult res) {
	switch (res) {
		
		case VK_SUCCESS:
			return "VK_SUCCESS";
		case VK_NOT_READY:
			return "VK_NOT_READY";
		case VK_TIMEOUT:
			return "VK_TIMEOUT";
		case VK_EVENT_SET:
			return "VK_EVENT_SET";
		case VK_EVENT_RESET:
			return "VK_EVENT_RESET";
		case VK_INCOMPLETE:
			return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:
			return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:
			return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:
			return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:
			return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:
			return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_SURFACE_LOST_KHR:
			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:
			return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:
			return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:
			return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:
			return "VK_ERROR_INVALID_SHADER_NV";
		default:
			return "VK_ERROR_UNKNOWN";
	}
}


vk::fence::fence(device const & parent) : parent(parent) {
	VkFenceCreateInfo create = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
	};
	VKR(parent.vkCreateFence(parent, &create, nullptr, &handle))
}

vk::fence::~fence() {
	if (handle) parent.vkDestroyFence(parent, handle, nullptr);
}

void vk::fence::reset() {
	VKR(parent.vkResetFences(parent, 1, &handle))
}

void vk::fence::wait(uint64_t timeout) {
	parent.vkWaitForFences(parent, 1, &handle, VK_FALSE, timeout);
}

vk::semaphore::semaphore(device const & parent) : parent(parent) {
	VkSemaphoreCreateInfo create = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
	};
	VKR(parent.vkCreateSemaphore(parent, &create, nullptr, &handle))
}

vk::semaphore::~semaphore() {
	if (handle) parent.vkDestroySemaphore(parent, handle, nullptr);
}

vk::shader::shader(device const & parent, uint8_t const * spv, size_t spv_len) : parent(parent) {
	VkShaderModuleCreateInfo module_create = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = spv_len,
		.pCode = reinterpret_cast<uint32_t const *>(spv),
	};
	VKR(parent.vkCreateShaderModule(parent, &module_create, nullptr, &handle))
}

vk::shader::~shader() {
	if (handle != VK_NULL_HANDLE) parent.vkDestroyShaderModule(parent, handle, nullptr);
}

void vk::queue_accessor_direct::submit(VkSubmitInfo * infos, uint32_t infos_count, VkFence fence) {
	parent.vkQueueSubmit(queue.handle, infos_count, infos, fence);
}

void vk::queue_accessor_mutexed::submit(VkSubmitInfo * infos, uint32_t infos_count, VkFence fence) {
	mut.lock();
	parent.vkQueueSubmit(queue.handle, infos_count, infos, fence);
	mut.unlock();
}
