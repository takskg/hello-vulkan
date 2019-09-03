#include "GLTFReader.h"

GLTFReader::
GLTFReader(std::filesystem::path pathBase)
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
	auto streamPath = m_pathBase / std::filesystem::u8path(filename);
	std::wstringbuf strBuf(streamPath);
	auto stream = std::make_shared<std::wistream>(&strBuf);
	if (!stream)
	{
	}
	return std::reinterpret_pointer_cast<std::istream>(stream);
}
