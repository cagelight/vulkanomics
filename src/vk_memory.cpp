#include "vulkanomics.hpp"
#include "vk_internal.hpp"

vk::memory::memory(device const & parent, vk::physical_device::memory_type const * mem, VkDeviceSize size) : parent(parent), size_(size), mem_type_(mem) {
	VkMemoryAllocateInfo memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = size_,
		.memoryTypeIndex = mem->index,
	};
	VKR(parent.vkAllocateMemory(parent, &memory_allocate_info, nullptr, &handle))
}

static VkDeviceSize next_alignment(VkDeviceSize position, VkDeviceSize alignment) {
	if (alignment == 0) return position;
	VkDeviceSize r = position % alignment;
	if (r == 0) return position;
	return position + alignment - r;
}

vk::memory::memory(device const & parent, vk::physical_device::memory_type const * mem, std::vector<vk::memory_bound_structure *> const & buffers) : parent(parent), size_(0), mem_type_(mem) {
	
	std::vector<VkDeviceSize> offsets;
	for (vk::memory_bound_structure * bufp : buffers) {
		VkMemoryRequirements req = bufp->memory_requirements();
		VkDeviceSize next_align = next_alignment(size_, req.alignment);
		size_ = next_align + req.size;
		offsets.emplace_back(next_align);
	}
	
	VkMemoryAllocateInfo memory_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = size_,
		.memoryTypeIndex = mem->index,
	};
	VKR(parent.vkAllocateMemory(parent, &memory_allocate_info, nullptr, &handle))
	
	for (size_t i = 0; i < buffers.size(); i++) {
		buffers[i]->bind_to_memory(offsets[i], *this);
	}
}

vk::memory::~memory() {
	if (handle != VK_NULL_HANDLE) parent.vkFreeMemory(parent, handle, nullptr);
}

void * vk::memory::map(VkDeviceSize offset, VkDeviceSize size) {
	if (mapped_size != 0) unmap();
	mapped_offset = offset;
	mapped_size = size;
	void * region;
	VKR(parent.vkMapMemory(parent, handle, offset, size, 0, &region))
	return region;
}

void vk::memory::unmap() {
	if (!(mem_type_->type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
		VkMappedMemoryRange flush_range = {
			.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			.pNext = NULL,
			.memory = handle,
			.offset = mapped_offset,
			.size = mapped_size,
		};
		VKR(parent.vkFlushMappedMemoryRanges(parent, 1, &flush_range))
	}
	parent.vkUnmapMemory(parent, handle);
	mapped_offset = 0;
	mapped_size = 0;
}

void * vk::memory_bound_structure::map() {
	return bound_memory_->map(bound_offset_, size_);
}

void vk::memory_bound_structure::unmap() {
	bound_memory_->unmap();
}

vk::buffer::buffer(device const & parent, VkDeviceSize size, VkBufferUsageFlags usage) : memory_bound_structure(size), parent(parent), usage_(usage) {
	VkBufferCreateInfo buffer_create = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
	};
	VKR(parent.vkCreateBuffer(parent, &buffer_create, nullptr, &handle))
}

vk::buffer::~buffer() {
	if (handle != VK_NULL_HANDLE) parent.vkDestroyBuffer(parent, handle, nullptr);
}

VkMemoryRequirements vk::buffer::memory_requirements() const {
	VkMemoryRequirements req;
	parent.vkGetBufferMemoryRequirements(parent, handle, &req);
	return req;
}

void vk::buffer::bind_to_memory(VkDeviceSize offset, vk::memory & mem) {
	this->bound_offset_ = offset;
	this->bound_memory_ = &mem;
	VKR(parent.vkBindBufferMemory(parent, handle, mem.handle, offset))
}

vk::image::~image() {
	parent.vkDestroyImage(parent, handle, nullptr);
}

VkMemoryRequirements vk::image::memory_requirements() const {
	VkMemoryRequirements req;
	parent.vkGetImageMemoryRequirements(parent, handle, &req);
	return req;
}

void vk::image::bind_to_memory(VkDeviceSize offset, vk::memory & mem) {
	this->bound_offset_ = offset;
	this->bound_memory_ = &mem;
	VKR(parent.vkBindImageMemory(parent, handle, mem.handle, offset))
}
