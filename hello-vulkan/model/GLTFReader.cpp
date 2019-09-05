#include "pch.h"
#include "GLTFReader.h"

GLTFReader::
GLTFReader(std::experimental::filesystem::path pathBase)
: m_pathBase(std::move(pathBase))
{
}

GLTFReader::
~GLTFReader()
{
}

std::shared_ptr<std::istream> GLTFReader::
GetInputStream(const std::string& filename) const
{
	auto streamPath = m_pathBase / std::experimental::filesystem::u8path(filename);
	/*std::wstringbuf strBuf(streamPath);
	auto stream = std::make_shared<std::wifstream>(&strBuf, std::ios_base::binary);
	if (!stream)
	{
	}*/
	auto foge = std::make_shared<std::ifstream>(streamPath, std::ios_base::binary);
	return foge;
}
