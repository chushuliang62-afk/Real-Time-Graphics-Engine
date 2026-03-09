/******************************************************************************
This file is part of the Newcastle OpenGL Tutorial Series

Author:Rich Davison
Contact:richgdavison@gmail.com
License: MIT (see LICENSE file at the top of the source tree)
*/////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../OGLRenderer.h"
#include "../Vector2.h"

#include <string>
#include <memory>
	
using UniqueOGLTexture = std::unique_ptr<class OGLTexture>;
using SharedOGLTexture = std::shared_ptr<class OGLTexture>;

class OGLTexture	{
public:
	OGLTexture();
	OGLTexture(GLuint texToOwn);
	OGLTexture(const std::string& name);
	~OGLTexture();

	static UniqueOGLTexture TextureFromData(char* data, uint32_t width, uint32_t height, uint32_t channels);

	static UniqueOGLTexture TextureFromFile(const std::string&name);

	static UniqueOGLTexture LoadCubemap(
		const std::string& xPosFile,
		const std::string& xNegFile,
		const std::string& yPosFile,
		const std::string& yNegFile,
		const std::string& zPosFile,
		const std::string& zNegFile);

	GLuint GetObjectID() const	{
		return texID;
	}


	static bool LoadTexture(const std::string& filename, char*& outData, uint32_t& width, uint32_t& height, uint32_t& channels, uint32_t& flags);


protected:						
	GLuint texID;

	Vector2 dimensions;
};