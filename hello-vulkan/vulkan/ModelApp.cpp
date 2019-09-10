#include "pch.h"
#include "ModelApp.h"
#include "model/GLTFReader.h"


using namespace glm;
using namespace std;


ModelApp::
ModelApp()
: VulkanAppBase()
, m_model()
, m_uniformBuffers()
, m_descriptorSetLayout()
, m_descriptorPool()
, m_sampler()
, m_pipelineLayout()
, m_pipelineOpaque()
, m_pipelineAlpha()
{
}

ModelApp::
~ModelApp()
{
}

void ModelApp::
prepare(void)
{
	//モデル読み込み
	wchar exePath[_MAX_PATH];
	wstring filePath;
	GetModuleFileName(NULL, exePath, _MAX_PATH);
	wchar szDir[_MAX_DIR];
	wchar szDrive[_MAX_DRIVE];
	_wsplitpath_s(exePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, nullptr, 0, nullptr, 0);
	filePath.assign(szDrive);
	filePath.append(szDir);
	filePath.append(L"model\\model2.vrm");
	auto modelFilePath = experimental::filesystem::path(filePath.c_str());
	auto reader = make_unique<GLTFReader>(modelFilePath.parent_path());
	auto glbStream = reader->GetInputStream(modelFilePath.filename().u8string());
	auto glbResourceReader = make_shared<Microsoft::glTF::GLBResourceReader>(std::move(reader), std::move(glbStream));
	auto document = Microsoft::glTF::Deserialize(glbResourceReader->GetJson());

	_CreateModelGeometry(document, glbResourceReader);
	_CreateModelMaterial(document, glbResourceReader);

	_CreateUniformBuffers();
	_CreateDescriptorSetLayout();
	_CreateDescriptorPool();

	m_sampler = _CreateSampler();
	_CreateDescriptorSet();

	//頂点入力設定
	VkVertexInputBindingDescription inputBinding{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
	array<VkVertexInputAttributeDescription, 3> inputAttribs{
		{
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
			{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
			{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, uv)},
		}
	};
	VkPipelineVertexInputStateCreateInfo vertexInputCi{};
	vertexInputCi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCi.vertexBindingDescriptionCount = 1;
	vertexInputCi.pVertexBindingDescriptions = &inputBinding;
	vertexInputCi.vertexAttributeDescriptionCount = uint32(inputAttribs.size());
	vertexInputCi.pVertexAttributeDescriptions = inputAttribs.data();

	//ビューポート
	VkViewport viewport;
	{
		viewport.x = 0.0f;
		viewport.y = float32(m_swapchainExtent.height);
		viewport.width = float32(m_swapchainExtent.width);
		viewport.height = -1.0f * float32(m_swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
	}
	VkRect2D scissor = {
		{0,0},
		m_swapchainExtent
	};
	VkPipelineViewportStateCreateInfo viewportCi{};
	viewportCi.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCi.viewportCount = 1;
	viewportCi.pViewports = &viewport;
	viewportCi.scissorCount = 1;
	viewportCi.pScissors = &scissor;

	//プリミティブトポロジー
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCi{};
	inputAssemblyCi.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCi.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	//ラスタライザーステート
	VkPipelineRasterizationStateCreateInfo rasterizerCi{};
	rasterizerCi.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCi.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCi.cullMode = VK_CULL_MODE_NONE;
	rasterizerCi.lineWidth = 1.0f;

	//マルチサンプル
	VkPipelineMultisampleStateCreateInfo multisampleCi{};
	multisampleCi.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	//パイプラインレイアウト
	VkPipelineLayoutCreateInfo pipelineLayoutCi{};
	pipelineLayoutCi.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCi.setLayoutCount = 1;
	pipelineLayoutCi.pSetLayouts = &m_descriptorSetLayout;
	vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutCi, nullptr, &m_pipelineLayout);

	//不透明用 パイプラインの構築
	{
		//ブレンディング
		const auto colorWriteAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		VkPipelineColorBlendAttachmentState blendAttachment{};
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.colorWriteMask = colorWriteAll;
		VkPipelineColorBlendStateCreateInfo cbCi{};
		cbCi.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cbCi.attachmentCount = 1;
		cbCi.pAttachments = &blendAttachment;

		//デプスステンシルステート
		VkPipelineDepthStencilStateCreateInfo depthStencilCi{};
		depthStencilCi.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCi.depthTestEnable = VK_TRUE;
		depthStencilCi.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilCi.depthWriteEnable = VK_TRUE;
		depthStencilCi.stencilTestEnable = VK_FALSE;

		//シェーダー読み込み
		vector<VkPipelineShaderStageCreateInfo> shaderStages
		{
			_LoadShaderModule(L"shader\\texshaderUv.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			_LoadShaderModule(L"shader\\texshaderOpaque.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
		};
		//パイプライン構築
		VkGraphicsPipelineCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		ci.stageCount = uint32(shaderStages.size());
		ci.pStages = shaderStages.data();
		ci.pInputAssemblyState = &inputAssemblyCi;
		ci.pVertexInputState = &vertexInputCi;
		ci.pRasterizationState = &rasterizerCi;
		ci.pDepthStencilState = &depthStencilCi;
		ci.pMultisampleState = &multisampleCi;
		ci.pViewportState = &viewportCi;
		ci.pColorBlendState = &cbCi;
		ci.renderPass = m_renderPass;
		ci.layout = m_pipelineLayout;
		vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &ci, nullptr, &m_pipelineOpaque);

		for (const auto& v : shaderStages)
		{
			vkDestroyShaderModule(m_vkDevice, v.module, nullptr);
		}
	}

	//半透明用 パイプラインの構築
	{
		//ブレンディング
		const auto colorWriteAll = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		VkPipelineColorBlendAttachmentState blendAttachment{};
		blendAttachment.blendEnable = VK_TRUE;
		blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		blendAttachment.colorWriteMask = colorWriteAll;
		VkPipelineColorBlendStateCreateInfo cbCi{};
		cbCi.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		cbCi.attachmentCount = 1;
		cbCi.pAttachments = &blendAttachment;

		//デプスステンシルステート
		VkPipelineDepthStencilStateCreateInfo depthStencilCi{};
		depthStencilCi.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCi.depthTestEnable = VK_TRUE;
		depthStencilCi.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilCi.depthWriteEnable = VK_TRUE;
		depthStencilCi.stencilTestEnable = VK_FALSE;

		//シェーダー読み込み
		vector<VkPipelineShaderStageCreateInfo> shaderStages
		{
			_LoadShaderModule(L"shader\\texshaderUv.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			_LoadShaderModule(L"shader\\texshaderAlpha.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
		};
		//パイプライン構築
		VkGraphicsPipelineCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		ci.stageCount = uint32(shaderStages.size());
		ci.pStages = shaderStages.data();
		ci.pInputAssemblyState = &inputAssemblyCi;
		ci.pVertexInputState = &vertexInputCi;
		ci.pRasterizationState = &rasterizerCi;
		ci.pDepthStencilState = &depthStencilCi;
		ci.pMultisampleState = &multisampleCi;
		ci.pViewportState = &viewportCi;
		ci.pColorBlendState = &cbCi;
		ci.renderPass = m_renderPass;
		ci.layout = m_pipelineLayout;
		vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &ci, nullptr, &m_pipelineAlpha);

		for (const auto& v : shaderStages)
		{
			vkDestroyShaderModule(m_vkDevice, v.module, nullptr);
		}
	}
}

void ModelApp::
cleanup(void)
{
	for (auto& v : m_uniformBuffers)
	{
		vkDestroyBuffer(m_vkDevice, v.buffer, nullptr);
		vkFreeMemory(m_vkDevice, v.memory, nullptr);
	}
	vkDestroySampler(m_vkDevice, m_sampler, nullptr);

	vkDestroyPipelineLayout(m_vkDevice, m_pipelineLayout, nullptr);
	vkDestroyPipeline(m_vkDevice, m_pipelineOpaque, nullptr);
	vkDestroyPipeline(m_vkDevice, m_pipelineAlpha, nullptr);

	for (auto& mesh : m_model.meshes)
	{
		vkFreeMemory(m_vkDevice, mesh.vertexBuffer.memory, nullptr);
		vkFreeMemory(m_vkDevice, mesh.indexBuffer.memory, nullptr);
		vkDestroyBuffer(m_vkDevice, mesh.vertexBuffer.buffer, nullptr);
		vkDestroyBuffer(m_vkDevice, mesh.indexBuffer.buffer, nullptr);
		uint32 count = uint32(mesh.descriptoreSet.size());
		mesh.descriptoreSet.clear();
	}
	for (auto& material : m_model.materials)
	{
		vkFreeMemory(m_vkDevice, material.texture.memory, nullptr);
		vkDestroyImage(m_vkDevice, material.texture.image, nullptr);
		vkDestroyImageView(m_vkDevice, material.texture.view, nullptr);
	}

	vkDestroyDescriptorPool(m_vkDevice, m_descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(m_vkDevice, m_descriptorSetLayout, nullptr);
}

void ModelApp::
makeCommand(VkCommandBuffer command)
{
	using namespace Microsoft::glTF;

	//ユニフォームバッファを更新
	static float32 angle = 0.0f;
	angle += 0.5f;
	if (angle > 360.0f)
	{
		angle -= 360.0f;
	}
	ShaderParameters shaderParam{};
	shaderParam.mtxWorld = glm::identity<glm::mat4>();
	shaderParam.mtxView = glm::rotate(lookAtRH(vec3(0.0f, 1.5f, -1.0f), vec3(0.0f, 1.25f, 0.0f), vec3(0.0f, 1.0f, 0.0f)), static_cast<float32>(glm::radians(angle)), vec3(0.0f, 1.0f, 0.0f));
	shaderParam.mtxProj = perspective(glm::radians(60.0f), 1280.0f / 720.0f, 0.01f, 100.0f);
	{
		auto memory = m_uniformBuffers[m_imageIndex].memory;
		void* p;
		vkMapMemory(m_vkDevice, memory, 0, VK_WHOLE_SIZE, 0, &p);
		memcpy(p, &shaderParam, sizeof(shaderParam));
		vkUnmapMemory(m_vkDevice, memory);
	}

	for (auto mode : {ALPHA_OPAQUE, ALPHA_MASK, ALPHA_BLEND})
	{
		for (const auto& mesh : m_model.meshes)
		{
			//対応するメッシュのみを描画する
			if (m_model.materials[mesh.materialIndex].alphaMode != mode)
			{
				continue;
			}

			//モードに応じてパイプラインを変更する
			switch (mode)
			{
			case Microsoft::glTF::ALPHA_OPAQUE:
				vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineOpaque);
				break;
			case Microsoft::glTF::ALPHA_BLEND:
				vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineOpaque);
				break;
			case Microsoft::glTF::ALPHA_MASK:
				vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineAlpha);
				break;

			default:
				break;
			}

			//バッファオブジェクトのセット
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(command, 0, 1, &mesh.vertexBuffer.buffer, &offset);
			vkCmdBindIndexBuffer(command, mesh.indexBuffer.buffer, offset, VK_INDEX_TYPE_UINT32);

			//ディスクリプタセットのセット
			VkDescriptorSet descriptorSets[] = {
				mesh.descriptoreSet[m_imageIndex]
			};
			vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, descriptorSets, 0, nullptr);

			//メッシュの描画
			vkCmdDrawIndexed(command, mesh.indexCount, 1, 0, 0, 0);
		}
	}
}

void ModelApp::
_CreateModelGeometry(const Microsoft::glTF::Document& doc, std::shared_ptr<Microsoft::glTF::GLTFResourceReader> reader)
{
	using namespace Microsoft::glTF;
	for (const auto& mesh : doc.meshes.Elements())
	{
		for (const auto& meshPrimitive : mesh.primitives)
		{
			std::vector<Vertex> vertices;
			std::vector<uint32> indices;

			//頂点位置情報取得
			auto& idPos = meshPrimitive.GetAttributeAccessorId(ACCESSOR_POSITION);
			auto& accPos = doc.accessors.Get(idPos);
			//法線情報の取得
			auto& idNrm = meshPrimitive.GetAttributeAccessorId(ACCESSOR_NORMAL);
			auto& accNrm = doc.accessors.Get(idNrm);
			//UV座標の取得
			auto& idUv = meshPrimitive.GetAttributeAccessorId(ACCESSOR_TEXCOORD_0);
			auto& accUv = doc.accessors.Get(idUv);
			//頂点インデックスの取得
			auto& idIdx = meshPrimitive.indicesAccessorId;
			auto& accIdx = doc.accessors.Get(idIdx);

			//実データ列を取得
			auto vertPos = reader->ReadBinaryData<float32>(doc, accPos);
			auto vertNrm = reader->ReadBinaryData<float32>(doc, accNrm);
			auto vertUv = reader->ReadBinaryData<float32>(doc, accUv);

			auto vertCount = accPos.count;
			for (uint32 idx=0; idx<vertCount; ++idx)
			{
				//頂点データの構築
				int32 vid0 = 3 * idx, vid1 = 3 * idx + 1, vid2 = 3 * idx + 2;
				int32 tid0 = 2 * idx, tid1 = 2 * idx + 1;
				vertices.emplace_back(
					Vertex{
						vec3(vertPos[vid0], vertPos[vid1], vertPos[vid2]),
						vec3(vertNrm[vid0], vertNrm[vid1], vertNrm[vid2]),
						vec2(vertUv[tid0], vertUv[tid1])
					}
				);
			}

			//インデックスの構築
			indices = reader->ReadBinaryData<uint32>(doc, accIdx);
			auto vbSize = uint32(sizeof(Vertex) * vertices.size());
			auto idSize = uint32(sizeof(uint32) * indices.size());
			ModelMesh mesh;
			mesh.vertexBuffer = _CreateBufferObj(vbSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertices.data());
			mesh.indexBuffer = _CreateBufferObj(idSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indices.data());
			mesh.vertexCount = uint32(vertices.size());
			mesh.indexCount = uint32(indices.size());
			mesh.materialIndex = int32(doc.materials.GetIndex(meshPrimitive.materialId));
			m_model.meshes.push_back(mesh);
		}
	}
}

void ModelApp::
_CreateModelMaterial(const Microsoft::glTF::Document& doc, std::shared_ptr<Microsoft::glTF::GLTFResourceReader> reader)
{
	for (auto& m : doc.materials.Elements())
	{
		auto textureId = m.metallicRoughness.baseColorTexture.textureId;
		if (textureId.empty())
		{
			textureId = m.normalTexture.textureId;
		}
		auto& texture = doc.textures.Get(textureId);
		auto& image = doc.images.Get(texture.imageId);
		auto imageBufferView = doc.bufferViews.Get(image.bufferViewId);
		auto imageData = reader->ReadBinaryData<char>(doc, imageBufferView);

		//imageDataからテクスチャを生成
		Material material{};
		material.alphaMode = m.alphaMode;
		material.texture = _CreateTextureFromMemory(imageData);
		m_model.materials.push_back(material);
	}
}

void ModelApp::
_CreateUniformBuffers(void)
{
	m_uniformBuffers.resize(m_swapchainViews.size());
	for (auto& v : m_uniformBuffers)
	{
		VkMemoryPropertyFlags uboFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		v = _CreateBufferObj(sizeof(ShaderParameters), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, uboFlags, nullptr);
	}
}

void ModelApp::
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
	ci.bindingCount = uint32(bindings.size());
	ci.pBindings = bindings.data();
	vkCreateDescriptorSetLayout(m_vkDevice, &ci, nullptr, &m_descriptorSetLayout);
}

void ModelApp::
_CreateDescriptorPool(void)
{
	array<VkDescriptorPoolSize, 2> descPoolSize;
	descPoolSize[0].descriptorCount = 1;
	descPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descPoolSize[1].descriptorCount = 1;
	descPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	uint32 maxDescriptorCount = uint32(m_swapchainImages.size() * m_model.meshes.size());
	VkDescriptorPoolCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.maxSets = maxDescriptorCount;
	ci.poolSizeCount = uint32(descPoolSize.size());
	ci.pPoolSizes = descPoolSize.data();
	vkCreateDescriptorPool(m_vkDevice, &ci, nullptr, &m_descriptorPool);
}

void ModelApp::
_CreateDescriptorSet(void)
{
	vector<VkDescriptorSetLayout> layouts;
	for (uint32 idx=0; idx<uint32(m_uniformBuffers.size()); ++idx)
	{
		layouts.push_back(m_descriptorSetLayout);
	}

	for (auto& mesh : m_model.meshes)
	{
		//ディスクリプタセットの確保
		VkDescriptorSetAllocateInfo ai{};
		ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		ai.descriptorPool = m_descriptorPool;
		ai.descriptorSetCount = uint32(m_uniformBuffers.size());
		ai.pSetLayouts = layouts.data();
		mesh.descriptoreSet.resize(m_uniformBuffers.size());
		vkAllocateDescriptorSets(m_vkDevice, &ai, mesh.descriptoreSet.data());

		//ディスクリプタセットへ書き込み
		auto material = m_model.materials[mesh.materialIndex];
		for (uint32 idx=0; idx<uint32(m_uniformBuffers.size()); ++idx)
		{
			VkDescriptorBufferInfo descUbo{};
			descUbo.buffer = m_uniformBuffers[idx].buffer;
			descUbo.offset = 0;
			descUbo.range = VK_WHOLE_SIZE;

			VkDescriptorImageInfo descImg{};
			descImg.imageView = material.texture.view;
			descImg.sampler = m_sampler;
			descImg.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkWriteDescriptorSet ubo{};
			ubo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			ubo.dstBinding = 0;
			ubo.descriptorCount = 1;
			ubo.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			ubo.pBufferInfo = &descUbo;
			ubo.dstSet = mesh.descriptoreSet[idx];

			VkWriteDescriptorSet tex{};
			tex.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			tex.dstBinding = 1;
			tex.descriptorCount = 1;
			tex.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			tex.pImageInfo = &descImg;
			tex.dstSet = mesh.descriptoreSet[idx];

			vector<VkWriteDescriptorSet> writeSets = {
				ubo, tex
			};
			vkUpdateDescriptorSets(m_vkDevice, uint32(writeSets.size()), writeSets.data(), 0, nullptr);
		}
	}
}

ModelApp::BufferObj ModelApp::
_CreateBufferObj(uint32 size, VkBufferUsageFlags usage, VkMemoryPropertyFlags flags, const void* initialData)
{
	BufferObj obj;
	VkBufferCreateInfo ci{};
	ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	ci.usage = usage;
	ci.size = size;
	vkCreateBuffer(m_vkDevice, &ci, nullptr, &obj.buffer);

	//メモリ量の算出
	VkMemoryRequirements reqs;
	vkGetBufferMemoryRequirements(m_vkDevice, obj.buffer, &reqs);
	VkMemoryAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.allocationSize = reqs.size;
	info.memoryTypeIndex = _GetMemoryTypeIndex(reqs.memoryTypeBits, flags);
	vkAllocateMemory(m_vkDevice, &info, nullptr, &obj.memory);
	vkBindBufferMemory(m_vkDevice, obj.buffer, obj.memory, 0);

	if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0 && initialData != nullptr)
	{
		void* p;
		vkMapMemory(m_vkDevice, obj.memory, 0, VK_WHOLE_SIZE, 0, &p);
		memcpy(p, initialData, size);
		vkUnmapMemory(m_vkDevice, obj.memory);
	}
	return obj;
}

VkPipelineShaderStageCreateInfo ModelApp::
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

VkSampler ModelApp::
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

ModelApp::TextureObj ModelApp::
_CreateTextureFromMemory(const std::vector<char>& imageData)
{
	BufferObj stagingBuffer;
	TextureObj texture{};
	int32 width, height, channels;
	auto* image = stbi_load_from_memory(reinterpret_cast<const uint8*>(imageData.data()), int32(imageData.size()), &width, &height, &channels, 0);
	auto format = VK_FORMAT_R8G8B8A8_UNORM;
	{
		//VkImage生成
		VkImageCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ci.extent = { uint32(width), uint32(height), 1 };
		ci.format = format;
		ci.imageType = VK_IMAGE_TYPE_2D;
		ci.arrayLayers = 1;
		ci.mipLevels = 1;
		ci.samples = VK_SAMPLE_COUNT_1_BIT;
		ci.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		vkCreateImage(m_vkDevice, &ci, nullptr, &texture.image);

		//メモリ量算出
		VkMemoryRequirements reqs;
		vkGetImageMemoryRequirements(m_vkDevice, texture.image, &reqs);
		VkMemoryAllocateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		info.allocationSize = reqs.size;
		info.memoryTypeIndex = _GetMemoryTypeIndex(reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		//メモリ確保
		vkAllocateMemory(m_vkDevice, &info, nullptr, &texture.memory);
		//メモリバインド
		vkBindImageMemory(m_vkDevice, texture.image, texture.memory, 0);
	}
	{
		uint32 imageSize = width * height * sizeof(uint32);
		//ステージングバッファを用意
		stagingBuffer = _CreateBufferObj(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, image);
	}

	VkBufferImageCopy copyRegion{};
	copyRegion.imageExtent = { uint32(width), uint32(height), 1 };
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

	VkCommandBufferBeginInfo commandBi{};
	commandBi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	vkBeginCommandBuffer(command, &commandBi);
	
	_SetImageMemoryBarrier(command, texture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	vkCmdCopyBufferToImage(command, stagingBuffer.buffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	vkEndCommandBuffer(command);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command;
	vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
	{
		//テクスチャ参照用ビューを生成
		VkImageViewCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ci.image = texture.image;
		ci.format = format;
		ci.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		ci.subresourceRange = {
			VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1
		};
		vkCreateImageView(m_vkDevice, &ci, nullptr, &texture.view);
	}

	vkDeviceWaitIdle(m_vkDevice);
	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &command);

	//ステージングバッファを開放
	vkFreeMemory(m_vkDevice, stagingBuffer.memory, nullptr);
	vkDestroyBuffer(m_vkDevice, stagingBuffer.buffer, nullptr);

	return texture;
}

void ModelApp::
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
	
	default:
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

	default:
		break;
	}

	vkCmdPipelineBarrier(command, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imb);
}
