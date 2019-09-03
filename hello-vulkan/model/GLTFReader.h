#pragma once

#include <GLTFSDK/GLTF.h>
#include <GLTFSDK/GLBResourceReader.h>
#include <GLTFSDK/Deserialize.h>
#include <iostream>
#include <filesystem>
#include <sstream>


class GLTFReader : public Microsoft::glTF::IStreamReader
{
public:
	GLTFReader(std::filesystem::path pathBase);
	~GLTFReader();


public:
	std::shared_ptr<std::istream>
	GetInputStream(const std::string& filename) const override;


private:
	std::filesystem::path m_pathBase;
};
