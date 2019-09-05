#ifndef __Vulkan_CubeTexApp_H__
#define __Vulkan_CubeTexApp_H__

#include "vulkan/VulkanAppBase.h"


class CubeTexApp : public VulkanAppBase
{
public:
	CubeTexApp();
	~CubeTexApp();

public:
	virtual void prepare() override;
	virtual void cleanup() override;

	virtual void makeCommand(VkCommandBuffer command) override;


private:
	struct BufferObj
	{
		VkBuffer buffer;
		VkDeviceMemory  memory;
	};
	struct TextureObj
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
	};
	struct ShaderParameters
	{
		glm::mat4 mtxWorld;
		glm::mat4 mtxView;
		glm::mat4 mtxProj;
	};
	struct CubeVertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 uv;
	};


private:
	BufferObj
	_CreateBufferObj(uint32 size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
	VkPipelineShaderStageCreateInfo
	_LoadShaderModule(const wchar* fileName, VkShaderStageFlagBits stage);

	void
	_CreateCube(void);
	void
	_CreateUniformBuffers(void);
	void
	_CreateDescriptorSetLayout(void);
	void
	_CreateDescriptorPool(void);
	void
	_CreateDescriptorSet(void);

	VkSampler
	_CreateSampler(void);
	TextureObj
	_CreateTexture(const char* fileName);
	void
	_SetImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
	BufferObj m_vertexBuffer;
	BufferObj m_indexBuffer;
	std::vector<BufferObj> m_uniformBuffers;
	TextureObj m_texture;

	VkDescriptorSetLayout m_descriptorSetLayout;
	VkDescriptorPool  m_descriptorPool;
	std::vector<VkDescriptorSet> m_descriptorSet;

	VkSampler m_sampler;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline   m_pipeline;
	uint32_t m_indexCount;
};


#endif//__Vulkan_CubeTexApp_H__