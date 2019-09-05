#ifndef __Vulkan_ModelApp__
#define __Vulkan_ModelApp__

#include "vulkan/VulkanAppBase.h"

namespace Microsoft
{
	namespace glTF
	{
		class Document;
		class GLTFResourceReader;
	}
}


class ModelApp : public VulkanAppBase
{
public:
	ModelApp();
	~ModelApp();

	virtual void prepare(void) override;
	virtual void cleanup(void) override;
	virtual void makeCommand(VkCommandBuffer command) override;

private:
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 uv;
	};
	struct BufferObj
	{
		VkBuffer buffer;
		VkDeviceMemory memory;
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
	struct ModelMesh
	{
		BufferObj vertexBuffer;
		BufferObj indexBuffer;
		uint32 vertexCount;
		uint32 indexCount;
		int32 materialIndex;
		std::vector<VkDescriptorSet> descriptoreSet;
	};
	struct Material 
	{
		TextureObj texture;
		Microsoft::glTF::AlphaMode alphaMode;
	};
	struct Model 
	{
		std::vector<ModelMesh> meshes;
		std::vector<Material> materials;
	};


private:
	void
	_CreateModelGeometry(const Microsoft::glTF::Document&, std::shared_ptr<Microsoft::glTF::GLTFResourceReader> reader);
	void
	_CreateModelMaterial(const Microsoft::glTF::Document&, std::shared_ptr<Microsoft::glTF::GLTFResourceReader> reader);

	void
	_CreateUniformBuffers(void);
	void
	_CreateDescriptorSetLayout(void);
	void
	_CreateDescriptorPool(void);
	void
	_CreateDescriptorSet(void);

	BufferObj
	_CreateBufferObj(uint32 size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, const void* initialData);
	VkPipelineShaderStageCreateInfo
	_LoadShaderModule(const wchar* fileName, VkShaderStageFlagBits stage);
	VkSampler
	_CreateSampler(void);
	TextureObj
	_CreateTextureFromMemory(const std::vector<char>& imageData);
	void
	_SetImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

private:
	Model m_model;
	std::vector<BufferObj> m_uniformBuffers;
	VkDescriptorSetLayout m_descriptorSetLayout;
	VkDescriptorPool m_descriptorPool;
	VkSampler m_sampler;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_pipelineOpaque;
	VkPipeline m_pipelineAlpha;
};



#endif//__Vulkan_ModelApp__