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

#ifdef PROGENY_DEBUG
#define PROGENY_VK_DEBUG
#endif

namespace vk {
	
	#define VK_FN_EIDECL
	#include "vulkanomics_fn.inl"
	
	class exception : public std::runtime_error {
	public:
		exception(std::string const & str) : runtime_error(str) {}
	};
	
	struct device;
	
	struct physical_device {
		VkPhysicalDevice handle = VK_NULL_HANDLE;
		VkPhysicalDeviceProperties properties;
		std::vector<VkExtensionProperties> extensions;
		std::vector<VkLayerProperties> layers;
		std::vector<VkQueueFamilyProperties> queue_families;
		std::vector<VkBool32> queue_families_presentable;
		VkPhysicalDeviceMemoryProperties memory_properties;
		
		struct memory_type {
			uint32_t index;
			VkMemoryType const * type;
			VkMemoryHeap const * heap;
		};
		
		std::vector<memory_type> memory_types;
		
		memory_type const * memory_type_best_staging;
		memory_type const * memory_type_best_device_ideal;
		
		physical_device() = delete;
		physical_device(VkPhysicalDevice &);
		physical_device(physical_device const &) = delete;
		physical_device & operator = (physical_device const &) = delete;
		physical_device(physical_device &&) = default;
	};
	
	namespace instance {
		void init(); //initialize without surface
		void init(xcb_connection_t *, xcb_window_t &); //initialize with XCB surface
		void term() noexcept;
	}
	
	namespace surface {
		extern VkSurfaceKHR handle;
		extern VkSurfaceCapabilitiesKHR const & capabilities;
		extern std::vector<VkSurfaceFormatKHR> const & formats;
		extern std::vector<VkPresentModeKHR> const & present_modes;
		void setup(physical_device const &);
	}
	
	std::vector<vk::physical_device> const & get_physical_devices();
	
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
	
	struct shader {
		VkShaderModule handle = VK_NULL_HANDLE;
		device const & parent;
		
		shader() = delete;
		shader(device const & parent, uint8_t const * spv, size_t spv_len);
		
		~shader();
	};
	
	struct memory;
	
	struct memory_bound_structure {
		VkDeviceSize const & size() const {return size_;}
		virtual VkMemoryRequirements memory_requirements() const = 0;
		virtual void bind_to_memory(VkDeviceSize offset, vk::memory & mem) = 0;
		VkDeviceSize bound_offset() const { return bound_offset_; }
		memory const * bound_memory() const { return bound_memory_; }
		bool is_bound() const { return bound_memory_; }
		void * map();
		void unmap();
	protected:
		VkDeviceSize size_;
		memory * bound_memory_ = nullptr;
		VkDeviceSize bound_offset_ = 0;
		memory_bound_structure(VkDeviceSize size) : size_(size) {}
	};
	
	struct memory {
		
		VkDeviceMemory handle = VK_NULL_HANDLE;
		device const & parent;
		
		VkDeviceSize const & size() const {return size_;}
		vk::physical_device::memory_type const * memory_type() const { return mem_type_; }
		
		void * map(VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
		void unmap();
		
		memory() = delete;
		memory(device const & parent, vk::physical_device::memory_type const * mem, VkDeviceSize size);
		memory(device const & parent, vk::physical_device::memory_type const * mem, std::vector<vk::memory_bound_structure *> const &);
		~memory();
		
	private:
		VkDeviceSize size_;
		vk::physical_device::memory_type const * mem_type_;
		VkDeviceSize mapped_offset = 0;
		VkDeviceSize mapped_size = 0;
	};
	
	struct buffer : public memory_bound_structure {
		
		VkBuffer handle = VK_NULL_HANDLE;
		device const & parent;
		
		VkBufferUsageFlags const & usage() const {return usage_;}
		
		VkMemoryRequirements memory_requirements() const;
		void bind_to_memory(VkDeviceSize offset, vk::memory & mem);
	
		buffer() = delete;
		buffer(device const & parent, VkDeviceSize size, VkBufferUsageFlags usage);
		~buffer();
		
	private:
		VkBufferUsageFlags usage_;
	};
	
	struct image : public memory_bound_structure {
		
		VkImage handle = VK_NULL_HANDLE;
		device const & parent;
		
		VkMemoryRequirements memory_requirements() const;
		void bind_to_memory(VkDeviceSize offset, vk::memory & mem);
		
		image() = delete;
		~image();
	};
	
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
	
	namespace command {
		
		struct buffer;
		
		struct pool {
			VkCommandPool handle;
			device const & parent;
			
			buffer allocate_buffer(VkCommandBufferLevel lev = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			std::vector<buffer> allocate_buffers(uint32_t count, VkCommandBufferLevel lev = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
			
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
			void dispatch(uint32_t x, uint32_t y, uint32_t z);
			
		private:
			buffer(pool const & parent) : parent(parent) {}
		};
	}
}
