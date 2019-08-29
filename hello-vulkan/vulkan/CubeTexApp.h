#pragma once

#include "vulkan/VulkanAppBase.h"
#include <glm/glm.hpp>


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


private:
	BufferObj
	_CreateBufferObj(uint32 size, VkBufferUsageFlags usage);
	VkPipelineShaderStageCreateInfo
	_LoadShaderModule(const wchar* fileName, VkShaderStageFlagBits stage);


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
