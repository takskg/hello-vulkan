#ifndef __Vulkan_VulkanAppBase_H__
#define __Vulkan_VulkanAppBase_H__



//Vulkanの実装はここに押し込める
class VulkanAppBase
{
public:
	VulkanAppBase();
	virtual ~VulkanAppBase();
	
	void initialize(GLFWwindow* window, const char* appName);
	void terminate();

public:
	virtual
	void
	render();
	virtual void prepare() { }
	virtual void cleanup() { }
	virtual void makeCommand(VkCommandBuffer command) { }



protected:
	bool
	_CreateInstance(const char* appName);
	void
	_SelectPhysicalDevice(void);
	uint32
	_SearchGraphicsQueueIndex(void);
	void
	_CreateDevice(void);
	void
	_CreateCommandPool(void);
	void
	_SelectSurfaceFormat(VkFormat format);
	void
	_CreateSurface(GLFWwindow* window);
	void
	_CreateSwapChain(GLFWwindow* window);
	void
	_CreateDepthBuffer(void);
	uint32
	_GetMemoryTypeIndex(uint32_t requestBits, VkMemoryPropertyFlags requestProps) const;
	void
	_CreateViews();
	void
	_CreateRenderPass();
	void
	_CreateFramebuffer();
	void
	_CreateCommandBuffers();
	void
	_CreateSemaphores();


protected:
	VkInstance m_vkInstance;
	VkDevice m_vkDevice;
	VkPhysicalDevice m_vkPhysicalDevice;
	VkPhysicalDeviceMemoryProperties m_vkDeviceMemProps;
	VkQueue m_vkQueue;
	VkCommandPool m_vkCommandPool;

	VkSurfaceKHR        m_surface;
	VkSurfaceFormatKHR  m_surfaceFormat;
	VkSurfaceCapabilitiesKHR  m_surfaceCaps;

	VkSwapchainKHR  m_swapchain;
	VkExtent2D    m_swapchainExtent;
	VkPresentModeKHR m_presentMode;
	std::vector<VkImage> m_swapchainImages;
	std::vector<VkImageView> m_swapchainViews;

	VkImage         m_depthBuffer;
	VkDeviceMemory  m_depthBufferMemory;
	VkImageView     m_depthBufferView;

	VkRenderPass      m_renderPass;
	std::vector<VkFramebuffer>    m_framebuffers;

	std::vector<VkFence>          m_fences;
	VkSemaphore m_renderCompletedSem;
	VkSemaphore	m_presentCompletedSem;

	std::vector<VkCommandBuffer> m_commands;

	uint32 m_graphicsQueueIndex;
	uint32  m_imageIndex;


private://Debug
	void
	_EnableDebugReport();
	void
	_DisableDebugReport();

	PFN_vkCreateDebugReportCallbackEXT	m_vkCreateDebugReportCallbackEXT;
	PFN_vkDebugReportMessageEXT	m_vkDebugReportMessageEXT;
	PFN_vkDestroyDebugReportCallbackEXT m_vkDestroyDebugReportCallbackEXT;
	VkDebugReportCallbackEXT  m_debugReport;
};



#endif//__Vulkan_VulkanAppBase_H__