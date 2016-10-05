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

vk::command::buffer::buffer(pool const & parent, VkCommandBufferLevel lev) : parent(parent) {
	VkCommandBufferAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = nullptr,
		.commandPool = parent.handle,
		.level = lev,
		.commandBufferCount = 1,
	};
	VKR(parent.parent.vkAllocateCommandBuffers(parent.parent, &allocate, &handle))
}

vk::command::buffer::~buffer() {
	if (handle) parent.parent.vkFreeCommandBuffers(parent.parent, parent.handle, 1, &handle);
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

void vk::command::buffer::bind_descriptor_sets(VkPipelineBindPoint bind_point, pipeline::layout & layout, std::vector<descriptor::set const *> const & descriptors) {
	std::vector<VkDescriptorSet> descs;
	for (descriptor::set const * s : descriptors) {
		descs.push_back(*s);
	}
	parent.parent.vkCmdBindDescriptorSets(handle, bind_point, layout.handle, 0, descs.size(), descs.data(), 0, nullptr);
}

void vk::command::buffer::dispatch(uint32_t x, uint32_t y, uint32_t z) {
	parent.parent.vkCmdDispatch(handle, x, y, z);
}

void vk::command::buffer::barrier(VkPipelineStageFlags stages_src, VkPipelineStageFlags stages_dst, std::vector<VkMemoryBarrier> const & memb, std::vector<VkBufferMemoryBarrier> const & bmemb, std::vector<VkImageMemoryBarrier> const & imemb, VkDependencyFlags dep) {
	parent.parent.vkCmdPipelineBarrier(handle, stages_src, stages_dst, dep, memb.size(), memb.data(), bmemb.size(), bmemb.data(), imemb.size(), imemb.data());
}
