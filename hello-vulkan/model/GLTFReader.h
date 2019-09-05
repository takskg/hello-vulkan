#pragma once

#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <GLTFSDK/Deserialize.h>
#include <experimental/filesystem>
#include <sstream>


class GLTFReader : public Microsoft::glTF::IStreamReader
{
public:
	GLTFReader(std::experimental::filesystem::path pathBase);
	~GLTFReader();


public:
	std::shared_ptr<std::istream>
	GetInputStream(const std::string& filename) const override;


private:
	std::experimental::filesystem::path m_pathBase;
};
