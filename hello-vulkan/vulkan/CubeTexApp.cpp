#include "CubeTexApp.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
}

void CubeTexApp::
cleanup()
{
}

void CubeTexApp::
makeCommand(VkCommandBuffer command)
{
}

CubeTexApp::BufferObj CubeTexApp::
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
