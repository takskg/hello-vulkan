﻿#include "pch.h"
#include "CubeTexApp.h"

using namespace glm;
using namespace std;




CubeTexApp::
CubeTexApp()
: VulkanAppBase()
, m_vertexBuffer()
, m_indexBuffer()
, m_uniformBuffers()
, m_texture()
, m_descriptorSetLayout()
, m_descriptorPool()
, m_descriptorSet()
, m_sampler()
, m_pipelineLayout()
, m_pipeline()
, m_indexCount(0)
{
}

CubeTexApp::
~CubeTexApp()
{
}

void CubeTexApp::
prepare()
{
	_CreateCube();
	_CreateUniformBuffers();
	_CreateDescriptorSetLayout();
	_CreateDescriptorPool();

	m_texture = _CreateTexture("texture.tga");

	m_sampler = _CreateSampler();
	_CreateDescriptorSet();


	// 頂点の入力設定
	VkVertexInputBindingDescription inputBinding{
	  0,                          // binding
	  sizeof(CubeVertex),         // stride
	  VK_VERTEX_INPUT_RATE_VERTEX // inputRate
	};
	array<VkVertexInputAttributeDescription, 3> inputAttribs{
	  {
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeVertex, pos)},
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeVertex, color)},
		{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(CubeVertex, uv)},
	  }
	};
	VkPipelineVertexInputStateCreateInfo vertexInputCI{};
	vertexInputCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCI.vertexBindingDescriptionCount = 1;
	vertexInputCI.pVertexBindingDescriptions = &inputBinding;
	vertexInputCI.vertexAttributeDescriptionCount = uint32_t(inputAttribs.size());
	vertexInputCI.pVertexAttributeDescriptions = inputAttribs.data();

	// ブレンディングの設定
	const auto colorWriteAll = \
		VK_COLOR_COMPONENT_R_BIT | \
		VK_COLOR_COMPONENT_G_BIT | \
		VK_COLOR_COMPONENT_B_BIT | \
		VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendAttachmentState blendAttachment{};
	blendAttachment.blendEnable = VK_TRUE;
	blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	blendAttachment.colorWriteMask = colorWriteAll;
	VkPipelineColorBlendStateCreateInfo cbCI{};
	cbCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cbCI.attachmentCount = 1;
	cbCI.pAttachments = &blendAttachment;

	// ビューポートの設定
	VkViewport viewport;
	{
		viewport.x = 0.0f;
		viewport.y = float(m_swapchainExtent.height);
		viewport.width = float(m_swapchainExtent.width);
		viewport.height = -1.0f * float(m_swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
	}
	VkRect2D scissor = {
	  { 0,0},// offset
	  m_swapchainExtent
	};
	VkPipelineViewportStateCreateInfo viewportCI{};
	viewportCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCI.viewportCount = 1;
	viewportCI.pViewports = &viewport;
	viewportCI.scissorCount = 1;
	viewportCI.pScissors = &scissor;

	// プリミティブトポロジー設定
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
	inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;


	// ラスタライザーステート設定
	VkPipelineRasterizationStateCreateInfo rasterizerCI{};
	rasterizerCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCI.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCI.cullMode = VK_CULL_MODE_NONE;
	rasterizerCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerCI.lineWidth = 1.0f;

	// マルチサンプル設定
	VkPipelineMultisampleStateCreateInfo multisampleCI{};
	multisampleCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// デプスステンシルステート設定
	VkPipelineDepthStencilStateCreateInfo depthStencilCI{};
	depthStencilCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCI.depthTestEnable = VK_TRUE;
	depthStencilCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilCI.depthWriteEnable = VK_TRUE;
	depthStencilCI.stencilTestEnable = VK_FALSE;

	// シェーダーバイナリの読み込み
	vector<VkPipelineShaderStageCreateInfo> shaderStages
	{
	  _LoadShaderModule(L"shader\\texshader.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
	  _LoadShaderModule(L"shader\\texshader.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	// パイプラインレイアウト
	VkPipelineLayoutCreateInfo pipelineLayoutCI{};
	pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCI.setLayoutCount = 1;
	pipelineLayoutCI.pSetLayouts = &m_descriptorSetLayout;
	vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutCI, nullptr, &m_pipelineLayout);

	// パイプラインの構築
	VkGraphicsPipelineCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	ci.stageCount = uint32_t(shaderStages.size());
	ci.pStages = shaderStages.data();
	ci.pInputAssemblyState = &inputAssemblyCI;
	ci.pVertexInputState = &vertexInputCI;
	ci.pRasterizationState = &rasterizerCI;
	ci.pDepthStencilState = &depthStencilCI;
	ci.pMultisampleState = &multisampleCI;
	ci.pViewportState = &viewportCI;
	ci.pColorBlendState = &cbCI;
	ci.renderPass = m_renderPass;
	ci.layout = m_pipelineLayout;
	vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &ci, nullptr, &m_pipeline);

	// ShaderModule はもう不要のため破棄
	for (const auto& v : shaderStages)
	{
		vkDestroyShaderModule(m_vkDevice, v.module, nullptr);
	}
}

void CubeTexApp::
cleanup()
{
	for (auto& v : m_uniformBuffers)
	{
		vkDestroyBuffer(m_vkDevice, v.buffer, nullptr);
		vkFreeMemory(m_vkDevice, v.memory, nullptr);
	}
	vkDestroySampler(m_vkDevice, m_sampler, nullptr);
	vkDestroyImage(m_vkDevice, m_texture.image, nullptr);
	vkDestroyImageView(m_vkDevice, m_texture.view, nullptr);
	vkFreeMemory(m_vkDevice, m_texture.memory, nullptr);

	vkDestroyPipelineLayout(m_vkDevice, m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_vkDevice, m_pipeline, nullptr);

	vkFreeMemory(m_vkDevice, m_vertexBuffer.memory, nullptr);
	vkFreeMemory(m_vkDevice, m_indexBuffer.memory, nullptr);
	vkDestroyBuffer(m_vkDevice, m_vertexBuffer.buffer, nullptr);
	vkDestroyBuffer(m_vkDevice, m_indexBuffer.buffer, nullptr);

	vkDestroyDescriptorPool(m_vkDevice, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_vkDevice, m_descriptorSetLayout, nullptr);
}

void CubeTexApp::
makeCommand(VkCommandBuffer command)
{
	// ユニフォームバッファの中身を更新する.
	static float32 camRotate = 0.0f;
	camRotate += 0.5f;
	if (camRotate > 360.0f)
	{
		camRotate -= 360.0f;
	}
	ShaderParameters shaderParam{};
	shaderParam.mtxWorld = glm::rotate(glm::identity<glm::mat4>(), glm::radians(45.0f), glm::vec3(0, 1, 0));
	shaderParam.mtxView = glm::rotate(lookAtRH(vec3(0.0f, 3.0f, 5.0f), vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f)), glm::radians(camRotate), glm::vec3(0.0f, 1.0f, 0.0f));
	shaderParam.mtxProj = perspective(glm::radians(60.0f), 640.0f / 480, 0.01f, 100.0f);
	{
		auto memory = m_uniformBuffers[m_imageIndex].memory;
		void* p;
		vkMapMemory(m_vkDevice, memory, 0, VK_WHOLE_SIZE, 0, &p);
		memcpy(p, &shaderParam, sizeof(shaderParam));
		vkUnmapMemory(m_vkDevice, memory);
	}

	// 作成したパイプラインをセット
	vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

	// 各バッファオブジェクトのセット
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(command, 0, 1, &m_vertexBuffer.buffer, &offset);
	vkCmdBindIndexBuffer(command, m_indexBuffer.buffer, offset, VK_INDEX_TYPE_UINT32);

	// ディスクリプタセットをセット
	VkDescriptorSet descriptorSets[] = {
	  m_descriptorSet[m_imageIndex]
	};
	vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, descriptorSets, 0, nullptr);

	// 3角形描画
	vkCmdDrawIndexed(command, m_indexCount, 1, 0, 0, 0);
}

CubeTexApp::BufferObj CubeTexApp::
_CreateBufferObj(uint32 size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags/* = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT*/)
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
	info.memoryTypeIndex = _GetMemoryTypeIndex(reqs.memoryTypeBits, flags);
	// メモリの確保
	vkAllocateMemory(m_vkDevice, &info, nullptr, &obj.memory);

	// メモリのバインド
	vkBindBufferMemory(m_vkDevice, obj.buffer, obj.memory, 0);
	return obj;
}

VkPipelineShaderStageCreateInfo CubeTexApp::
_LoadShaderModule(const wchar* fileName, VkShaderStageFlagBits stage)
{
	wchar exePath[_MAX_PATH];
	wstring filePath;
	GetModuleFileName(NULL, exePath, _MAX_PATH);
	wchar szDir[_MAX_DIR];
	wchar szDrive[_MAX_DRIVE];
	_wsplitpath_s(exePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, nullptr, 0, nullptr, 0);
	filePath.assign(szDrive);
	filePath.append(szDir);
	filePath.append(fileName);

	ifstream infile(filePath.c_str(), std::ios::binary);
	if (!infile)
	{
		wstring outputStr(L"file not found.\n");
		outputStr.append(fileName);
		outputStr.append(L"\n");
		OutputDebugString(outputStr.c_str());
		DebugBreak();
	}
	vector<char> filedata;
	filedata.resize(uint32_t(infile.seekg(0, ifstream::end).tellg()));
	infile.seekg(0, ifstream::beg).read(filedata.data(), filedata.size());

	VkShaderModule shaderModule;
	VkShaderModuleCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ci.pCode = reinterpret_cast<uint32_t*>(filedata.data());
	ci.codeSize = filedata.size();
	VkResult result = vkCreateShaderModule(m_vkDevice, &ci, nullptr, &shaderModule);

	VkPipelineShaderStageCreateInfo shaderStageCI{};
	shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCI.stage = stage;
	shaderStageCI.module = shaderModule;
	shaderStageCI.pName = "main";
	return shaderStageCI;
}

void CubeTexApp::
_CreateCube(void)
{
	const float k = 1.0f;
	const vec3 red(1.0f, 0.0f, 0.0f);
	const vec3 green(0.0f, 1.0f, 0.0f);
	const vec3 blue(0.0f, 0.0f, 1.0f);
	const vec3 white(1.0f);
	const vec3 black(0.0f);
	const vec3 yellow(1.0f, 1.0f, 0.0f);
	const vec3 magenta(1.0f, 0.0f, 1.0f);
	const vec3 cyan(0.0f, 1.0f, 1.0f);

	const vec2 lb(0.0f, 0.0f);
	const vec2 lt(0.0f, 1.0f);
	const vec2 rb(1.0f, 0.0f);
	const vec2 rt(1.0f, 1.0f);
	CubeVertex vertices[] = {
		// front
		// 正面.
		{ vec3(-k, k, k), yellow,  lb },
		{ vec3(-k,-k, k), red,     lt },
		{ vec3(k, k, k), white,   rb },
		{ vec3(k,-k, k), magenta, rt },
		// 右.
		{ vec3(k, k, k), white,   lb },
		{ vec3(k,-k, k), magenta, lt },
		{ vec3(k, k,-k), cyan,    rb },
		{ vec3(k,-k,-k), blue,    rt },
		// 左
		{ vec3(-k, k,-k), green,  lb },
		{ vec3(-k,-k,-k), black,  lt },
		{ vec3(-k, k, k), yellow, rb },
		{ vec3(-k,-k, k), red,    rt },
		// 裏.
		{ vec3(k, k,-k), cyan, lb },
		{ vec3(k,-k,-k), blue, lt },
		{ vec3(-k, k,-k), green, rb },
		{ vec3(-k,-k,-k), black, rt },
		// 上.
		{ vec3(-k, k,-k), green, lb },
		{ vec3(-k, k, k), yellow, lt },
		{ vec3(k, k,-k), cyan, rb },
		{ vec3(k, k, k), white, rt },
		// 底.
		{ vec3(-k,-k, k), red, lb },
		{ vec3(-k,-k,-k), black, lt },
		{ vec3(k,-k, k), magenta, rb },
		{ vec3(k,-k,-k), blue, rt },
	};
	uint32_t indices[] = {
	  0, 2, 1, 1, 2, 3, // front
	  4, 6, 5, 5, 6, 7, // right
	  8,10, 9, 9,10,11, // left

	  12, 14, 13, 13, 14, 15, // back
	  16, 18, 17, 17, 18, 19, // top
	  20, 22, 21, 21, 22, 23, // bottom
	};


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
}

void CubeTexApp::
_CreateUniformBuffers(void)
{
	m_uniformBuffers.resize(m_swapchainViews.size());
	for (auto& v : m_uniformBuffers)
	{
		VkMemoryPropertyFlags uboFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		v = _CreateBufferObj(sizeof(ShaderParameters), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags);
	}
}

void CubeTexApp::
_CreateDescriptorSetLayout(void)
{
	vector<VkDescriptorSetLayoutBinding> bindings;
	VkDescriptorSetLayoutBinding bindingUBO{}, bindingTex{};
	bindingUBO.binding = 0;
	bindingUBO.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindingUBO.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindingUBO.descriptorCount = 1;
	bindings.push_back(bindingUBO);

	bindingTex.binding = 1;
	bindingTex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindingTex.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindingTex.descriptorCount = 1;
	bindings.push_back(bindingTex);

	VkDescriptorSetLayoutCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ci.bindingCount = uint32_t(bindings.size());
	ci.pBindings = bindings.data();
	vkCreateDescriptorSetLayout(m_vkDevice, &ci, nullptr, &m_descriptorSetLayout);
}

void CubeTexApp::
_CreateDescriptorPool(void)
{
	array<VkDescriptorPoolSize, 2> descPoolSize;
	descPoolSize[0].descriptorCount = 1;
	descPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descPoolSize[1].descriptorCount = 1;
	descPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.maxSets = uint32_t(m_uniformBuffers.size());
	ci.poolSizeCount = uint32_t(descPoolSize.size());
	ci.pPoolSizes = descPoolSize.data();
	vkCreateDescriptorPool(m_vkDevice, &ci, nullptr, &m_descriptorPool);
}

void CubeTexApp::
_CreateDescriptorSet(void)
{
	vector<VkDescriptorSetLayout> layouts;
	for (int i = 0; i<int(m_uniformBuffers.size()); ++i)
	{
		layouts.push_back(m_descriptorSetLayout);
	}
	VkDescriptorSetAllocateInfo ai{};
	ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	ai.descriptorPool = m_descriptorPool;
	ai.descriptorSetCount = static_cast<uint32>(m_uniformBuffers.size());
	ai.pSetLayouts = layouts.data();
	m_descriptorSet.resize(m_uniformBuffers.size());
	auto result = vkAllocateDescriptorSets(m_vkDevice, &ai, m_descriptorSet.data());

	// ディスクリプタセットへ書き込み.
	for (int i = 0; i<int(m_uniformBuffers.size()); ++i)
	{
		VkDescriptorBufferInfo descUBO{};
		descUBO.buffer = m_uniformBuffers[i].buffer;
		descUBO.offset = 0;
		descUBO.range = VK_WHOLE_SIZE;

		VkDescriptorImageInfo  descImage{};
		descImage.imageView = m_texture.view;
		descImage.sampler = m_sampler;
		descImage.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet ubo{};
		ubo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		ubo.dstBinding = 0;
		ubo.descriptorCount = 1;
		ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo.pBufferInfo = &descUBO;
		ubo.dstSet = m_descriptorSet[i];

		VkWriteDescriptorSet tex{};
		tex.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		tex.dstBinding = 1;
		tex.descriptorCount = 1;
		tex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		tex.pImageInfo = &descImage;
		tex.dstSet = m_descriptorSet[i];

		vector<VkWriteDescriptorSet> writeSets = {
		  ubo, tex
		};
		vkUpdateDescriptorSets(m_vkDevice, uint32_t(writeSets.size()), writeSets.data(), 0, nullptr);
	}
}

VkSampler CubeTexApp::
_CreateSampler(void)
{
	VkSampler sampler;
	VkSamplerCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	ci.minFilter = VK_FILTER_LINEAR;
	ci.magFilter = VK_FILTER_LINEAR;
	ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	ci.maxAnisotropy = 1.0f;
	ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	vkCreateSampler(m_vkDevice, &ci, nullptr, &sampler);
	return sampler;
}

CubeTexApp::TextureObj CubeTexApp::
_CreateTexture(const char* fileName)
{
	BufferObj stagingBuffer;
	TextureObj texture{};
	int width, height, channels;
	char exePath[_MAX_PATH];
	string filePath;
	GetModuleFileNameA(NULL, exePath, _MAX_PATH);
	char szDir[_MAX_DIR];
	char szDrive[_MAX_DRIVE];
	_splitpath_s(exePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, nullptr, 0, nullptr, 0);
	filePath.assign(szDrive);
	filePath.append(szDir);
	filePath.append(fileName);

	auto* pImage = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
	auto format = VK_FORMAT_R8G8B8A8_UNORM;

	{
		// テクスチャのVkImage を生成
		VkImageCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ci.extent = { uint32_t(width), uint32_t(height), 1 };
		ci.format = format;
		ci.imageType = VK_IMAGE_TYPE_2D;
		ci.arrayLayers = 1;
		ci.mipLevels = 1;
		ci.samples = VK_SAMPLE_COUNT_1_BIT;
		ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		vkCreateImage(m_vkDevice, &ci, nullptr, &texture.image);

		// メモリ量の算出
		VkMemoryRequirements reqs;
		vkGetImageMemoryRequirements(m_vkDevice, texture.image, &reqs);
		VkMemoryAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize = reqs.size;
		// メモリタイプの判定
		info.memoryTypeIndex = _GetMemoryTypeIndex(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		// メモリの確保
		vkAllocateMemory(m_vkDevice, &info, nullptr, &texture.memory);
		// メモリのバインド
		vkBindImageMemory(m_vkDevice, texture.image, texture.memory, 0);
	}

	{
		uint32_t imageSize = width * height * sizeof(uint32_t);
		// ステージングバッファを用意.
		stagingBuffer = _CreateBufferObj(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		void* p;
		vkMapMemory(m_vkDevice, stagingBuffer.memory, 0, VK_WHOLE_SIZE, 0, &p);
		memcpy(p, pImage, imageSize);
		vkUnmapMemory(m_vkDevice, stagingBuffer.memory);
	}

	VkBufferImageCopy copyRegion{};
	copyRegion.imageExtent = { uint32_t(width), uint32_t(height), 1 };
	copyRegion.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	VkCommandBuffer command;
	{
		VkCommandBufferAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		ai.commandBufferCount = 1;
		ai.commandPool = m_vkCommandPool;
		ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		vkAllocateCommandBuffers(m_vkDevice, &ai, &command);
	}

	VkCommandBufferBeginInfo commandBI{};
	commandBI.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(command, &commandBI);
	_SetImageMemoryBarrier(command, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkCmdCopyBufferToImage(command, stagingBuffer.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	_SetImageMemoryBarrier(command, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	vkEndCommandBuffer(command);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command;
	vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
	{
		// テクスチャ参照用のビューを生成
		VkImageViewCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ci.image = texture.image;
		ci.format = format;
		ci.components = {
		  VK_COMPONENT_SWIZZLE_R,
		  VK_COMPONENT_SWIZZLE_G,
		  VK_COMPONENT_SWIZZLE_B,
		  VK_COMPONENT_SWIZZLE_A,
		};
		ci.subresourceRange = {
		  VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1
		};
		vkCreateImageView(m_vkDevice, &ci, nullptr, &texture.view);
	}

	vkDeviceWaitIdle(m_vkDevice);
	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &command);

	// ステージングバッファ解放.
	vkFreeMemory(m_vkDevice, stagingBuffer.memory, nullptr);
	vkDestroyBuffer(m_vkDevice, stagingBuffer.buffer, nullptr);

	stbi_image_free(pImage);
	return texture;
}

void CubeTexApp::
_SetImageMemoryBarrier(VkCommandBuffer command, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkImageMemoryBarrier imb{};
	imb.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imb.oldLayout = oldLayout;
	imb.newLayout = newLayout;
	imb.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imb.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imb.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	imb.image = image;

	VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

	switch (oldLayout)
	{
	case VK_IMAGE_LAYOUT_UNDEFINED:
		imb.srcAccessMask = 0;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		imb.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	}

	switch (newLayout)
	{
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		imb.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		imb.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		break;
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		imb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		break;
	}

	//srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;  // パイプライン中でリソースへの書込み最終のステージ.
	//dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;  // パイプライン中で次にリソースに書き込むステージ.

	vkCmdPipelineBarrier(
		command,
		srcStage,
		dstStage,
		0,
		0,  // memoryBarrierCount
		nullptr,
		0,  // bufferMemoryBarrierCount
		nullptr,
		1,  // imageMemoryBarrierCount
		&imb);
}
