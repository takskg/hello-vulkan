#include "TriangleApp.h"

using namespace glm;
using namespace std;

TriangleApp::
TriangleApp()
: VulkanAppBase()
, m_vertexBuffer()
, m_indexBuffer()
, m_pipelineLayout(nullptr)
, m_pipeline(nullptr)
, m_indexCount(0)
{

}

TriangleApp::
~TriangleApp()
{

}

void TriangleApp::
prepare()
{
	const vec3 red(1.0f, 0.0f, 0.0f);
	const vec3 green(0.0f, 1.0f, 0.0f);
	const vec3 blue(0.0f, 0.0f, 1.0f);
	Vertex vertices[] = {
	  { vec3(-1.0f, 0.0f, 0.0f), red },
	  { vec3(+1.0f, 0.0f, 0.0f), blue },
	  { vec3(0.0f, 1.0f, 0.0f), green },
	};
	uint32_t indices[] = { 0, 1, 2 };

	m_vertexBuffer = _CreateBufferObj(sizeof(vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	m_indexBuffer = _CreateBufferObj(sizeof(indices), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	// 頂点データの書き込み
	{
		void* p;
		vkMapMemory(m_vkDevice, m_vertexBuffer.memory, 0, VK_WHOLE_SIZE, 0, &p);
		memcpy(p, vertices, sizeof(vertices));
		vkUnmapMemory(m_vkDevice, m_vertexBuffer.memory);
	}
	// インデックスデータの書き込み
	{
		void* p;
		vkMapMemory(m_vkDevice, m_indexBuffer.memory, 0, VK_WHOLE_SIZE, 0, &p);
		memcpy(p, indices, sizeof(indices));
		vkUnmapMemory(m_vkDevice, m_indexBuffer.memory);
	}
	m_indexCount = _countof(indices);

	// 頂点の入力設定
	VkVertexInputBindingDescription inputBinding{
	  0,                          // binding
	  sizeof(Vertex),          // stride
	  VK_VERTEX_INPUT_RATE_VERTEX // inputRate
	};
	array<VkVertexInputAttributeDescription, 2> inputAttribs{
	  {
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
	  }
	};
}

void TriangleApp::
cleanup()
{
	vkDestroyPipelineLayout(m_vkDevice, m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_vkDevice, m_pipeline, nullptr);

	vkFreeMemory(m_vkDevice, m_vertexBuffer.memory, nullptr);
	vkFreeMemory(m_vkDevice, m_indexBuffer.memory, nullptr);
	vkDestroyBuffer(m_vkDevice, m_vertexBuffer.buffer, nullptr);
	vkDestroyBuffer(m_vkDevice, m_indexBuffer.buffer, nullptr);
}

void TriangleApp::
makeCommand(VkCommandBuffer command)
{
}




TriangleApp::BufferObj TriangleApp::
_CreateBufferObj(uint32 size, VkBufferUsageFlags usage)
{
	BufferObj obj;
	VkBufferCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	ci.usage = usage;
	ci.size = size;
	auto result = vkCreateBuffer(m_vkDevice, &ci, nullptr, &obj.buffer);

	// メモリ量の算出
	VkMemoryRequirements reqs;
	vkGetBufferMemoryRequirements(m_vkDevice, obj.buffer, &reqs);
	VkMemoryAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.allocationSize = reqs.size;
	// メモリタイプの判定
	auto flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	info.memoryTypeIndex = _GetMemoryTypeIndex(reqs.memoryTypeBits, flags);
	// メモリの確保
	vkAllocateMemory(m_vkDevice, &info, nullptr, &obj.memory);

	// メモリのバインド
	vkBindBufferMemory(m_vkDevice, obj.buffer, obj.memory, 0);
	return obj;
}
