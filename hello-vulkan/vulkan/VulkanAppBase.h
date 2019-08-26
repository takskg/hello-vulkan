#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <algorithm>
#include <vector>
#include <array>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan_win32.h>

//ÇÊÇ≠égÇ¢ÇªÇ§Ç»Ç‚Ç¬ÇíËã`
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float float32;
typedef double float64;


//VulkanÇÃé¿ëïÇÕÇ±Ç±Ç…âüÇµçûÇﬂÇÈ
class VulkanAppBase
{
public:
	VulkanAppBase();
	virtual ~VulkanAppBase();
	
	void initialize(GLFWwindow* window, const char* appName);
	void terminate();

public:
	virtual void render() {}
	virtual void prepare() { }
	virtual void cleanup() { }
	virtual void makeCommand(VkCommandBuffer command) { }



private:
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
	_CreateFramebuffer();
	void
	_CreateCommandBuffers();
	void
	_CreateSemaphores();

private:
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
};