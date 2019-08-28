#pragma once

#include "vulkan/VulkanAppBase.h"
#include <glm/vec3.hpp>

class TriangleApp : public VulkanAppBase
{
protected:
	struct BufferObj
	{
		VkBuffer buffer;
		VkDeviceMemory  memory;
	};
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
	};


public:
	TriangleApp();
	~TriangleApp();

	virtual void prepare() override;
	virtual void cleanup() override;

	virtual void makeCommand(VkCommandBuffer command) override;

private:
	BufferObj
	_CreateBufferObj(uint32 size, VkBufferUsageFlags usage);


private:
	//ÉÅÉìÉo
	BufferObj m_vertexBuffer;
	BufferObj m_indexBuffer;

	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipeline;
	uint32 m_indexCount;
};