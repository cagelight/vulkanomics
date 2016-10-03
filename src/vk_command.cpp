#include "vulkanomics.hpp"
#include "vk_internal.hpp"

vk::command::pool::pool(device const & parent, VkCommandPoolCreateFlags flags, uint32_t queue_family) : parent(parent) {
	VkCommandPoolCreateInfo pool_create = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = flags,
		.queueFamilyIndex = queue_family
	};
	VKR(parent.vkCreateCommandPool(parent, &pool_create, nullptr, &handle))
}

vk::command::pool::~pool() {
	if (handle) parent.vkDestroyCommandPool(parent, handle, nullptr);
}

vk::command::buffer vk::command::pool::allocate_buffer(VkCommandBufferLevel lev) {
	buffer b {*this};
	VkCommandBufferAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = handle,
		.level = lev,
		.commandBufferCount = 1,
	};
	VKR(parent.vkAllocateCommandBuffers(parent, &allocate, &b.handle))
	return b;
}

std::vector<vk::command::buffer> vk::command::pool::allocate_buffers(uint32_t count, VkCommandBufferLevel lev) {
	std::vector<VkCommandBuffer> cbufs;
	cbufs.resize(count);
	VkCommandBufferAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = handle,
		.level = lev,
		.commandBufferCount = count,
	};
	VKR(parent.vkAllocateCommandBuffers(parent, &allocate, cbufs.data()))
	std::vector<buffer> bufs;
	for (VkCommandBuffer & cmd : cbufs) {
		buffer b {*this};
		b.handle = cmd;
		bufs.push_back(b);
	}
	return bufs;
}

void vk::command::buffer::begin(VkCommandBufferUsageFlags flags, VkCommandBufferInheritanceInfo const * inheritance) {
	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = nullptr,
		.flags = flags,
		.pInheritanceInfo = inheritance,
	};
	VKR(parent.parent.vkBeginCommandBuffer(handle, &begin_info))
}

void vk::command::buffer::end() {
	VKR(parent.parent.vkEndCommandBuffer(handle))
}

void vk::command::buffer::bind_compute_pipeline(compute_pipeline const & pip) {
	parent.parent.vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_COMPUTE, pip);
}

void vk::command::buffer::bind_graphics_pipeline(graphics_pipeline const & pip) {
	parent.parent.vkCmdBindPipeline(handle, VK_PIPELINE_BIND_POINT_GRAPHICS, pip);
}

void vk::command::buffer::dispatch(uint32_t x, uint32_t y, uint32_t z) {
	parent.parent.vkCmdDispatch(handle, x, y, z);
}
