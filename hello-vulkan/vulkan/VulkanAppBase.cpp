#include "vulkan/VulkanAppBase.h"


VulkanAppBase::
VulkanAppBase()
: m_vkInstance()
, m_vkDevice()
, m_vkPhysicalDevice()
, m_vkDeviceMemProps()
, m_vkQueue()
, m_vkCommandPool()
, m_surface()
, m_surfaceFormat()
, m_surfaceCaps()
, m_swapchain()
, m_swapchainExtent()
, m_presentMode(VK_PRESENT_MODE_FIFO_KHR)
, m_swapchainImages()
, m_swapchainViews()
, m_depthBuffer()
, m_depthBufferMemory()
, m_depthBufferView()
, m_renderPass()
, m_framebuffers()
, m_fences()
, m_renderCompletedSem()
, m_presentCompletedSem()
, m_commands()
, m_graphicsQueueIndex(0)
{

}
/*virtual*/
VulkanAppBase::
~VulkanAppBase()
{

}

void VulkanAppBase::
initialize(GLFWwindow* window, const char* appName)
{
	//インスタンス作成
	_CreateInstance(appName);

	//デバイス選択
	_SelectPhysicalDevice();

	m_graphicsQueueIndex = _SearchGraphicsQueueIndex();

	//デバイス作成
	_CreateDevice();
	//コマンドプール作成
	_CreateCommandPool();

	//サーフェイス作成
	_CreateSurface(window);
	//スワップチェイン作成
	_CreateSwapChain(window);

	//デプスバッファ作成
	_CreateDepthBuffer();
	//スワップチェインイメージとデプスバッファへのImageViewを生成
	_CreateViews();

	//フレームバッファ作成
	_CreateFramebuffer();

	//コマンドバッファ作成
	_CreateCommandBuffers();

	//描画フレーム同期用
	_CreateSemaphores();

	prepare();
}
void VulkanAppBase::
terminate()
{
	vkDeviceWaitIdle(m_vkDevice);

	cleanup();

	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, uint32_t(m_commands.size()), m_commands.data());
	m_commands.clear();

	vkDestroyRenderPass(m_vkDevice, m_renderPass, nullptr);
	for (auto& v : m_framebuffers)
	{
		vkDestroyFramebuffer(m_vkDevice, v, nullptr);
	}
	m_framebuffers.clear();

	vkFreeMemory(m_vkDevice, m_depthBufferMemory, nullptr);
	vkDestroyImage(m_vkDevice, m_depthBuffer, nullptr);
	vkDestroyImageView(m_vkDevice, m_depthBufferView, nullptr);

	for (auto& v : m_swapchainViews)
	{
		vkDestroyImageView(m_vkDevice, v, nullptr);
	}
	m_swapchainImages.clear();
	vkDestroySwapchainKHR(m_vkDevice, m_swapchain, nullptr);

	for (auto& v : m_fences)
	{
		vkDestroyFence(m_vkDevice, v, nullptr);
	}
	m_fences.clear();
	vkDestroySemaphore(m_vkDevice, m_presentCompletedSem, nullptr);
	vkDestroySemaphore(m_vkDevice, m_renderCompletedSem, nullptr);

	vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);

	vkDestroySurfaceKHR(m_vkInstance, m_surface, nullptr);
	vkDestroyDevice(m_vkDevice, nullptr);
#ifdef _DEBUG
	//disableDebugReport();
#endif
	vkDestroyInstance(m_vkInstance, nullptr);
}






bool VulkanAppBase::
_CreateInstance(const char* appName)
{
	std::vector<const char*> extentions;

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = appName;
	appInfo.pEngineName = appName;
	appInfo.apiVersion = VK_API_VERSION_1_1;
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

	const char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = layers;
	createInfo.enabledExtensionCount = static_cast<uint32>(extentions.size());
	createInfo.ppEnabledExtensionNames = extentions.data();
	createInfo.pApplicationInfo = &appInfo;
	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);

	return true;
}

void VulkanAppBase::
_SelectPhysicalDevice(void)
{
	uint32 deviceCount = 0;
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
	std::vector<VkPhysicalDevice> physDevices(deviceCount);
	vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, physDevices.data());

	//とりあえず最初のデバイスを使用
	m_vkPhysicalDevice = physDevices[0];
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &m_vkDeviceMemProps);
}

uint32 VulkanAppBase::
_SearchGraphicsQueueIndex(void)
{
	uint32 propCount = 0u;
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &propCount, nullptr);
	std::vector<VkQueueFamilyProperties> props(propCount);
	vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &propCount, props.data());

	uint32 queueCount = 0u;
	for (uint32 idx=0; idx<queueCount; ++idx)
	{
		if (props[idx].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queueCount = idx;
			break;
		}
	}
	return queueCount;
}

void VulkanAppBase::
_CreateDevice(void)
{
	const float32 defaultQueuePriority = 1.0f;
	VkDeviceQueueCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueFamilyIndex = m_graphicsQueueIndex;
	createInfo.queueCount = 1;
	createInfo.pQueuePriorities = &defaultQueuePriority;

	std::vector<VkExtensionProperties> extProps;
	{
		uint32 count = 0;
		vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &count, nullptr);
		extProps.resize(count);
		vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &count, extProps.data());
	}

	std::vector<const char*> extentions;
	for (const auto& v : extProps)
	{
		extentions.push_back(v.extensionName);
	}

	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pQueueCreateInfos = &createInfo;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.ppEnabledExtensionNames = extentions.data();
	deviceInfo.enabledExtensionCount = static_cast<uint32>(extentions.size());

	VkResult result = vkCreateDevice(m_vkPhysicalDevice, &deviceInfo, nullptr, &m_vkDevice);

	vkGetDeviceQueue(m_vkDevice, m_graphicsQueueIndex, 0, &m_vkQueue);
}

void VulkanAppBase::
_CreateCommandPool(void)
{
	VkCommandPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.queueFamilyIndex = m_graphicsQueueIndex;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkCreateCommandPool(m_vkDevice, &info, nullptr, &m_vkCommandPool);
}

void VulkanAppBase::
_SelectSurfaceFormat(VkFormat format)
{
	uint32_t surfaceFormatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_surface, &surfaceFormatCount, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(surfaceFormatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, m_surface, &surfaceFormatCount, formats.data());

	// 検索して一致するフォーマットを見つける.
	for (const auto& f : formats)
	{
		if (f.format == format)
		{
			m_surfaceFormat = f;
		}
	}
}

void VulkanAppBase::
_CreateSurface(GLFWwindow* window)
{
	// サーフェース生成
	glfwCreateWindowSurface(m_vkInstance, window, nullptr, &m_surface);
	// サーフェースのフォーマット情報選択
	_SelectSurfaceFormat(VK_FORMAT_B8G8R8A8_UNORM);
	// サーフェースの能力値情報取得
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, m_surface, &m_surfaceCaps);
	VkBool32 isSupport;
	vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevice, m_graphicsQueueIndex, m_surface, &isSupport);
}

void VulkanAppBase::
_CreateSwapChain(GLFWwindow* window)
{
	auto imageCount = (std::max)(2u, m_surfaceCaps.minImageCount);
	auto extent = m_surfaceCaps.currentExtent;
	if (extent.width == ~0u)
	{
		// 値が無効なのでウィンドウサイズを使用する.
		int width, height;
		glfwGetWindowSize(window, &width, &height);
		extent.width = uint32_t(width);
		extent.height = uint32_t(height);
	}
	uint32_t queueFamilyIndices[] = { m_graphicsQueueIndex };
	VkSwapchainCreateInfoKHR ci{};
	ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	ci.surface = m_surface;
	ci.minImageCount = imageCount;
	ci.imageFormat = m_surfaceFormat.format;
	ci.imageColorSpace = m_surfaceFormat.colorSpace;
	ci.imageExtent = extent;
	ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	ci.preTransform = m_surfaceCaps.currentTransform;
	ci.imageArrayLayers = 1;
	ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	ci.queueFamilyIndexCount = 0;
	ci.presentMode = m_presentMode;
	ci.oldSwapchain = VK_NULL_HANDLE;
	ci.clipped = VK_TRUE;
	ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	auto result = vkCreateSwapchainKHR(m_vkDevice, &ci, nullptr, &m_swapchain);

	m_swapchainExtent = extent;
}

void VulkanAppBase::
_CreateDepthBuffer()
{
	VkImageCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	ci.imageType = VK_IMAGE_TYPE_2D;
	ci.format = VK_FORMAT_D32_SFLOAT;
	ci.extent.width = m_swapchainExtent.width;
	ci.extent.height = m_swapchainExtent.height;
	ci.extent.depth = 1;
	ci.mipLevels = 1;
	ci.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	ci.samples = VK_SAMPLE_COUNT_1_BIT;
	ci.arrayLayers = 1;
	auto result = vkCreateImage(m_vkDevice, &ci, nullptr, &m_depthBuffer);

	VkMemoryRequirements reqs;
	vkGetImageMemoryRequirements(m_vkDevice, m_depthBuffer, &reqs);
	VkMemoryAllocateInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	ai.allocationSize = reqs.size;
	ai.memoryTypeIndex = _GetMemoryTypeIndex(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(m_vkDevice, &ai, nullptr, &m_depthBufferMemory);
	vkBindImageMemory(m_vkDevice, m_depthBuffer, m_depthBufferMemory, 0);
}

uint32 VulkanAppBase::
_GetMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const
{
	uint32_t result = ~0u;
	for (uint32_t i = 0; i < m_vkDeviceMemProps.memoryTypeCount; ++i)
	{
		if (requestBits & 1)
		{
			const auto& types = m_vkDeviceMemProps.memoryTypes[i];
			if ((types.propertyFlags & requestProps) == requestProps)
			{
				result = i;
				break;
			}
		}
		requestBits >>= 1;
	}
	return result;
}

void VulkanAppBase::
_CreateViews()
{
	uint32_t imageCount;
	vkGetSwapchainImagesKHR(m_vkDevice, m_swapchain, &imageCount, nullptr);
	m_swapchainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(m_vkDevice, m_swapchain, &imageCount, m_swapchainImages.data());
	m_swapchainViews.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		VkImageViewCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ci.format = m_surfaceFormat.format;
		ci.components = {
		  VK_COMPONENT_SWIZZLE_R,
		  VK_COMPONENT_SWIZZLE_G,
		  VK_COMPONENT_SWIZZLE_B,
		  VK_COMPONENT_SWIZZLE_A,
		};
		ci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		ci.image = m_swapchainImages[i];
		auto result = vkCreateImageView(m_vkDevice, &ci, nullptr, &m_swapchainViews[i]);
	}

	// for depthbuffer
	{
		VkImageViewCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ci.format = VK_FORMAT_D32_SFLOAT;
		ci.components = {
		  VK_COMPONENT_SWIZZLE_R,
		  VK_COMPONENT_SWIZZLE_G,
		  VK_COMPONENT_SWIZZLE_B,
		  VK_COMPONENT_SWIZZLE_A,
		};
		ci.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
		ci.image = m_depthBuffer;
		auto result = vkCreateImageView(m_vkDevice, &ci, nullptr, &m_depthBufferView);
	}
}

void VulkanAppBase::
_CreateFramebuffer()
{
	VkFramebufferCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	ci.renderPass = m_renderPass;
	ci.width = m_swapchainExtent.width;
	ci.height = m_swapchainExtent.height;
	ci.layers = 1;
	m_framebuffers.clear();
	for (auto& v : m_swapchainViews)
	{
		std::array<VkImageView, 2> attachments;
		ci.attachmentCount = uint32_t(attachments.size());
		ci.pAttachments = attachments.data();
		attachments[0] = v;
		attachments[1] = m_depthBufferView;

		VkFramebuffer framebuffer;
		auto result = vkCreateFramebuffer(m_vkDevice, &ci, nullptr, &framebuffer);
		m_framebuffers.push_back(framebuffer);
	}
}

void VulkanAppBase::
_CreateCommandBuffers()
{
	VkCommandBufferAllocateInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.commandPool = m_vkCommandPool;
	ai.commandBufferCount = uint32_t(m_swapchainViews.size());
	ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	m_commands.resize(ai.commandBufferCount);
	auto result = vkAllocateCommandBuffers(m_vkDevice, &ai, m_commands.data());

	// コマンドバッファのフェンスも同数用意する.
	m_fences.resize(ai.commandBufferCount);
	VkFenceCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (auto& v : m_fences)
	{
		result = vkCreateFence(m_vkDevice, &ci, nullptr, &v);
	}
}

void VulkanAppBase::
_CreateSemaphores()
{
	VkSemaphoreCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	vkCreateSemaphore(m_vkDevice, &ci, nullptr, &m_renderCompletedSem);
	vkCreateSemaphore(m_vkDevice, &ci, nullptr, &m_presentCompletedSem);
}