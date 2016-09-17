#ifdef USE_RENDER_VULKAN
#ifndef RENDER_PARAMS_H
#define RENDER_PARAMS_H
#pragma once
#include <color.h>
#include <vector>
#include <array> 
#include "vulkan_functions.h"
#include "image.h"
#include "autodeleter.h"
#include "window_params.h"

#if defined(USE_PLATFORM_WIN32_KHR)
typedef HMODULE LibraryHandle;
#elif defined(USE_PLATFORM_XCB_KHR)
typedef void* LibraryHandle;
#endif

namespace HelloEngine 
{
	struct QueueParameters {
		VkQueue                       handle;
		uint32_t                      familyIndex;
		                                   
		QueueParameters() :
			handle(nullptr),
			familyIndex(0) {
		}
	};
	
	struct ImageParameters {
		VkImage                       handle;
		VkImageView                   view;
		VkSampler                     sampler;
		VkDeviceMemory                memory;

		ImageParameters() :
			handle(VK_NULL_HANDLE),
			view(VK_NULL_HANDLE),
			sampler(VK_NULL_HANDLE),
			memory(VK_NULL_HANDLE) {
		}
	};

	struct SwapChainParameters {
		VkSwapchainKHR                handle;
		VkFormat                      format;
		std::vector<ImageParameters>  images;
		VkExtent2D                    extent;

		SwapChainParameters() :
			handle(VK_NULL_HANDLE),
			format(VK_FORMAT_UNDEFINED),
			images(),
			extent() {
		}
	};

	struct BufferParameters {
		VkBuffer                      handle;
		VkDeviceMemory                memory;
		uint32_t                      size;

		BufferParameters() :
			handle(VK_NULL_HANDLE),
			memory(VK_NULL_HANDLE),
			size(0) {
		}
	};

	struct VertexData {
		float x, y, z, w;
		float u, v;
	};

	struct DescriptorSetParameters {
		VkDescriptorPool                pool;
		VkDescriptorSetLayout           layout;
		VkDescriptorSet                 handle;

		DescriptorSetParameters() :
			pool(VK_NULL_HANDLE),
			layout(VK_NULL_HANDLE),
			handle(VK_NULL_HANDLE) {
		}
	};

	struct RenderingResourcesData {
		VkFramebuffer                         framebuffer;
		VkCommandBuffer                       commandBuffer;
		VkSemaphore                           imageAvailableSemaphore;
		VkSemaphore                           finishedRenderingSemaphore;
		VkFence                               fence;

		RenderingResourcesData() :
			framebuffer(VK_NULL_HANDLE),
			commandBuffer(nullptr),
			imageAvailableSemaphore(VK_NULL_HANDLE),
			finishedRenderingSemaphore(VK_NULL_HANDLE),
			fence(VK_NULL_HANDLE) {
		}
	};	

	class RenderParameters {
	public:
		RenderParameters();
		~RenderParameters();		
		bool								Initialize(WindowParameters *parameters, const std::vector<float>& vertex_data, Color color);
		bool								Draw();	
		bool								OnWindowSizeChanged();
		bool								ReadyToDraw() const;
	private:
		Color								m_ClearColor;
		bool								m_CanRender;
		static const size_t					RESOURCE_COUNT = 3;
		VkInstance							m_Instance;
		VkPhysicalDevice					m_PhysicalDevice;
		VkDevice							m_Device;
		VkSurfaceKHR						m_PresentationSurface;
		VkCommandPool						m_PresentQueueCmdPool;
		VkRenderPass						m_RenderPass;
		VkPipeline							m_GraphicsPipeline;
		VkCommandPool						m_CommandPool;
		WindowParameters				   *m_Window;
		QueueParameters						m_GraphicsQueue;
		QueueParameters						m_PresentQueue;
		SwapChainParameters					m_SwapChain;
		BufferParameters					m_VertexBuffer;
		BufferParameters                    m_StagingBuffer;
		BufferParameters                    m_UniformBuffer;
		ImageParameters                     m_Image;
		DescriptorSetParameters             m_DescriptorSet;
		VkPipelineLayout                    m_PipelineLayout;
		std::vector<RenderingResourcesData> m_RenderingResources;
		LibraryHandle						m_VulkanLibrary;

		bool CreateInstance();
		bool CreatePresentationSurface();
		bool CreateDevice();
		bool CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device, uint32_t &graphics_queue_family_index, uint32_t &present_queue_family_index) const;
		bool GetDeviceQueue();
		bool CreateRenderingResources();
		bool CreateSwapChain();
		bool CreateCommandBuffers();
		bool CreateSemaphores();
		bool CreateRenderPass();
		bool CreateSwapChainImageViews();
		bool CreateFramebuffer(VkFramebuffer &framebuffer, VkImageView image_view) const;
		bool CreatePipeline();
		bool CopyVertexData(const std::vector<float>& vertex_data);
		bool CreateVertexBuffer(const std::vector<float>& vertex_data);
		bool CreateFences();	
		bool CreateStagingBuffer();
		bool CreateDescriptorSetLayout();
		bool CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlagBits memoryProperty, BufferParameters &buffer);
		bool PrepareFrame(VkCommandBuffer command_buffer, const ImageParameters &image_parameters, VkFramebuffer &framebuffer) const;
		bool AllocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlagBits property, VkDeviceMemory *memory) const;
		bool CreateCommandPool(uint32_t queue_family_index, VkCommandPool *pool) const;
		bool AllocateCommandBuffers(VkCommandPool pool, uint32_t count, VkCommandBuffer *command_buffers) const;
		bool LoadVulkanLibrary();
		bool LoadExportedEntryPoints() const;
		bool LoadGlobalLevelEntryPoints() const;   
		bool LoadInstanceLevelEntryPoints() const;
		bool LoadDeviceLevelEntryPoints() const;	
		bool CreateTexture();
		bool CreateDescriptorPool();
		bool CreateImage(uint32_t width, uint32_t height, VkImage *image) const;
		bool AllocateDescriptorSet();
		bool AllocateImageMemory(VkMemoryPropertyFlagBits property, VkDeviceMemory *memory);
		bool CreateUniformBuffer();
		bool CreateImageView(ImageParameters &image_parameters);
		bool CreateSampler(VkSampler *sampler);
		bool CreatePipelineLayout();
		bool CopyTextureData(const Image &image);
		bool CopyUniformBufferData();
		bool UpdateDescriptorSet();
		const std::array<float, 16> GetUniformBufferData() const;
		void DestroyBuffer(BufferParameters& buffer) const;

		static bool                          CheckExtensionAvailability(const char *extension_name, const std::vector<VkExtensionProperties> &available_extensions);
		static uint32_t                      GetSwapChainNumImages(VkSurfaceCapabilitiesKHR &surface_capabilities);
		static VkSurfaceFormatKHR            GetSwapChainFormat(std::vector<VkSurfaceFormatKHR> &surface_formats);
		static VkExtent2D                    GetSwapChainExtent(VkSurfaceCapabilitiesKHR &surface_capabilities);
		static VkImageUsageFlags             GetSwapChainUsageFlags(VkSurfaceCapabilitiesKHR &surface_capabilities);
		static VkSurfaceTransformFlagBitsKHR GetSwapChainTransform(VkSurfaceCapabilitiesKHR &surface_capabilities);
		static VkPresentModeKHR              GetSwapChainPresentMode(std::vector<VkPresentModeKHR> &present_modes);
	
		AutoDeleter<VkPipelineLayout, PFN_vkDestroyPipelineLayout>  CreatePipelineLayout() const;
		AutoDeleter<VkShaderModule, PFN_vkDestroyShaderModule>		CreateShaderModule(const char* filename) const;
	};
}
#endif
#endif
