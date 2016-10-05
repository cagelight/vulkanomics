#pragma once

#include <mutex>
#include <vector>
#include <stdexcept>
#include <algorithm>

#include <xcb/xcb.h>

#pragma once
#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

//================================================================
//----------------------------------------------------------------
//================================================================

namespace vk {
	
	#define VK_FN_EIDECL
	#include "vulkanomics_fn.inl"
	
	class exception : public std::runtime_error {
	public:
		exception(std::string const & str) : runtime_error(str) {}
	};
	
//================================================================
//----------------------------------------------------------------
//================================================================
// PHYSICAL DEVICE
	
	struct device;
	
	struct physical_device {
		VkPhysicalDevice handle = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties properties;
		std::vector<VkExtensionProperties> extensions;
		std::vector<VkLayerProperties> layers;
		std::vector<VkQueueFamilyProperties> queue_families;
		std::vector<VkBool32> queue_families_presentable;
		VkPhysicalDeviceMemoryProperties memory_properties;
		
		uint32_t find_staging_memory(uint32_t restrict_mask = UINT32_MAX) const;
		uint32_t find_device_memory(uint32_t restrict_mask = UINT32_MAX) const;
		
		physical_device() = delete;
		physical_device(VkPhysicalDevice &);
		physical_device(physical_device const &) = delete;
		physical_device & operator = (physical_device const &) = delete;
		physical_device(physical_device &&) = default;
	};
	
	std::vector<vk::physical_device> const & get_physical_devices();
	
//================================================================
//----------------------------------------------------------------
//================================================================
// INSTANCE
	
	namespace instance {
		void init(); //initialize without surface
		void init(xcb_connection_t *, xcb_window_t &); //initialize with XCB surface
		void term() noexcept;
	}
	
//================================================================
//----------------------------------------------------------------
//================================================================
// SURFACE
	
	namespace surface {
		extern VkSurfaceKHR handle;
		extern VkSurfaceCapabilitiesKHR const & capabilities;
		extern std::vector<VkSurfaceFormatKHR> const & formats;
		extern std::vector<VkPresentModeKHR> const & present_modes;
		void setup(physical_device const &);
	}
	
//================================================================
//----------------------------------------------------------------
//================================================================
// LOGICAL DEVICE
	
	struct device {
		
		struct capability { //ordered least important to most important, for sorting
			static constexpr uint32_t transfer = 1 << 0;
			static constexpr uint32_t compute = 1 << 1;
			static constexpr uint32_t graphics = 1 << 2;
			static constexpr uint32_t presentable = 1 << 3;
			typedef uint32_t flags;
		};
		
		typedef std::vector<capability::flags> capability_set;
		static inline void capability_sort(capability_set & set) {
			std::sort(set.begin(), set.end(), [](capability::flags a, capability::flags b){return a>b;});
		}
		
		struct queue {
			VkQueue handle = VK_NULL_HANDLE;
			capability::flags cap_flags;
			uint32_t queue_family;
		};
		
		struct protoqueue {
			uint32_t queue_index;
			capability::flags cap_flags;
			uint32_t queue_family;
		};
		
		struct initializer {
			physical_device const & parent;
			std::vector<VkDeviceQueueCreateInfo> create_infos;
			std::vector<std::vector<float>> queue_priorities;
			std::vector<protoqueue> protoqueues;
			capability::flags overall_capability = 0;
			std::vector<char const *> device_extensions;
			std::vector<char const *> device_layers {
			#ifdef PROGENY_VK_DEBUG
				"VK_LAYER_LUNARG_standard_validation",
			#endif
			};
			
			initializer() = delete;
			initializer(physical_device const &, capability_set const &);
		};
		
		physical_device const & parent;
		std::vector<queue> queues;
		capability::flags overall_capability;
		std::vector<char const *> device_extensions;
		std::vector<char const *> device_layers;
		#define VK_FN_DDECL
		#include "vulkanomics_fn.inl"
		
		device() = delete;
		device(initializer &);
		device(device const &) = delete;
		device(device &&) = delete;
		
		device & operator = (device const &) = delete;
		bool operator == (device const & other) {return this->handle == other.handle;}
		operator VkDevice const & () const { return handle; }
		
		~device();
		
	private:
		VkDevice handle = VK_NULL_HANDLE;
	};
	
//================================================================
//----------------------------------------------------------------
//================================================================
// FENCE & SEMAPHORE
	
	struct fence {
		device const & parent;
		fence(device const & parent);
		~fence();
		void reset();
		void wait(uint64_t timeout = UINT64_MAX);
		operator VkFence const & () const {return handle;}
	private:
		VkFence handle;
	};
	
	struct semaphore {
		device const & parent;
		semaphore(device const & parent);
		~semaphore();
		operator VkSemaphore const & () const {return handle;}
	private:
		VkSemaphore handle;
	};
	
//================================================================
//----------------------------------------------------------------
//================================================================
// QUEUE
	
	class queue_accessor {
	protected:
		vk::device::queue & queue;
	public:
		virtual void submit(VkSubmitInfo * infos, uint32_t infos_count, VkFence fence) = 0;
		virtual ~queue_accessor() {}
		device::capability::flags const & cap_flags;
		uint32_t const & queue_family;
		inline bool transfer_capable() { return cap_flags & device::capability::transfer; }
		inline bool compute_capable() { return cap_flags & device::capability::compute; }
		inline bool graphics_capable() { return cap_flags & device::capability::graphics; }
		inline bool present_capable() { return cap_flags & device::capability::presentable; }
	protected:
		vk::device & parent;
		queue_accessor() = delete;
		queue_accessor(vk::device & parent, uint32_t index) : queue(parent.queues[index]), cap_flags(queue.cap_flags), queue_family(queue.queue_family), parent(parent)  {}
		queue_accessor(queue_accessor const &) = delete;
		queue_accessor(queue_accessor &&) = delete;
		queue_accessor & operator = (queue_accessor const &) = delete;
		queue_accessor & operator = (queue_accessor &&) = delete;
	};
	
	class queue_accessor_direct : public queue_accessor {
	public:
		queue_accessor_direct(vk::device & parent, uint32_t index) : queue_accessor(parent, index) {}
		~queue_accessor_direct() {}
		void submit(VkSubmitInfo * infos, uint32_t infos_count, VkFence fence);
	};
	
	class queue_accessor_mutexed : public queue_accessor {
		queue_accessor_mutexed(vk::device & parent, uint32_t index) : queue_accessor(parent, index) {}
		~queue_accessor_mutexed() {}
		void submit(VkSubmitInfo * infos, uint32_t infos_count, VkFence fence);
	private:
		std::mutex mut;
	};
	
//================================================================
//----------------------------------------------------------------
//================================================================
// SHADER
	
	struct shader {
		VkShaderModule handle = VK_NULL_HANDLE;
		device const & parent;
		
		shader() = delete;
		shader(device const & parent, uint8_t const * spv, size_t spv_len);
		
		~shader();
	};
	
//================================================================
//----------------------------------------------------------------
//================================================================
// MEMORY
	
	struct memory;
	
	struct memory_bound_structure {
		
		virtual VkMemoryRequirements memory_requirements() const = 0;
		virtual void bind_to_memory(VkDeviceSize offset, vk::memory & mem) = 0;
		VkDeviceSize bound_offset() const { return bound_offset_; }
		memory const * bound_memory() const { return bound_memory_; }
		bool is_bound() const { return bound_memory_; }
		void * map();
		void unmap();
		virtual ~memory_bound_structure() {}
	protected:
		memory * bound_memory_ = nullptr;
		VkDeviceSize bound_offset_ = 0;
		memory_bound_structure() {}
	};
	
	struct memory {
		
		VkDeviceMemory handle = VK_NULL_HANDLE;
		device const & parent;
		
		VkDeviceSize const & size() const {return size_;}
		uint32_t memory_type() const { return mem_type_; }
		
		void * map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
		void unmap();
		
		memory() = delete;
		memory(device const & parent, uint32_t mem, VkDeviceSize size);
		memory(device const & parent, uint32_t mem, std::vector<vk::memory_bound_structure *> const &);
		~memory();
		
	private:
		VkDeviceSize size_;
		uint32_t mem_type_;
		VkDeviceSize mapped_offset = 0;
		VkDeviceSize mapped_size = 0;
	};
	
//================================================================
//----------------------------------------------------------------
//================================================================
// MEMORY BOUND
	
	struct buffer : public memory_bound_structure {
		
		VkBuffer handle = VK_NULL_HANDLE;
		device const & parent;
		
		VkBufferUsageFlags const & usage() const {return usage_;}
		
		VkDeviceSize const & size() const {return size_;}
		VkMemoryRequirements memory_requirements() const;
		void bind_to_memory(VkDeviceSize offset, vk::memory & mem);
		VkDescriptorBufferInfo descript(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
	
		buffer() = delete;
		buffer(device const & parent, VkDeviceSize size, VkBufferUsageFlags usage);
		~buffer();
		
	private:
		VkDeviceSize size_;
		VkBufferUsageFlags usage_;
	};
	
	struct image : public memory_bound_structure {
		
		struct view {
			image const & parent;
			
			view(image const & parent, VkImageViewType view_type, VkImageAspectFlags aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT, uint32_t base_mip = 0, uint32_t base_level = 0, VkComponentMapping cmap = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A});
			~view();
			
			operator VkImageView const & () const {return handle;}
		private:
			VkImageView handle = VK_NULL_HANDLE;
		};
		
		device const & parent;
		
		VkImageUsageFlags const & usage() const {return usage_;}
		VkFormat const & format() const {return format_;}
		VkImageType const & image_type() const {return image_type_;}
		VkImageLayout const & layout() const {return layout_;}
		
		VkMemoryRequirements memory_requirements() const;
		void bind_to_memory(VkDeviceSize offset, vk::memory & mem);
		
		image() = delete;
		image(
			/*required*/
			device const & parent, 
			VkImageType type, 
			VkFormat format, 
			VkExtent3D extent, 
			VkImageUsageFlags usage, 
			/*optional*/
			VkImageCreateFlags flags = 0, 
			VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL, 
			VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED,
			uint32_t mip_levels = 1, 
			uint32_t layers = 1, 
			VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT,
			VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE,
			uint32_t const * queue_indicies = nullptr, //when sharing_mode is VK_SHARING_MODE_CONCURRENT
			uint32_t queue_indicies_count = 0 //when sharing_mode is VK_SHARING_MODE_CONCURRENT
		);
		virtual ~image();
		
		operator VkImage const & () const {return handle;}
	private:
		VkImage handle = VK_NULL_HANDLE;
		VkImageUsageFlags usage_;
		VkFormat format_;
		VkImageType image_type_;
		VkImageLayout layout_;
	};
	
//================================================================
//----------------------------------------------------------------
//================================================================
// PIPELINE
	
	struct pipeline {
		
		VkPipeline handle = VK_NULL_HANDLE;
		device const & parent;
		
		struct layout {
			VkPipelineLayout handle = VK_NULL_HANDLE;
			device const & parent;
			
			layout(device const & parent, VkPipelineLayoutCreateInfo const *);
			layout(device const & parent, std::vector<VkDescriptorSetLayout>, std::vector<VkPushConstantRange>);
			~layout();
		};
		
		operator VkPipeline const & () const {return handle;}
		
	protected:
		pipeline(device const & parent) : parent(parent) {}
		virtual ~pipeline();
	};
	
	struct graphics_pipeline : public pipeline {
		graphics_pipeline(device const & parent, VkGraphicsPipelineCreateInfo const * create);
	};
	
	struct compute_pipeline : public pipeline {
		compute_pipeline(device const & parent, VkComputePipelineCreateInfo const * create);
		compute_pipeline(device const & parent, layout const &, shader const &, char const * entry_point, VkShaderStageFlagBits stage = VK_SHADER_STAGE_COMPUTE_BIT);
	};
	
//================================================================
//----------------------------------------------------------------
//================================================================
// DESCRIPTOR

	namespace descriptor {
		
		typedef std::vector<VkDescriptorPoolSize> pool_size_set;
		typedef std::vector<VkDescriptorBufferInfo> buffer_info_set;
		
		struct layout {
			device const & parent;
			layout(device const &, std::vector<VkDescriptorSetLayoutBinding> const & bindings);
			~layout();
			
			VkDescriptorSetLayout const & get_handle() const {return handle;}
			operator VkDescriptorSetLayout const & () const {return handle;}
		private:
			VkDescriptorSetLayout handle;
		};
		
		struct pool {
			device const & parent;
			pool(device const &, pool_size_set const & pool_sizes, uint32_t max_sets);
			~pool();
			operator VkDescriptorPool const & () const {return handle;}
		private:
			VkDescriptorPool handle;
		};
		
		struct set { friend struct pool;
			pool const & parent;
			set(pool const &, layout const & lay);
			~set();
			operator VkDescriptorSet const & () const {return handle;}
		private:
			VkDescriptorSet handle;
		};
		
		struct update_session {
			update_session(device const & p) : parent(p) {}
			~update_session() = default;
			void update();
			void copy(set & src, set & dst, uint32_t src_binding, uint32_t dst_binding, uint32_t count = 1, uint32_t src_index = 0, uint32_t dst_index = 0);
			void write_buffer(set &, uint32_t binding, uint32_t index, VkDescriptorType type, buffer_info_set const &);
		private:
			device const & parent;
			std::vector<VkWriteDescriptorSet> wset {};
			std::vector<VkCopyDescriptorSet> cset {};
		};
	}
	
//================================================================
//----------------------------------------------------------------
//================================================================
// COMMAND
	
	namespace command {
		
		struct buffer;
		
		struct pool {
			VkCommandPool handle;
			device const & parent;
			
			pool(device const &, VkCommandPoolCreateFlags flags, uint32_t queue_family);
			~pool();
		};
		
		struct buffer { friend struct pool;
			VkCommandBuffer handle;
			pool const & parent;
			
			void begin(VkCommandBufferUsageFlags flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VkCommandBufferInheritanceInfo const * inheritance = nullptr);
			void end();
			
			void bind_compute_pipeline(compute_pipeline const &);
			void bind_graphics_pipeline(graphics_pipeline const &);
			void bind_descriptor_sets(VkPipelineBindPoint bind_point, pipeline::layout & layout, std::vector<descriptor::set const *> const & descriptors);
			void dispatch(uint32_t x, uint32_t y, uint32_t z);
			void barrier(VkPipelineStageFlags stages_src, VkPipelineStageFlags stages_dst, std::vector<VkMemoryBarrier> const &, std::vector<VkBufferMemoryBarrier> const &, std::vector<VkImageMemoryBarrier> const &, VkDependencyFlags dep = 0);
			
			buffer(pool const & parent, VkCommandBufferLevel lev = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			buffer(buffer const &) = delete;
			buffer(buffer &&) = delete;
			~buffer();
			
		private:

		};
	}
	
//================================================================
//----------------------------------------------------------------
//================================================================
	
}
