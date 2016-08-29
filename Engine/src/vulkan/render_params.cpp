#ifdef USE_RENDER_VULKAN
#include "tools.h"
#include "render_params.h"
#include "renderer.h"
#include <string.h>
#include <iostream>

namespace HelloEngine
{
	bool Renderer::Initialize(WindowParameters *parameters, const std::vector<float>& vertex_data, Color color)
	{
		return m_Params->Initialize(parameters, vertex_data, color);
	}

	bool Renderer::OnWindowSizeChanged()
	{
		return m_Params->OnWindowSizeChanged();
	}

	bool Renderer::ReadyToDraw() const
	{
		return m_Params->ReadyToDraw();
	}

	bool Renderer::Draw()
	{
		return m_Params->Draw();
	}	

	RenderParameters::RenderParameters() :
		m_CanRender(false),
	    m_Instance(nullptr),
		m_PhysicalDevice(nullptr),
		m_Device(nullptr),
		m_PresentationSurface(VK_NULL_HANDLE),
		m_PresentQueueCmdPool(VK_NULL_HANDLE),
		m_RenderPass(VK_NULL_HANDLE),
		m_GraphicsPipeline(VK_NULL_HANDLE),
		m_CommandPool(VK_NULL_HANDLE),
		m_Window(),
		m_GraphicsQueue(),
		m_PresentQueue(),
		m_SwapChain(),
		m_VertexBuffer(),
		m_RenderingResources(RESOURCE_COUNT),
		m_VulkanLibrary()
	{
	}

	bool RenderParameters::Initialize(WindowParameters *parameters, const std::vector<float>& vertex_data, Color color) {
		m_Window = parameters;
		m_ClearColor = color;
		if (!LoadVulkanLibrary()) {
			return false;
		}
		if (!LoadExportedEntryPoints()) {
			return false;
		}
		if (!LoadGlobalLevelEntryPoints()) {
			return false;
		}
		if (!CreateInstance()) {
			return false;
		}
		if (!LoadInstanceLevelEntryPoints()) {
			return false;
		}
		if (!CreatePresentationSurface()) {
			return false;
		}
		if (!CreateDevice()) {
			return false;
		}
		if (!LoadDeviceLevelEntryPoints()) {
			return false;
		}
		if (!GetDeviceQueue()) {
			return false;
		}
		if (!CreateSwapChain()) {
			return false;
		}
		if (!CreateRenderPass()) {
			return false;
		}
		if (!CreatePipeline()) {
			return false;
		}		
		if (!CreateRenderingResources()) {
			return false;
		}
		if (!CreateVertexBuffer(vertex_data)) {
			return false;
		}
		if (!CreateStagingBuffer()) {
			return false;
		}
		if (!CopyVertexData(vertex_data)) {
			return false;
		}
		return true;
	}	

	bool RenderParameters::ReadyToDraw() const
	{
		return m_CanRender;
	}

	bool RenderParameters::OnWindowSizeChanged()
	{
		return CreateSwapChain();
	}

	bool RenderParameters::Draw()
	{
		static size_t           resource_index = 0;
		RenderingResourcesData &current_rendering_resource = m_RenderingResources[resource_index];
		VkSwapchainKHR          swap_chain = m_SwapChain.handle;
		uint32_t                image_index;

		resource_index = (resource_index + 1) % RESOURCE_COUNT;

		if (vkWaitForFences(m_Device, 1, &current_rendering_resource.fence, VK_FALSE, 1000000000) != VK_SUCCESS) {
			std::cout << "Waiting for fence takes too long!" << std::endl;
			return false;
		}
		vkResetFences(m_Device, 1, &current_rendering_resource.fence);

		VkResult result = vkAcquireNextImageKHR(m_Device, swap_chain, UINT64_MAX, current_rendering_resource.imageAvailableSemaphore, VK_NULL_HANDLE, &image_index);
		switch (result) {
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
			return OnWindowSizeChanged();
		default:
			std::cout << "Problem occurred during swap chain image acquisition!" << std::endl;
			return false;
		}

		if (!PrepareFrame(current_rendering_resource.commandBuffer, m_SwapChain.images[image_index], current_rendering_resource.framebuffer)) {
			return false;
		}

		VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submit_info = {
			VK_STRUCTURE_TYPE_SUBMIT_INFO,                          // VkStructureType              sType
			nullptr,                                                // const void                  *pNext
			1,                                                      // uint32_t                     waitSemaphoreCount
			&current_rendering_resource.imageAvailableSemaphore,    // const VkSemaphore           *pWaitSemaphores
			&wait_dst_stage_mask,                                   // const VkPipelineStageFlags  *pWaitDstStageMask;
			1,                                                      // uint32_t                     commandBufferCount
			&current_rendering_resource.commandBuffer,              // const VkCommandBuffer       *pCommandBuffers
			1,                                                      // uint32_t                     signalSemaphoreCount
			&current_rendering_resource.finishedRenderingSemaphore  // const VkSemaphore           *pSignalSemaphores
		};

		if (vkQueueSubmit(m_GraphicsQueue.handle, 1, &submit_info, current_rendering_resource.fence) != VK_SUCCESS) {
			return false;
		}

		VkPresentInfoKHR present_info = {
			VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,                     // VkStructureType              sType
			nullptr,                                                // const void                  *pNext
			1,                                                      // uint32_t                     waitSemaphoreCount
			&current_rendering_resource.finishedRenderingSemaphore, // const VkSemaphore           *pWaitSemaphores
			1,                                                      // uint32_t                     swapchainCount
			&swap_chain,                                            // const VkSwapchainKHR        *pSwapchains
			&image_index,                                           // const uint32_t              *pImageIndices
			nullptr                                                 // VkResult                    *pResults
		};
		result = vkQueuePresentKHR(m_PresentQueue.handle, &present_info);

		switch (result) {
		case VK_SUCCESS:
			break;
		case VK_ERROR_OUT_OF_DATE_KHR:
		case VK_SUBOPTIMAL_KHR:
			return OnWindowSizeChanged();
		default:
			std::cout << "Problem occurred during image presentation!" << std::endl;
			return false;
		}

		return true;
	}

	bool RenderParameters::CreateInstance() {
		uint32_t extensions_count = 0;
		if ((vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, nullptr) != VK_SUCCESS) ||
			(extensions_count == 0)) {
			std::cout << "Error occurred during instance extensions enumeration!\n";
			return false;
		}

		std::vector<VkExtensionProperties> available_extensions(extensions_count);
		if (vkEnumerateInstanceExtensionProperties(nullptr, &extensions_count, &available_extensions[0]) != VK_SUCCESS) {
			std::cout << "Error occurred during instance extensions enumeration!\n";
			return false;
		}

		std::vector<const char*> instance_extensions = {
			VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(USE_PLATFORM_WIN32_KHR)
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(USE_PLATFORM_XCB_KHR)
			VK_KHR_XCB_SURFACE_EXTENSION_NAME
#endif
		};
		for (size_t i = 0; i < instance_extensions.size(); ++i) {
			if (!CheckExtensionAvailability(instance_extensions[i], available_extensions)) {
				std::cout << "Could not find instance extension named \"" << instance_extensions[i] << "\"!\n";
				return false;
			}
		}
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pEngineName = "Test Engine";
		app_info.pApplicationName = "Pavel Test";
		app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);

		VkInstanceCreateInfo create_info{};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		create_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
		create_info.ppEnabledExtensionNames = &instance_extensions[0];

		if (vkCreateInstance(&create_info, nullptr, &m_Instance) != VK_SUCCESS) {
			std::cout << "Could not create Vulkan instance!\n";
			return false;
		}
		return true;
	}

	bool RenderParameters::CheckExtensionAvailability(const char *extension_name, const std::vector<VkExtensionProperties> &available_extensions) {
		for (size_t i = 0; i < available_extensions.size(); ++i) {
			if (strcmp(available_extensions[i].extensionName, extension_name) == 0) {
				return true;
			}
		}
		return false;
	}

	bool RenderParameters::CreateDevice() {
		uint32_t num_devices = 0;
		if ((vkEnumeratePhysicalDevices(m_Instance, &num_devices, nullptr) != VK_SUCCESS) ||
			(num_devices == 0)) {
			std::cout << "Error occurred during physical devices enumeration!\n";
			return false;
		}

		std::vector<VkPhysicalDevice> physical_devices(num_devices);
		if (vkEnumeratePhysicalDevices(m_Instance, &num_devices, &physical_devices[0]) != VK_SUCCESS) {
			std::cout << "Error occurred during physical devices enumeration!\n";
			return false;
		}

		uint32_t selected_graphics_queue_family_index = UINT32_MAX;
		uint32_t selected_present_queue_family_index = UINT32_MAX;

		for (uint32_t i = 0; i < num_devices; ++i) {
			if (CheckPhysicalDeviceProperties(physical_devices[i], selected_graphics_queue_family_index, selected_present_queue_family_index)) {
				m_PhysicalDevice = physical_devices[i];
			}
		}
		if (m_PhysicalDevice == nullptr) {
			std::cout << "Could not select physical device based on the chosen properties!\n";
			return false;
		}

		std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
		std::vector<float> queue_priorities = { 1.0f };

		queue_create_infos.push_back({
			VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,       // VkStructureType              sType
			nullptr,                                          // const void                  *pNext
			0,                                                // VkDeviceQueueCreateFlags     flags
			selected_graphics_queue_family_index,             // uint32_t                     queueFamilyIndex
			static_cast<uint32_t>(queue_priorities.size()),   // uint32_t                     queueCount
			&queue_priorities[0]                              // const float                 *pQueuePriorities
		});

		if (selected_graphics_queue_family_index != selected_present_queue_family_index) {
			queue_create_infos.push_back({
				VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,     // VkStructureType              sType
				nullptr,                                        // const void                  *pNext
				0,                                              // VkDeviceQueueCreateFlags     flags
				selected_present_queue_family_index,            // uint32_t                     queueFamilyIndex
				static_cast<uint32_t>(queue_priorities.size()), // uint32_t                     queueCount
				&queue_priorities[0]                            // const float                 *pQueuePriorities
			});
		}

		std::vector<const char*> extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		VkDeviceCreateInfo device_create_info = {
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,             // VkStructureType                    sType
			nullptr,                                          // const void                        *pNext
			0,                                                // VkDeviceCreateFlags                flags
			static_cast<uint32_t>(queue_create_infos.size()), // uint32_t                           queueCreateInfoCount
			&queue_create_infos[0],                           // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
			0,                                                // uint32_t                           enabledLayerCount
			nullptr,                                          // const char * const                *ppEnabledLayerNames
			static_cast<uint32_t>(extensions.size()),         // uint32_t                           enabledExtensionCount
			&extensions[0],                                   // const char * const                *ppEnabledExtensionNames
			nullptr                                           // const VkPhysicalDeviceFeatures    *pEnabledFeatures
		};

		if (vkCreateDevice(m_PhysicalDevice, &device_create_info, nullptr, &m_Device) != VK_SUCCESS) {
			std::cout << "Could not create Vulkan device!\n";
			return false;
		}

		m_GraphicsQueue.familyIndex = selected_graphics_queue_family_index;
		m_PresentQueue.familyIndex = selected_present_queue_family_index;
		return true;
	}

	bool RenderParameters::CheckPhysicalDeviceProperties(VkPhysicalDevice physical_device, uint32_t &selected_graphics_queue_family_index, uint32_t &selected_present_queue_family_index) const
	{
		uint32_t extensions_count = 0;
		if ((vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, nullptr) != VK_SUCCESS) ||
			(extensions_count == 0)) {
			std::cout << "Error occurred during physical device " << physical_device << " extensions enumeration!\n";
			return false;
		}

		std::vector<VkExtensionProperties> available_extensions(extensions_count);
		if (vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extensions_count, &available_extensions[0]) != VK_SUCCESS) {
			std::cout << "Error occurred during physical device " << physical_device << " extensions enumeration!\n";
			return false;
		}

		std::vector<const char*> device_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		for (size_t i = 0; i < device_extensions.size(); ++i) {
			if (!CheckExtensionAvailability(device_extensions[i], available_extensions)) {
				std::cout << "Physical device " << physical_device << " doesn't support extension named \"" << device_extensions[i] << "\"!\n";
				return false;
			}
		}

		VkPhysicalDeviceProperties device_properties;
		VkPhysicalDeviceFeatures   device_features;

		vkGetPhysicalDeviceProperties(physical_device, &device_properties);
		vkGetPhysicalDeviceFeatures(physical_device, &device_features);

		uint32_t major_version = VK_VERSION_MAJOR(device_properties.apiVersion);

		if ((major_version < 1) ||
			(device_properties.limits.maxImageDimension2D < 4096)) {
			std::cout << "Physical device " << physical_device << " doesn't support required parameters!\n";
			return false;
		}

		uint32_t queue_families_count = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, nullptr);
		if (queue_families_count == 0) {
			std::cout << "Physical device " << physical_device << " doesn't have any queue families!\n";
			return false;
		}

		std::vector<VkQueueFamilyProperties>  queue_family_properties(queue_families_count);
		std::vector<VkBool32>                 queue_present_support(queue_families_count);

		vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_families_count, &queue_family_properties[0]);

		uint32_t graphics_queue_family_index = UINT32_MAX;
		uint32_t present_queue_family_index = UINT32_MAX;

		for (uint32_t i = 0; i < queue_families_count; ++i) {
			vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, m_PresentationSurface, &queue_present_support[i]);

			if ((queue_family_properties[i].queueCount > 0) &&
				(queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
				if (graphics_queue_family_index == UINT32_MAX) {
					graphics_queue_family_index = i;
				}
				if (queue_present_support[i]) {
					selected_graphics_queue_family_index = i;
					selected_present_queue_family_index = i;
					return true;
				}
			}
		}
		for (uint32_t i = 0; i < queue_families_count; ++i) {
			if (queue_present_support[i]) {
				present_queue_family_index = i;
				break;
			}
		}
		if ((graphics_queue_family_index == UINT32_MAX) ||
			(present_queue_family_index == UINT32_MAX)) {
			std::cout << "Could not find queue family with required properties on physical device " << physical_device << "!\n";
			return false;
		}

		selected_graphics_queue_family_index = graphics_queue_family_index;
		selected_present_queue_family_index = present_queue_family_index;
		return true;
	}

	bool RenderParameters::GetDeviceQueue() {
		vkGetDeviceQueue(m_Device, m_GraphicsQueue.familyIndex, 0, &m_GraphicsQueue.handle);
		vkGetDeviceQueue(m_Device, m_PresentQueue.familyIndex, 0, &m_PresentQueue.handle);
		return true;
	}

	bool RenderParameters::CreateRenderingResources()
	{
		if (!CreateCommandBuffers()) {
			return false;
		}
		if (!CreateSemaphores()) {
			return false;
		}
		if (!CreateFences()) {
			return false;
		}
		return true;
	}

	bool RenderParameters::CreateSemaphores() {
		VkSemaphoreCreateInfo semaphore_create_info{};
		semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		for (size_t i = 0; i < m_RenderingResources.size(); ++i) {
			if ((vkCreateSemaphore(m_Device, &semaphore_create_info, nullptr, &m_RenderingResources[i].imageAvailableSemaphore) != VK_SUCCESS) ||
				(vkCreateSemaphore(m_Device, &semaphore_create_info, nullptr, &m_RenderingResources[i].finishedRenderingSemaphore) != VK_SUCCESS)) {
				std::cout << "Could not create semaphores!" << std::endl;
				return false;
			}
		}
		return true;
	}

	bool RenderParameters::CreateSwapChain() {
		m_CanRender = false;

		if (m_Device != nullptr) {
			vkDeviceWaitIdle(m_Device);
		}

		VkSurfaceCapabilitiesKHR surface_capabilities;
		if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_PresentationSurface, &surface_capabilities) != VK_SUCCESS) {
			std::cout << "Could not check presentation surface capabilities!\n";
			return false;
		}

		uint32_t formats_count;
		if ((vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_PresentationSurface, &formats_count, nullptr) != VK_SUCCESS) ||
			(formats_count == 0)) {
			std::cout << "Error occurred during presentation surface formats enumeration!\n";
			return false;
		}

		std::vector<VkSurfaceFormatKHR> surface_formats(formats_count);
		if (vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_PresentationSurface, &formats_count, &surface_formats[0]) != VK_SUCCESS) {
			std::cout << "Error occurred during presentation surface formats enumeration!\n";
			return false;
		}

		uint32_t present_modes_count;
		if ((vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_PresentationSurface, &present_modes_count, nullptr) != VK_SUCCESS) ||
			(present_modes_count == 0)) {
			std::cout << "Error occurred during presentation surface present modes enumeration!\n";
			return false;
		}

		std::vector<VkPresentModeKHR> present_modes(present_modes_count);
		if (vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_PresentationSurface, &present_modes_count, &present_modes[0]) != VK_SUCCESS) {
			std::cout << "Error occurred during presentation surface present modes enumeration!\n";
			return false;
		}

		uint32_t                      desired_number_of_images = GetSwapChainNumImages(surface_capabilities);
		VkSurfaceFormatKHR            desired_format = GetSwapChainFormat(surface_formats);
		VkExtent2D                    desired_extent = GetSwapChainExtent(surface_capabilities);
		VkImageUsageFlags             desired_usage = GetSwapChainUsageFlags(surface_capabilities);
		VkSurfaceTransformFlagBitsKHR desired_transform = GetSwapChainTransform(surface_capabilities);
		VkPresentModeKHR              desired_present_mode = GetSwapChainPresentMode(present_modes);
		VkSwapchainKHR                old_swap_chain = m_SwapChain.handle;

		if (static_cast<int>(desired_usage) == -1) {
			return false;
		}
		if (static_cast<int>(desired_present_mode) == -1) {
			return false;
		}
		if ((desired_extent.width == 0) || (desired_extent.height == 0)) {
			return true;
		}

		VkSwapchainCreateInfoKHR swapchain_create_info {};
		swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchain_create_info.surface = m_PresentationSurface;
		swapchain_create_info.minImageCount = desired_number_of_images;
		swapchain_create_info.imageExtent = desired_extent;
		swapchain_create_info.imageFormat = desired_format.format;
		swapchain_create_info.presentMode = desired_present_mode;
		swapchain_create_info.preTransform = desired_transform;
		swapchain_create_info.clipped = VK_TRUE;
		swapchain_create_info.imageUsage = desired_usage;
		swapchain_create_info.imageColorSpace = desired_format.colorSpace;
		swapchain_create_info.imageArrayLayers = 1;
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchain_create_info.oldSwapchain = old_swap_chain;

		if (vkCreateSwapchainKHR(m_Device, &swapchain_create_info, nullptr, &m_SwapChain.handle) != VK_SUCCESS) {
			std::cout << "Could not create swap chain!\n";
			return false;
		}
		if (old_swap_chain != VK_NULL_HANDLE) {
			vkDestroySwapchainKHR(m_Device, old_swap_chain, nullptr);
		}
		m_SwapChain.format = desired_format.format;
		uint32_t image_count = 0;
		if ((vkGetSwapchainImagesKHR(m_Device, m_SwapChain.handle, &image_count, nullptr) != VK_SUCCESS) ||
			(image_count == 0)) {
			std::cout << "Could not get swap chain images!\n";
			return false;
		}
		m_SwapChain.images.resize(image_count);

		std::vector<VkImage> images(image_count);
		if (vkGetSwapchainImagesKHR(m_Device, m_SwapChain.handle, &image_count, &images[0]) != VK_SUCCESS) {
			std::cout << "Could not get swap chain images!\n";
			return false;
		}
		for (size_t i = 0; i < m_SwapChain.images.size(); ++i) {
			m_SwapChain.images[i].handle = images[i];
		}
		m_SwapChain.extent = desired_extent;

		return CreateSwapChainImageViews();
	}

	bool RenderParameters::CreateSwapChainImageViews()
	{
		for (size_t i = 0; i < m_SwapChain.images.size(); ++i) {
			VkImageViewCreateInfo image_view_create_info{};
			image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			image_view_create_info.image = m_SwapChain.images[i].handle;
			image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			image_view_create_info.format = m_SwapChain.format;
			image_view_create_info.components = {
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY,
				VK_COMPONENT_SWIZZLE_IDENTITY
			};
			image_view_create_info.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			if (vkCreateImageView(m_Device, &image_view_create_info, nullptr, &m_SwapChain.images[i].view) != VK_SUCCESS) {
				std::cout << "Could not create image view for framebuffer!\n";
				return false;
			}
		}
		m_CanRender = true;
		return true;
	}

	bool RenderParameters::CreateFramebuffer(VkFramebuffer &framebuffer, VkImageView image_view) const
	{
		if (framebuffer != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(m_Device, framebuffer, nullptr);
			framebuffer = VK_NULL_HANDLE;
		}

		VkFramebufferCreateInfo framebuffer_create_info{};
		framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_create_info.renderPass = m_RenderPass;
		framebuffer_create_info.attachmentCount = 1;
		framebuffer_create_info.pAttachments = &image_view;
		framebuffer_create_info.width = m_SwapChain.extent.width;
		framebuffer_create_info.height = m_SwapChain.extent.height;
		framebuffer_create_info.layers = 1;

		if (vkCreateFramebuffer(m_Device, &framebuffer_create_info, nullptr, &framebuffer) != VK_SUCCESS) {
			std::cout << "Could not create a framebuffer!\n";
			return false;
		}
		return true;
	}

	AutoDeleter<VkShaderModule, PFN_vkDestroyShaderModule> RenderParameters::CreateShaderModule(const char* filename) const
	{
		const std::vector<char> code = GetBinaryFileContents(filename);
		if (code.size() == 0) {
			return AutoDeleter<VkShaderModule, PFN_vkDestroyShaderModule>();
		}
		VkShaderModuleCreateInfo shader_module_create_info{};
		shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shader_module_create_info.codeSize = code.size();
		shader_module_create_info.pCode = reinterpret_cast<const uint32_t*>(&code[0]);

		VkShaderModule shader_module;
		if (vkCreateShaderModule(m_Device, &shader_module_create_info, nullptr, &shader_module) != VK_SUCCESS) {
			std::cout << "Could not create shader module from a \"" << filename << "\" file!\n";
			return AutoDeleter<VkShaderModule, PFN_vkDestroyShaderModule>();
		}
		return AutoDeleter<VkShaderModule, PFN_vkDestroyShaderModule>(shader_module, vkDestroyShaderModule, m_Device);
	}

	bool RenderParameters::CreatePipeline() {
		AutoDeleter<VkShaderModule, PFN_vkDestroyShaderModule> vertex_shader_module = CreateShaderModule("shaders/vert.spv");
		AutoDeleter<VkShaderModule, PFN_vkDestroyShaderModule> fragment_shader_module = CreateShaderModule("shaders/frag.spv");

		if (!vertex_shader_module || !fragment_shader_module) {
			return false;
		}

		std::vector<VkPipelineShaderStageCreateInfo> shader_stage_create_infos = {
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
				nullptr,                                                    // const void                                    *pNext
				0,                                                          // VkPipelineShaderStageCreateFlags               flags
				VK_SHADER_STAGE_VERTEX_BIT,                                 // VkShaderStageFlagBits                          stage
				vertex_shader_module.Get(),                                 // VkShaderModule                                 module
				"main",                                                     // const char                                    *pName
				nullptr                                                     // const VkSpecializationInfo                    *pSpecializationInfo
			},
			{
				VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,        // VkStructureType                                sType
				nullptr,                                                    // const void                                    *pNext
				0,                                                          // VkPipelineShaderStageCreateFlags               flags
				VK_SHADER_STAGE_FRAGMENT_BIT,                               // VkShaderStageFlagBits                          stage
				fragment_shader_module.Get(),                               // VkShaderModule                                 module
				"main",                                                     // const char                                    *pName
				nullptr                                                     // const VkSpecializationInfo                    *pSpecializationInfo
			}
		};

		std::vector<VkVertexInputBindingDescription> vertex_binding_descriptions = {
			{
				0,                                                          // uint32_t                                       binding
				sizeof(VertexData),                                         // uint32_t                                       stride
				VK_VERTEX_INPUT_RATE_VERTEX                                 // VkVertexInputRate                              inputRate
			}
		};

		std::vector<VkVertexInputAttributeDescription> vertex_attribute_descriptions = {
			{
				0,                                                          // uint32_t                                       location
				vertex_binding_descriptions[0].binding,                     // uint32_t                                       binding
				VK_FORMAT_R32G32B32A32_SFLOAT,                              // VkFormat                                       format
				offsetof(struct VertexData, x)                              // uint32_t                                       offset
			},
			{
				1,                                                          // uint32_t                                       location
				vertex_binding_descriptions[0].binding,                     // uint32_t                                       binding
				VK_FORMAT_R32G32B32A32_SFLOAT,                              // VkFormat                                       format
				offsetof(struct VertexData, r)                              // uint32_t                                       offset
			}
		};

		VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,    // VkStructureType                                sType
			nullptr,                                                      // const void                                    *pNext
			0,                                                            // VkPipelineVertexInputStateCreateFlags          flags
			static_cast<uint32_t>(vertex_binding_descriptions.size()),    // uint32_t                                       vertexBindingDescriptionCount
			&vertex_binding_descriptions[0],                              // const VkVertexInputBindingDescription         *pVertexBindingDescriptions
			static_cast<uint32_t>(vertex_attribute_descriptions.size()),  // uint32_t                                       vertexAttributeDescriptionCount
			&vertex_attribute_descriptions[0]                             // const VkVertexInputAttributeDescription       *pVertexAttributeDescriptions
		};

		VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,  // VkStructureType                                sType
			nullptr,                                                      // const void                                    *pNext
			0,                                                            // VkPipelineInputAssemblyStateCreateFlags        flags
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,                         // VkPrimitiveTopology                            topology
			VK_FALSE                                                      // VkBool32                                       primitiveRestartEnable
		};

		VkPipelineViewportStateCreateInfo viewport_state_create_info = {
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,        // VkStructureType                                sType
			nullptr,                                                      // const void                                    *pNext
			0,                                                            // VkPipelineViewportStateCreateFlags             flags
			1,                                                            // uint32_t                                       viewportCount
			nullptr,                                                      // const VkViewport                              *pViewports
			1,                                                            // uint32_t                                       scissorCount
			nullptr                                                       // const VkRect2D                                *pScissors
		};

		VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,   // VkStructureType                                sType
			nullptr,                                                      // const void                                    *pNext
			0,                                                            // VkPipelineRasterizationStateCreateFlags        flags
			VK_FALSE,                                                     // VkBool32                                       depthClampEnable
			VK_FALSE,                                                     // VkBool32                                       rasterizerDiscardEnable
			VK_POLYGON_MODE_FILL,                                         // VkPolygonMode                                  polygonMode
			VK_CULL_MODE_BACK_BIT,                                        // VkCullModeFlags                                cullMode
			VK_FRONT_FACE_COUNTER_CLOCKWISE,                              // VkFrontFace                                    frontFace
			VK_FALSE,                                                     // VkBool32                                       depthBiasEnable
			0.0f,                                                         // float                                          depthBiasConstantFactor
			0.0f,                                                         // float                                          depthBiasClamp
			0.0f,                                                         // float                                          depthBiasSlopeFactor
			1.0f                                                          // float                                          lineWidth
		};

		VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,     // VkStructureType                                sType
			nullptr,                                                      // const void                                    *pNext
			0,                                                            // VkPipelineMultisampleStateCreateFlags          flags
			VK_SAMPLE_COUNT_1_BIT,                                        // VkSampleCountFlagBits                          rasterizationSamples
			VK_FALSE,                                                     // VkBool32                                       sampleShadingEnable
			1.0f,                                                         // float                                          minSampleShading
			nullptr,                                                      // const VkSampleMask                            *pSampleMask
			VK_FALSE,                                                     // VkBool32                                       alphaToCoverageEnable
			VK_FALSE                                                      // VkBool32                                       alphaToOneEnable
		};

		std::vector<VkDynamicState> dynamic_states = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
			VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,         // VkStructureType                                sType
			nullptr,                                                      // const void                                    *pNext
			0,                                                            // VkPipelineDynamicStateCreateFlags              flags
			static_cast<uint32_t>(dynamic_states.size()),                 // uint32_t                                       dynamicStateCount
			&dynamic_states[0]                                            // const VkDynamicState                          *pDynamicStates
		};

		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {
			VK_FALSE,                                                     // VkBool32                                       blendEnable
			VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcColorBlendFactor
			VK_BLEND_FACTOR_ZERO,                                         // VkBlendFactor                                  dstColorBlendFactor
			VK_BLEND_OP_ADD,                                              // VkBlendOp                                      colorBlendOp
			VK_BLEND_FACTOR_ONE,                                          // VkBlendFactor                                  srcAlphaBlendFactor
			VK_BLEND_FACTOR_ZERO,                                         // VkBlendFactor                                  dstAlphaBlendFactor
			VK_BLEND_OP_ADD,                                              // VkBlendOp                                      alphaBlendOp
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |         // VkColorComponentFlags                          colorWriteMask
			VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,     // VkStructureType                                sType
			nullptr,                                                      // const void                                    *pNext
			0,                                                            // VkPipelineColorBlendStateCreateFlags           flags
			VK_FALSE,                                                     // VkBool32                                       logicOpEnable
			VK_LOGIC_OP_COPY,                                             // VkLogicOp                                      logicOp
			1,                                                            // uint32_t                                       attachmentCount
			&color_blend_attachment_state,                                // const VkPipelineColorBlendAttachmentState     *pAttachments
			{ 0.0f, 0.0f, 0.0f, 0.0f }                                    // float                                          blendConstants[4]
		};

		AutoDeleter<VkPipelineLayout, PFN_vkDestroyPipelineLayout> pipeline_layout = CreatePipelineLayout();
		if (!pipeline_layout) {
			return false;
		}

		VkGraphicsPipelineCreateInfo pipeline_create_info = {
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,              // VkStructureType                                sType
			nullptr,                                                      // const void                                    *pNext
			0,                                                            // VkPipelineCreateFlags                          flags
			static_cast<uint32_t>(shader_stage_create_infos.size()),      // uint32_t                                       stageCount
			&shader_stage_create_infos[0],                                // const VkPipelineShaderStageCreateInfo         *pStages
			&vertex_input_state_create_info,                              // const VkPipelineVertexInputStateCreateInfo    *pVertexInputState;
			&input_assembly_state_create_info,                            // const VkPipelineInputAssemblyStateCreateInfo  *pInputAssemblyState
			nullptr,                                                      // const VkPipelineTessellationStateCreateInfo   *pTessellationState
			&viewport_state_create_info,                                  // const VkPipelineViewportStateCreateInfo       *pViewportState
			&rasterization_state_create_info,                             // const VkPipelineRasterizationStateCreateInfo  *pRasterizationState
			&multisample_state_create_info,                               // const VkPipelineMultisampleStateCreateInfo    *pMultisampleState
			nullptr,                                                      // const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState
			&color_blend_state_create_info,                               // const VkPipelineColorBlendStateCreateInfo     *pColorBlendState
			&dynamic_state_create_info,                                                      // const VkPipelineDynamicStateCreateInfo        *pDynamicState
			pipeline_layout.Get(),                                        // VkPipelineLayout                               layout
			m_RenderPass,                                            // VkRenderPass                                   renderPass
			0,                                                            // uint32_t                                       subpass
			VK_NULL_HANDLE,                                               // VkPipeline                                     basePipelineHandle
			-1                                                            // int32_t                                        basePipelineIndex
		};

		if (vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &m_GraphicsPipeline) != VK_SUCCESS) {
			std::cout << "Could not create graphics pipeline!\n";
			return false;
		}
		return true;
	}

	bool RenderParameters::CreateVertexBuffer(const std::vector<float>& vertex_data)
	{
		m_VertexBuffer.size = static_cast<uint32_t>(vertex_data.size() * sizeof(vertex_data[0]));

		if (!CreateBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer)) {
			std::cout << "Could not create vertex buffer!" << std::endl;
			return false;
		}
		return true;
	}

	bool RenderParameters::CopyVertexData(const std::vector<float>& vertex_data) {
		void *staging_buffer_memory_pointer;
		if (vkMapMemory(m_Device, m_StagingBuffer.memory, 0, m_VertexBuffer.size, 0, &staging_buffer_memory_pointer) != VK_SUCCESS) {
			std::cout << "Could not map memory and upload data to a staging buffer!" << std::endl;
			return false;
		}

		memcpy(staging_buffer_memory_pointer, &vertex_data[0], m_VertexBuffer.size);

		VkMappedMemoryRange flush_range = {
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,            
			nullptr,                                         
			m_StagingBuffer.memory,                        
			0,                                                 
			m_VertexBuffer.size                          
		};
		vkFlushMappedMemoryRanges(m_Device, 1, &flush_range);

		vkUnmapMemory(m_Device, m_StagingBuffer.memory);

		VkCommandBufferBeginInfo command_buffer_begin_info = {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,        // VkStructureType                        sType
			nullptr,                                            // const void                            *pNext
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,        // VkCommandBufferUsageFlags              flags
			nullptr                                             // const VkCommandBufferInheritanceInfo  *pInheritanceInfo
		};

		VkCommandBuffer command_buffer = m_RenderingResources[0].commandBuffer;

		vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

		VkBufferCopy buffer_copy_info = {
			0,                                                  // VkDeviceSize                           srcOffset
			0,                                                  // VkDeviceSize                           dstOffset
			m_VertexBuffer.size                            // VkDeviceSize                           size
		};
		vkCmdCopyBuffer(command_buffer, m_StagingBuffer.handle, m_VertexBuffer.handle, 1, &buffer_copy_info);

		VkBufferMemoryBarrier buffer_memory_barrier = {
			VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,            // VkStructureType                        sType;
			nullptr,                                            // const void                            *pNext
			VK_ACCESS_MEMORY_WRITE_BIT,                         // VkAccessFlags                          srcAccessMask
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,                // VkAccessFlags                          dstAccessMask
			VK_QUEUE_FAMILY_IGNORED,                            // uint32_t                               srcQueueFamilyIndex
			VK_QUEUE_FAMILY_IGNORED,                            // uint32_t                               dstQueueFamilyIndex
			m_VertexBuffer.handle,                              // VkBuffer                               buffer
			0,                                                  // VkDeviceSize                           offset
			VK_WHOLE_SIZE                                       // VkDeviceSize                           size
		};
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, 1, &buffer_memory_barrier, 0, nullptr);

		vkEndCommandBuffer(command_buffer);

		// Submit command buffer and copy data from staging buffer to a vertex buffer
		VkSubmitInfo submit_info = {
			VK_STRUCTURE_TYPE_SUBMIT_INFO,                      // VkStructureType                        sType
			nullptr,                                            // const void                            *pNext
			0,                                                  // uint32_t                               waitSemaphoreCount
			nullptr,                                            // const VkSemaphore                     *pWaitSemaphores
			nullptr,                                            // const VkPipelineStageFlags            *pWaitDstStageMask;
			1,                                                  // uint32_t                               commandBufferCount
			&command_buffer,                                    // const VkCommandBuffer                 *pCommandBuffers
			0,                                                  // uint32_t                               signalSemaphoreCount
			nullptr                                             // const VkSemaphore                     *pSignalSemaphores
		};

		if (vkQueueSubmit(m_GraphicsQueue.handle, 1, &submit_info, VK_NULL_HANDLE) != VK_SUCCESS) {
			return false;
		}

		vkDeviceWaitIdle(m_Device);

		return true;
	}

	bool RenderParameters::CreateFences()
	{
		VkFenceCreateInfo fence_create_info{};
		fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < m_RenderingResources.size(); ++i) {
			if (vkCreateFence(m_Device, &fence_create_info, nullptr, &m_RenderingResources[i].fence) != VK_SUCCESS) {
				std::cout << "Could not create a fence!" << std::endl;
				return false;
			}
		}
		return true;
	}

	bool RenderParameters::CreateStagingBuffer()
	{
		m_StagingBuffer.size = 4000;
		if (!CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, m_StagingBuffer)) {
			std::cout << "Could not staging buffer!" << std::endl;
			return false;
		}

		return true;
	}

	bool RenderParameters::CreateBuffer(VkBufferUsageFlags usage, VkMemoryPropertyFlagBits memoryProperty, BufferParameters &buffer) {
		VkBufferCreateInfo buffer_create_info = {
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,             // VkStructureType        sType
			nullptr,                                          // const void            *pNext
			0,                                                // VkBufferCreateFlags    flags
			buffer.size,                                      // VkDeviceSize           size
			usage,                                            // VkBufferUsageFlags     usage
			VK_SHARING_MODE_EXCLUSIVE,                        // VkSharingMode          sharingMode
			0,                                                // uint32_t               queueFamilyIndexCount
			nullptr                                           // const uint32_t        *pQueueFamilyIndices
		};

		if (vkCreateBuffer(m_Device, &buffer_create_info, nullptr, &buffer.handle) != VK_SUCCESS) {
			std::cout << "Could not create buffer!" << std::endl;
			return false;
		}

		if (!AllocateBufferMemory(buffer.handle, memoryProperty, &buffer.memory)) {
			std::cout << "Could not allocate memory for a buffer!" << std::endl;
			return false;
		}

		if (vkBindBufferMemory(m_Device, buffer.handle, buffer.memory, 0) != VK_SUCCESS) {
			std::cout << "Could not bind memory to a buffer!" << std::endl;
			return false;
		}

		return true;
	}

	bool RenderParameters::PrepareFrame(VkCommandBuffer command_buffer, const ImageParameters &image_parameters, VkFramebuffer &framebuffer) const
	{
		if (!CreateFramebuffer(framebuffer, image_parameters.view)) {
			return false;
		}

		VkCommandBufferBeginInfo command_buffer_begin_info {};
		command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;			
		vkBeginCommandBuffer(command_buffer, &command_buffer_begin_info);

		VkImageSubresourceRange image_subresource_range {};
		image_subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		image_subresource_range.levelCount = 1;
		image_subresource_range.layerCount = 1;

		if (m_PresentQueue.handle != m_GraphicsQueue.handle) {
			VkImageMemoryBarrier barrier_from_present_to_draw{};
			barrier_from_present_to_draw.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier_from_present_to_draw.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier_from_present_to_draw.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			barrier_from_present_to_draw.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier_from_present_to_draw.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier_from_present_to_draw.srcQueueFamilyIndex = m_GraphicsQueue.familyIndex;
			barrier_from_present_to_draw.dstQueueFamilyIndex = m_PresentQueue.familyIndex;
			barrier_from_present_to_draw.image = image_parameters.handle;
			barrier_from_present_to_draw.subresourceRange = image_subresource_range;		
			vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_present_to_draw);
		}
		
		VkClearValue clear_value = {
			m_ClearColor.r, m_ClearColor.g, m_ClearColor.b                       
		};

		VkRenderPassBeginInfo render_pass_begin_info{};
		render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_begin_info.renderPass = m_RenderPass;
		render_pass_begin_info.framebuffer = framebuffer;
		render_pass_begin_info.renderArea = { { 0, 0 }, m_SwapChain.extent, };
		render_pass_begin_info.clearValueCount = 1;
		render_pass_begin_info.pClearValues = &clear_value;

		vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

		VkViewport viewport{};
		viewport.width = static_cast<float>(m_SwapChain.extent.width);
		viewport.height = static_cast<float>(m_SwapChain.extent.height);
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.extent = {
			m_SwapChain.extent.width,
			m_SwapChain.extent.height
		};

		vkCmdSetViewport(command_buffer, 0, 1, &viewport);
		vkCmdSetScissor(command_buffer, 0, 1, &scissor);

		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(command_buffer, 0, 1, &m_VertexBuffer.handle, &offset);

		vkCmdDraw(command_buffer, 4, 1, 0, 0);

		vkCmdEndRenderPass(command_buffer);

		if (m_GraphicsQueue.handle != m_PresentQueue.handle) {
			VkImageMemoryBarrier barrier_from_draw_to_present{};
			barrier_from_draw_to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER; 
			barrier_from_draw_to_present.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;                      
			barrier_from_draw_to_present.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;                      
			barrier_from_draw_to_present.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;                 
			barrier_from_draw_to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;                
			barrier_from_draw_to_present.srcQueueFamilyIndex = m_GraphicsQueue.familyIndex;                 
			barrier_from_draw_to_present.dstQueueFamilyIndex = m_PresentQueue.familyIndex;                  
			barrier_from_draw_to_present.image = image_parameters.handle;                              
			barrier_from_draw_to_present.subresourceRange = image_subresource_range;   
			vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier_from_draw_to_present);
		}

		if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
			std::cout << "Could not record command buffer!" << std::endl;
			return false;
		}
		return true;
	}

	bool RenderParameters::AllocateBufferMemory(VkBuffer buffer, VkMemoryPropertyFlagBits property, VkDeviceMemory *memory) const
	{
		VkMemoryRequirements buffer_memory_requirements;
		vkGetBufferMemoryRequirements(m_Device, buffer, &buffer_memory_requirements);

		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memory_properties);

		for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
			if ((buffer_memory_requirements.memoryTypeBits & (1 << i)) &&
				(memory_properties.memoryTypes[i].propertyFlags & property)) {

				VkMemoryAllocateInfo memory_allocate_info {};
				memory_allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				memory_allocate_info.allocationSize = buffer_memory_requirements.size;
				memory_allocate_info.memoryTypeIndex = i;
				if (vkAllocateMemory(m_Device, &memory_allocate_info, nullptr, memory) == VK_SUCCESS) {
					return true;
				}
			}
		}
		return false;
	}

	bool RenderParameters::CreateCommandPool(uint32_t queue_family_index, VkCommandPool *pool) const
	{
		VkCommandPoolCreateInfo cmd_pool_create_info{};
		cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		cmd_pool_create_info.queueFamilyIndex = queue_family_index;

		if (vkCreateCommandPool(m_Device, &cmd_pool_create_info, nullptr, pool) != VK_SUCCESS) {
			std::cout << "Could not create a command pool!\n";
			return false;
		}
		return true;
	}

	AutoDeleter<VkPipelineLayout, PFN_vkDestroyPipelineLayout> RenderParameters::CreatePipelineLayout() const
	{
		VkPipelineLayoutCreateInfo layout_create_info {};
		layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		VkPipelineLayout pipeline_layout;
		if (vkCreatePipelineLayout(m_Device, &layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS) {
			std::cout << "Could not create pipeline layout!\n";
			return AutoDeleter<VkPipelineLayout, PFN_vkDestroyPipelineLayout>();
		}
		return AutoDeleter<VkPipelineLayout, PFN_vkDestroyPipelineLayout>(pipeline_layout, vkDestroyPipelineLayout, m_Device);
	}

	uint32_t RenderParameters::GetSwapChainNumImages(VkSurfaceCapabilitiesKHR &surface_capabilities) {
		uint32_t image_count = surface_capabilities.minImageCount + 1;
		if ((surface_capabilities.maxImageCount > 0) &&
			(image_count > surface_capabilities.maxImageCount)) {
			image_count = surface_capabilities.maxImageCount;
		}
		return image_count;
	}

	VkSurfaceFormatKHR RenderParameters::GetSwapChainFormat(std::vector<VkSurfaceFormatKHR> &surface_formats) {
		if ((surface_formats.size() == 1) &&
			(surface_formats[0].format == VK_FORMAT_UNDEFINED)) {
			return{ VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
		}
		for (VkSurfaceFormatKHR &surface_format : surface_formats) {
			if (surface_format.format == VK_FORMAT_R8G8B8A8_UNORM) {
				return surface_format;
			}
		}
		return surface_formats[0];
	}

	VkExtent2D RenderParameters::GetSwapChainExtent(VkSurfaceCapabilitiesKHR &surface_capabilities) {
		if (surface_capabilities.currentExtent.width == -1) {
			VkExtent2D swap_chain_extent = { 640, 480 };
			if (swap_chain_extent.width < surface_capabilities.minImageExtent.width) {
				swap_chain_extent.width = surface_capabilities.minImageExtent.width;
			}
			if (swap_chain_extent.height < surface_capabilities.minImageExtent.height) {
				swap_chain_extent.height = surface_capabilities.minImageExtent.height;
			}
			if (swap_chain_extent.width > surface_capabilities.maxImageExtent.width) {
				swap_chain_extent.width = surface_capabilities.maxImageExtent.width;
			}
			if (swap_chain_extent.height > surface_capabilities.maxImageExtent.height) {
				swap_chain_extent.height = surface_capabilities.maxImageExtent.height;
			}
			return swap_chain_extent;
		}
		return surface_capabilities.currentExtent;
	}

	VkImageUsageFlags RenderParameters::GetSwapChainUsageFlags(VkSurfaceCapabilitiesKHR &surface_capabilities) {
		if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
			return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}
		std::cout << "VK_IMAGE_USAGE_TRANSFER_DST image usage is not supported by the swap chain!" << std::endl
			<< "Supported swap chain's image usages include:" << std::endl
			<< (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT ? "    VK_IMAGE_USAGE_TRANSFER_SRC\n" : "")
			<< (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT ? "    VK_IMAGE_USAGE_TRANSFER_DST\n" : "")
			<< (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_SAMPLED_BIT ? "    VK_IMAGE_USAGE_SAMPLED\n" : "")
			<< (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT ? "    VK_IMAGE_USAGE_STORAGE\n" : "")
			<< (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_COLOR_ATTACHMENT\n" : "")
			<< (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT\n" : "")
			<< (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT\n" : "")
			<< (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT ? "    VK_IMAGE_USAGE_INPUT_ATTACHMENT" : "")
			<< std::endl;
		return static_cast<VkImageUsageFlags>(-1);
	}

	VkSurfaceTransformFlagBitsKHR RenderParameters::GetSwapChainTransform(VkSurfaceCapabilitiesKHR &surface_capabilities)
	{
		if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
			return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		return surface_capabilities.currentTransform;
	}

	VkPresentModeKHR RenderParameters::GetSwapChainPresentMode(std::vector<VkPresentModeKHR> &present_modes)
	{
		for (auto& present_mode : present_modes) {
			if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return present_mode;
			}
		}
		for (auto& present_mode : present_modes) {
			if (present_mode == VK_PRESENT_MODE_FIFO_KHR) {
				return present_mode;
			}
		}
		std::cout << "FIFO present mode is not supported by the swap chain!\n";
		return static_cast<VkPresentModeKHR>(-1);
	}

	bool RenderParameters::LoadGlobalLevelEntryPoints() const
	{
#define VK_GLOBAL_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( nullptr, #fun )) ) {                    \
      std::cout << "Could not load global level function: " << #fun << "!" << std::endl;  \
      return false;                                                                       \
    }

#include "vk_functions.inl"

		return true;
	}

	bool RenderParameters::LoadInstanceLevelEntryPoints() const
	{
#define VK_INSTANCE_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetInstanceProcAddr( m_Instance, #fun )) ) {              \
      std::cout << "Could not load instance level function: " << #fun << "!" << std::endl;  \
      return false;                                                                         \
    }

#include "vk_functions.inl"

		return true;
	}

	bool RenderParameters::LoadDeviceLevelEntryPoints() const
	{
#define VK_DEVICE_LEVEL_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)vkGetDeviceProcAddr( m_Device, #fun )) ) {                \
      std::cout << "Could not load device level function: " << #fun << "!" << std::endl;  \
      return false;                                                                       \
    }

#include "vk_functions.inl"

		return true;
	}

	void RenderParameters::DestroyBuffer(BufferParameters& buffer) const
	{
		if (buffer.handle != VK_NULL_HANDLE) {
			vkDestroyBuffer(m_Device, buffer.handle, nullptr);
			buffer.handle = VK_NULL_HANDLE;
		}

		if (buffer.memory != VK_NULL_HANDLE) {
			vkFreeMemory(m_Device, buffer.memory, nullptr);
			buffer.memory = VK_NULL_HANDLE;
		}
	}

	bool RenderParameters::AllocateCommandBuffers(VkCommandPool pool, uint32_t count, VkCommandBuffer *command_buffers) const
	{
		VkCommandBufferAllocateInfo command_buffer_allocate_info{};
		command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		command_buffer_allocate_info.commandPool = pool;
		command_buffer_allocate_info.commandBufferCount = count;
		if (vkAllocateCommandBuffers(m_Device, &command_buffer_allocate_info, command_buffers) != VK_SUCCESS) {
			return false;
		}
		return true;
	}

	bool RenderParameters::CreateCommandBuffers() {
		if (!CreateCommandPool(m_GraphicsQueue.familyIndex, &m_CommandPool)) {
			std::cout << "Could not create command pool!" << std::endl;
			return false;
		}

		for (size_t i = 0; i < m_RenderingResources.size(); ++i) {
			if (!AllocateCommandBuffers(m_CommandPool, 1, &m_RenderingResources[i].commandBuffer)) {
				std::cout << "Could not allocate command buffer!" << std::endl;
				return false;
			}
		}
		return true;
	}

	bool RenderParameters::CreateRenderPass()
	{
		VkAttachmentDescription attachment_descriptions[] = {
			{
				0,                                          // VkAttachmentDescriptionFlags   flags
				m_SwapChain.format,                         // VkFormat                       format
				VK_SAMPLE_COUNT_1_BIT,                      // VkSampleCountFlagBits          samples
				VK_ATTACHMENT_LOAD_OP_CLEAR,                // VkAttachmentLoadOp             loadOp
				VK_ATTACHMENT_STORE_OP_STORE,               // VkAttachmentStoreOp            storeOp
				VK_ATTACHMENT_LOAD_OP_DONT_CARE,            // VkAttachmentLoadOp             stencilLoadOp
				VK_ATTACHMENT_STORE_OP_DONT_CARE,           // VkAttachmentStoreOp            stencilStoreOp
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,            // VkImageLayout                  initialLayout;
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR             // VkImageLayout                  finalLayout
			}
		};
		VkAttachmentReference color_attachment_references[] = {
			{
				0,                                          // uint32_t                       attachment
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL    // VkImageLayout                  layout
			}
		};

		VkSubpassDescription subpass_descriptions[] = {
			{
				0,                                          // VkSubpassDescriptionFlags      flags
				VK_PIPELINE_BIND_POINT_GRAPHICS,            // VkPipelineBindPoint            pipelineBindPoint
				0,                                          // uint32_t                       inputAttachmentCount
				nullptr,                                    // const VkAttachmentReference   *pInputAttachments
				1,                                          // uint32_t                       colorAttachmentCount
				color_attachment_references,                // const VkAttachmentReference   *pColorAttachments
				nullptr,                                    // const VkAttachmentReference   *pResolveAttachments
				nullptr,                                    // const VkAttachmentReference   *pDepthStencilAttachment
				0,                                          // uint32_t                       preserveAttachmentCount
				nullptr                                     // const uint32_t*                pPreserveAttachments
			}
		};

		std::vector<VkSubpassDependency> dependencies = {
			{
				VK_SUBPASS_EXTERNAL,                            // uint32_t                       srcSubpass
				0,                                              // uint32_t                       dstSubpass
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // VkPipelineStageFlags           srcStageMask
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // VkPipelineStageFlags           dstStageMask
				VK_ACCESS_MEMORY_READ_BIT,                      // VkAccessFlags                  srcAccessMask
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // VkAccessFlags                  dstAccessMask
				VK_DEPENDENCY_BY_REGION_BIT                     // VkDependencyFlags              dependencyFlags
			},
			{
				0,                                              // uint32_t                       srcSubpass
				VK_SUBPASS_EXTERNAL,                            // uint32_t                       dstSubpass
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // VkPipelineStageFlags           srcStageMask
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,           // VkPipelineStageFlags           dstStageMask
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // VkAccessFlags                  srcAccessMask
				VK_ACCESS_MEMORY_READ_BIT,                      // VkAccessFlags                  dstAccessMask
				VK_DEPENDENCY_BY_REGION_BIT                     // VkDependencyFlags              dependencyFlags
			}
		};
		VkRenderPassCreateInfo render_pass_create_info = {};
		render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_create_info.attachmentCount = 1;
		render_pass_create_info.pAttachments = attachment_descriptions;
		render_pass_create_info.subpassCount = 1;
		render_pass_create_info.pSubpasses = subpass_descriptions;
		render_pass_create_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
		render_pass_create_info.pDependencies = &dependencies[0];		

		if (vkCreateRenderPass(m_Device, &render_pass_create_info, nullptr, &m_RenderPass) != VK_SUCCESS) {
			std::cout << "Could not create render pass!\n";
			return false;
		}

		return true;
	}

	RenderParameters::~RenderParameters() 
	{
		if (m_Device != nullptr) {
			vkDeviceWaitIdle(m_Device);

			for (size_t i = 0; i < m_RenderingResources.size(); ++i) {
				if (m_RenderingResources[i].framebuffer != VK_NULL_HANDLE) {
					vkDestroyFramebuffer(m_Device, m_RenderingResources[i].framebuffer, nullptr);
				}
				if (m_RenderingResources[i].commandBuffer != nullptr) {
					vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &m_RenderingResources[i].commandBuffer);
				}
				if (m_RenderingResources[i].imageAvailableSemaphore != VK_NULL_HANDLE) {
					vkDestroySemaphore(m_Device, m_RenderingResources[i].imageAvailableSemaphore, nullptr);
				}
				if (m_RenderingResources[i].finishedRenderingSemaphore != VK_NULL_HANDLE) {
					vkDestroySemaphore(m_Device, m_RenderingResources[i].finishedRenderingSemaphore, nullptr);
				}
				if (m_RenderingResources[i].fence != VK_NULL_HANDLE) {
					vkDestroyFence(m_Device, m_RenderingResources[i].fence, nullptr);
				}
			}
			if (m_CommandPool != VK_NULL_HANDLE) {
				vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
				m_CommandPool = VK_NULL_HANDLE;
			}
			DestroyBuffer(m_VertexBuffer);
			DestroyBuffer(m_StagingBuffer);
			if (m_GraphicsPipeline != VK_NULL_HANDLE) {
				vkDestroyPipeline(m_Device, m_GraphicsPipeline, nullptr);
				m_GraphicsPipeline = VK_NULL_HANDLE;
			}
			if (m_RenderPass != VK_NULL_HANDLE) {
				vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
				m_RenderPass = VK_NULL_HANDLE;
			}
			if (m_SwapChain.handle != VK_NULL_HANDLE) {
				vkDestroySwapchainKHR(m_Device, m_SwapChain.handle, nullptr);
			}
			vkDestroyDevice(m_Device, nullptr);
		}
		if (m_PresentationSurface != VK_NULL_HANDLE) {
			vkDestroySurfaceKHR(m_Instance, m_PresentationSurface, nullptr);
		}
		if (m_Instance != nullptr) {
			vkDestroyInstance(m_Instance, nullptr);
		}
		if(m_Window) {
			delete m_Window;
		}	
		if(m_VulkanLibrary ) {
		#if defined(USE_PLATFORM_WIN32_KHR)
			FreeLibrary(m_VulkanLibrary);
		#elif defined(USE_PLATFORM_XCB_KHR)
			dlclose(m_VulkanLibrary);
		#endif
		  }
	}	

	bool RenderParameters::CreatePresentationSurface() {
	#if defined(USE_PLATFORM_WIN32_KHR)     	
     	struct VkWin32SurfaceCreateInfoKHR surface_create_info {};
		surface_create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surface_create_info.hinstance = m_Window->instance;
		surface_create_info.hwnd = m_Window->handle; 
   		if (vkCreateWin32SurfaceKHR(m_Instance, &surface_create_info, nullptr, &m_PresentationSurface) == VK_SUCCESS) {
			return true;
		}
	#elif defined(USE_PLATFORM_XCB_KHR)
		VkXcbSurfaceCreateInfoKHR surface_create_info {};
		surface_create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		surface_create_info.connection = m_Window->connection;
		surface_create_info.window = m_Window->handle;		
		if( vkCreateXcbSurfaceKHR(m_Instance, &surface_create_info, nullptr, &m_PresentationSurface ) == VK_SUCCESS ) {
			return true;
		}
		std::cout << "Could not create presentation surface!" << std::endl;
		return false;	
	#endif
	}

	bool RenderParameters::LoadExportedEntryPoints() const
	{
#if defined(USE_PLATFORM_WIN32_KHR)
    #define LoadProcAddress GetProcAddress
#elif defined(USE_PLATFORM_XCB_KHR)
    #define LoadProcAddress dlsym
#endif

#define VK_EXPORTED_FUNCTION( fun )                                                   \
    if( !(fun = (PFN_##fun)LoadProcAddress( m_VulkanLibrary, #fun )) ) {                \
      std::cout << "Could not load exported function: " << #fun << "!" << std::endl;  \
      return false;                                                                   \
    }

#include "vk_functions.inl"

		return true;
	}

	bool RenderParameters::LoadVulkanLibrary() 
	{
	#if defined(USE_PLATFORM_WIN32_KHR)
		m_VulkanLibrary = LoadLibrary( "vulkan-1.dll" );
	#elif defined(USE_PLATFORM_XCB_KHR)
		m_VulkanLibrary = dlopen( "libvulkan.so.1", RTLD_NOW );
	#endif

		if(m_VulkanLibrary == nullptr) {
			std::cout << "Could not load Vulkan library!" << std::endl;
			return false;
		}
		return true;		
	}
}
#endif