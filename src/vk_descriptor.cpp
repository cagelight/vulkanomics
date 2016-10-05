#include "vulkanomics.hpp"
#include "vk_internal.hpp"

/*
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

*/

vk::descriptor::layout::layout(device const & parent, std::vector<VkDescriptorSetLayoutBinding> const & bindings) : parent(parent) {
	VkDescriptorSetLayoutCreateInfo create = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data(),
	};
	VKR(parent.vkCreateDescriptorSetLayout(parent, &create, nullptr, &handle))
}

vk::descriptor::layout::~layout() {
	if (handle != VK_NULL_HANDLE) parent.vkDestroyDescriptorSetLayout(parent, handle, nullptr);
}

vk::descriptor::pool::pool(device const & parent, pool_size_set const & pool_sizes, uint32_t max_sets) : parent(parent) {
	VkDescriptorPoolCreateInfo create = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
		.maxSets = max_sets,
		.poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
		.pPoolSizes = pool_sizes.data()
	};
	VKR(parent.vkCreateDescriptorPool(parent, &create, nullptr, &handle))
}

vk::descriptor::pool::~pool() {
	if (handle != VK_NULL_HANDLE) parent.vkDestroyDescriptorPool(parent, handle, nullptr);
}

vk::descriptor::set::set(pool const & parent, layout const & lay) : parent(parent) {
	VkDescriptorSetAllocateInfo allocate = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = parent,
		.descriptorSetCount = 1,
		.pSetLayouts = &lay.get_handle(),
	};
	VKR(parent.parent.vkAllocateDescriptorSets(parent.parent, &allocate, &handle))
}

vk::descriptor::set::~set() {
	if (handle != VK_NULL_HANDLE) parent.parent.vkFreeDescriptorSets(parent.parent, parent, 1, &handle);
}

void vk::descriptor::update_session::update() {
	parent.vkUpdateDescriptorSets(parent, wset.size(), wset.data(), cset.size(), cset.data());
}

void vk::descriptor::update_session::copy(set & src, set & dst, uint32_t src_binding, uint32_t dst_binding, uint32_t count, uint32_t src_index, uint32_t dst_index) {
	VkCopyDescriptorSet cpyset = {
		.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
		.pNext = nullptr,
		.srcSet = src,
		.srcBinding = src_binding,
		.srcArrayElement = src_index,
		.dstSet = dst,
		.dstBinding = dst_binding,
		.dstArrayElement = dst_index,
		.descriptorCount = count,
	};
	cset.push_back(std::move(cpyset));
}

void vk::descriptor::update_session::write_buffer(set & s, uint32_t binding, uint32_t index, VkDescriptorType type, buffer_info_set const & bi) {
	VkWriteDescriptorSet wrtset = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = s,
		.dstBinding = binding,
		.dstArrayElement = index,
		.descriptorCount = static_cast<uint32_t>(bi.size()),
		.descriptorType = type,
		.pImageInfo = nullptr,
		.pBufferInfo = bi.data(),
		.pTexelBufferView = nullptr,
	};
	wset.push_back(std::move(wrtset));
}
