#include "HeightMap.h"
#include <iostream>

HeightMap::HeightMap(const std::string& name) {
    int iWidth, iHeight, iChans;
    unsigned char* data = SOIL_load_image(name.c_str(), &iWidth, &iHeight, &iChans, 1);

    if (!data) {
        std::cout << "HeightMap: Failed to load image file: " << name << std::endl;
        return;
    }

    numVertices = iWidth * iHeight;
    numIndices = (iWidth - 1) * (iHeight - 1) * 6;

    vertices = new Vector3[numVertices];
    textureCoords = new Vector2[numVertices];
    indices = new GLuint[numIndices];
    colours = new Vector4[numVertices];
    normals = new Vector3[numVertices];
    tangents = new Vector4[numVertices];

    Vector3 vertexScale = Vector3(16.0f, 1.0f, 16.0f);
    Vector2 textureScale = Vector2(1.0f / 16.0f, 1.0f / 16.0f);

    // Generate vertices
    for (int z = 0; z < iHeight; ++z) {
        for (int x = 0; x < iWidth; ++x) {
            int offset = (z * iWidth) + x;

            // Get height from image data
            float height = data[offset];

            // Set vertex position
            vertices[offset] = Vector3(
                (float)x * vertexScale.x,
                height * vertexScale.y,
                (float)z * vertexScale.z
            );

            // Set texture coordinates
            textureCoords[offset] = Vector2(
                (float)x * textureScale.x,
                (float)z * textureScale.y
            );

            // Set default colour
            colours[offset] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    SOIL_free_image_data(data);

    // Generate indices for triangle strips
    int i = 0;
    for (int z = 0; z < iHeight - 1; ++z) {
        for (int x = 0; x < iWidth - 1; ++x) {
            int a = (z * iWidth) + x;
            int b = (z * iWidth) + (x + 1);
            int c = ((z + 1) * iWidth) + (x + 1);
            int d = ((z + 1) * iWidth) + x;

            // Triangle 1
            indices[i++] = a;
            indices[i++] = c;
            indices[i++] = b;

            // Triangle 2
            indices[i++] = c;
            indices[i++] = a;
            indices[i++] = d;
        }
    }

    // Calculate normals
    for (int i = 0; i < numIndices; i += 3) {
        unsigned int a = indices[i];
        unsigned int b = indices[i + 1];
        unsigned int c = indices[i + 2];

        Vector3 normal = Vector3::Cross(
            vertices[b] - vertices[a],
            vertices[c] - vertices[a]
        );

        normals[a] += normal;
        normals[b] += normal;
        normals[c] += normal;
    }

    // Normalize all normals
    for (unsigned int i = 0; i < numVertices; ++i) {
        normals[i].Normalise();
    }

    // Calculate tangents
    for (unsigned int i = 0; i < numVertices; ++i) {
        Vector3 c = Vector3::Cross(normals[i], Vector3(0, 0, 1));
        if (c.Length() < 0.01f) {
            c = Vector3::Cross(normals[i], Vector3(0, 1, 0));
        }
        c.Normalise();
        tangents[i] = Vector4(c.x, c.y, c.z, 1.0f);
    }

    heightmapSize.x = vertexScale.x * (iWidth - 1);
    heightmapSize.y = vertexScale.y * 255.0f;  // Max height
    heightmapSize.z = vertexScale.z * (iHeight - 1);

    BufferData();

    std::cout << "HeightMap: Loaded terrain " << name << " (" << iWidth << "x" << iHeight << ")" << std::endl;
}
