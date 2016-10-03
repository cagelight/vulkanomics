#include "vulkanomics.hpp"
#include "vk_internal.hpp"

vk::pipeline::layout::layout(device const & parent, VkPipelineLayoutCreateInfo const * create) : parent(parent) {
	VKR(parent.vkCreatePipelineLayout(parent, create, nullptr, &handle))
}

vk::pipeline::layout::layout(device const & parent, std::vector<VkDescriptorSetLayout> descriptor_sets, std::vector<VkPushConstantRange> push_constants) : parent(parent) {
	VkPipelineLayoutCreateInfo pipeline_layout_create = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.setLayoutCount = static_cast<uint32_t>(descriptor_sets.size()),
		.pSetLayouts = descriptor_sets.data(),
		.pushConstantRangeCount = static_cast<uint32_t>(push_constants.size()),
		.pPushConstantRanges = push_constants.data(),
	};
	VKR(parent.vkCreatePipelineLayout(parent, &pipeline_layout_create, nullptr, &handle))
}

vk::pipeline::layout::~layout() {
	if (handle != VK_NULL_HANDLE) {
		parent.vkDestroyPipelineLayout(parent, handle, nullptr);
	}
}

vk::pipeline::~pipeline() {
	if (handle != VK_NULL_HANDLE) parent.vkDestroyPipeline(parent, handle, nullptr);
}

vk::graphics_pipeline::graphics_pipeline(device const & parent, VkGraphicsPipelineCreateInfo const * create) : pipeline(parent) {
	VKR(parent.vkCreateGraphicsPipelines(parent, VK_NULL_HANDLE, 1, create, nullptr, &handle))
}

vk::compute_pipeline::compute_pipeline(device const & parent, VkComputePipelineCreateInfo const * create) : pipeline(parent) {
	VKR(parent.vkCreateComputePipelines(parent, VK_NULL_HANDLE, 1, create, nullptr, &handle))
}

vk::compute_pipeline::compute_pipeline(device const & parent, layout const & lay, shader const & sh, char const * entry_point, VkShaderStageFlagBits stage) : pipeline(parent) {
	VkComputePipelineCreateInfo pipeline_create = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = stage,
			.module = sh.handle,
			.pName = entry_point,
			.pSpecializationInfo = nullptr,
		},
		.layout = lay.handle,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0,
	};
	VKR(parent.vkCreateComputePipelines(parent, VK_NULL_HANDLE, 1, &pipeline_create, nullptr, &handle))
}
