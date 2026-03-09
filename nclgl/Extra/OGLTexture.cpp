/******************************************************************************
This file is part of the Newcastle OpenGL Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*/////////////////////////////////////////////////////////////////////////////
#include "OGLTexture.h"

#include <filesystem>

// Disabled STB_IMAGE_IMPLEMENTATION because SOIL.lib already includes stb_image
// #define STB_IMAGE_IMPLEMENTATION

#include "./stb/stb_image.h"



OGLTexture::OGLTexture()	{
	glGenTextures(1, &texID);
}

OGLTexture::OGLTexture(GLuint texToOwn) {
	texID = texToOwn;
}

OGLTexture::OGLTexture(const std::string& name) {
	glGenTextures(1, &texID);
}


OGLTexture::~OGLTexture()	{
	glDeleteTextures(1, &texID);
}

UniqueOGLTexture OGLTexture::TextureFromData(char* data, uint32_t width, uint32_t height, uint32_t channels) {
	UniqueOGLTexture tex = std::make_unique<OGLTexture>();
	tex->dimensions = { float(width), float(height) };

	int dataSize = width * height * channels; //This always assumes data is 1 byte per channel

	int sourceType = GL_RGB;

	switch (channels) {
		case 1: sourceType = GL_RED	; break;
		case 2: sourceType = GL_RG	; break;
		case 3: sourceType = GL_RGB	; break;
		case 4: sourceType = GL_RGBA; break;
	}

	glBindTexture(GL_TEXTURE_2D, tex->texID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, sourceType, GL_UNSIGNED_BYTE, data);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	return tex;
}

UniqueOGLTexture OGLTexture::TextureFromFile(const std::string&name) {
	char* texData		= nullptr;
	uint32_t width		= 0;
	uint32_t height		= 0;
	uint32_t channels	= 0;
	uint32_t flags		= 0;

	LoadTexture(name, texData, width, height, channels, flags);  

	UniqueOGLTexture glTex = TextureFromData(texData, width, height, channels);

	free(texData);

	return glTex;
}

UniqueOGLTexture OGLTexture::LoadCubemap(
	const std::string& xPosFile, 
	const std::string& xNegFile, 
	const std::string& yPosFile, 
	const std::string& yNegFile, 
	const std::string& zPosFile, 
	const std::string& zNegFile) {

	const std::string* filenames[6] = {&xPosFile,&xNegFile,&yPosFile,&yNegFile,&zPosFile,&zNegFile};
	uint32_t width[6] = { 0 };
	uint32_t height[6] = { 0 };
	uint32_t channels[6] = { 0 };
	uint32_t flags[6] = { 0 };

	std::vector<char*> texData(6, nullptr);

	for (int i = 0; i < 6; ++i) {
		//TextureLoader::LoadTexture(*filenames[i], texData[i], width[i], height[i], channels[i], flags[i]);
		if (i > 0 && (width[i] != width[0] || height[0] != height[0])) {
			std::cout << __FUNCTION__ << " cubemap input textures don't match in size?\n";
			return nullptr;
		}
	}

	UniqueOGLTexture tex = std::make_unique<OGLTexture>();
	tex->dimensions = { float(width[0]), float(height[0]) };

	glBindTexture(GL_TEXTURE_CUBE_MAP, tex->GetObjectID());

	GLenum type = channels[0] == 4 ? GL_RGBA : GL_RGB;

	for (int i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width[i], height[i], 0, type, GL_UNSIGNED_BYTE, texData[i]);
	}

	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	return tex;
}

bool OGLTexture::LoadTexture(const std::string& filename, char*& outData, uint32_t& width, uint32_t& height, uint32_t& channels, uint32_t& flags) {
	if (filename.empty()) {
		return false;
	}

	std::filesystem::path path(filename);

	std::string extension = path.extension().string();

	bool isAbsolute = path.is_absolute();

	std::string realPath = filename;//  isAbsolute ? filename : Assets::TEXTUREDIR + filename;

	//By default, attempt to use stb image to get this texture
	int stbiWidth = 0;
	int stbiHeight = 0;
	int stbiChannels = 0;

	stbi_uc* texData = stbi_load(realPath.c_str(), &stbiWidth, &stbiHeight, &stbiChannels, 4); //4 forces this to always be rgba!
	width = stbiWidth;
	height = stbiHeight;
	channels = 4; //it gets forced, we don't care about the 'real' channel size

	if (texData) {
		outData = (char*)texData;
		return true;
	}

	return false;
}