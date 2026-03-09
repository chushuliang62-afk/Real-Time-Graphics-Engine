#include "BlockRenderer.h"
#include <iostream>
#include <cmath>

// #define BLOCK_VERBOSE
#ifdef BLOCK_VERBOSE
    #define BLOCK_LOG(x) std::cout << x << std::endl
#else
    #define BLOCK_LOG(x)
#endif

BlockRenderer::BlockRenderer()
    : woodAlbedo(0), woodNormal(0), leafAlbedo(0), leafNormal(0),
      instanceVBO(0), instanceBufferCapacity(0) {
}

BlockRenderer::~BlockRenderer() {
    if (instanceVBO != 0) {
        glDeleteBuffers(1, &instanceVBO);
    }
}

bool BlockRenderer::Initialize() {
    BLOCK_LOG("Initializing BlockRenderer...");

    // Create cube mesh
    CreateCubeMesh();

    if (!cubeMesh) {
        std::cerr << "ERROR: Failed to create cube mesh!" << std::endl;
        return false;
    }

    // Load block shader
    blockShader = std::make_unique<Shader>("block.vert", "block.frag");

    if (!blockShader->LoadSuccess()) {
        std::cerr << "ERROR: Failed to load block shader!" << std::endl;
        return false;
    }

    // Create instance buffer
    glGenBuffers(1, &instanceVBO);
    instanceBufferCapacity = 0;

    BLOCK_LOG("BlockRenderer initialized successfully (GPU instanced).");
    return true;
}

void BlockRenderer::SetWoodTexture(GLuint albedo, GLuint normal) {
    woodAlbedo = albedo;
    woodNormal = normal;
}

void BlockRenderer::SetLeafTexture(GLuint albedo, GLuint normal) {
    leafAlbedo = albedo;
    leafNormal = normal;
}

void BlockRenderer::RenderBlocks(
    const std::vector<BlockInstance>& blocks,
    Camera* camera,
    Light* light,
    const Matrix4& viewMatrix,
    const Matrix4& projMatrix
) {
    if (blocks.empty() || !blockShader || !cubeMesh) {
        return;
    }

    // Split by type and render separately
    std::vector<BlockInstance> wood = FilterByType(blocks, BlockType::WOOD);
    std::vector<BlockInstance> leaves = FilterByType(blocks, BlockType::LEAF);

    if (!wood.empty()) {
        RenderWoodBlocks(wood, camera, light, viewMatrix, projMatrix);
    }

    if (!leaves.empty()) {
        RenderLeafBlocks(leaves, camera, light, viewMatrix, projMatrix);
    }
}

void BlockRenderer::RenderWoodBlocks(
    const std::vector<BlockInstance>& blocks,
    Camera* camera,
    Light* light,
    const Matrix4& viewMatrix,
    const Matrix4& projMatrix
) {
    if (blocks.empty() || !blockShader || !cubeMesh) {
        return;
    }

    // Build model matrices
    std::vector<Matrix4> modelMatrices = BuildModelMatrices(blocks);

    // Bind shader
    glUseProgram(blockShader->GetProgram());

    // Set view/projection matrices
    glUniformMatrix4fv(glGetUniformLocation(blockShader->GetProgram(), "viewMatrix"),
        1, false, (float*)&viewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(blockShader->GetProgram(), "projMatrix"),
        1, false, (float*)&projMatrix);

    // Set lighting uniforms
    if (light) {
        glUniform3fv(glGetUniformLocation(blockShader->GetProgram(), "lightPos"),
            1, (float*)&light->GetPosition());
        glUniform4fv(glGetUniformLocation(blockShader->GetProgram(), "lightColour"),
            1, (float*)&light->GetColour());
    }

    // Set camera position
    if (camera) {
        Vector3 camPos = camera->GetPosition();
        glUniform3fv(glGetUniformLocation(blockShader->GetProgram(), "cameraPos"),
            1, (float*)&camPos);
    }

    // Bind wood textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, woodAlbedo);
    glUniform1i(glGetUniformLocation(blockShader->GetProgram(), "diffuseTex"), 0);

    if (woodNormal != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, woodNormal);
        glUniform1i(glGetUniformLocation(blockShader->GetProgram(), "normalTex"), 1);
        glUniform1i(glGetUniformLocation(blockShader->GetProgram(), "useNormalMap"), 1);
    } else {
        glUniform1i(glGetUniformLocation(blockShader->GetProgram(), "useNormalMap"), 0);
    }

    // Upload instance data and draw instanced
    UploadInstanceData(modelMatrices);
    DrawInstanced(modelMatrices.size());

    glUseProgram(0);
}

void BlockRenderer::RenderLeafBlocks(
    const std::vector<BlockInstance>& blocks,
    Camera* camera,
    Light* light,
    const Matrix4& viewMatrix,
    const Matrix4& projMatrix
) {
    if (blocks.empty() || !blockShader || !cubeMesh) {
        return;
    }

    // Build model matrices
    std::vector<Matrix4> modelMatrices = BuildModelMatrices(blocks);

    // Bind shader
    glUseProgram(blockShader->GetProgram());

    // Set view/projection matrices
    glUniformMatrix4fv(glGetUniformLocation(blockShader->GetProgram(), "viewMatrix"),
        1, false, (float*)&viewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(blockShader->GetProgram(), "projMatrix"),
        1, false, (float*)&projMatrix);

    // Set lighting uniforms
    if (light) {
        glUniform3fv(glGetUniformLocation(blockShader->GetProgram(), "lightPos"),
            1, (float*)&light->GetPosition());
        glUniform4fv(glGetUniformLocation(blockShader->GetProgram(), "lightColour"),
            1, (float*)&light->GetColour());
    }

    // Set camera position
    if (camera) {
        Vector3 camPos = camera->GetPosition();
        glUniform3fv(glGetUniformLocation(blockShader->GetProgram(), "cameraPos"),
            1, (float*)&camPos);
    }

    // Bind leaf textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, leafAlbedo);
    glUniform1i(glGetUniformLocation(blockShader->GetProgram(), "diffuseTex"), 0);

    if (leafNormal != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, leafNormal);
        glUniform1i(glGetUniformLocation(blockShader->GetProgram(), "normalTex"), 1);
        glUniform1i(glGetUniformLocation(blockShader->GetProgram(), "useNormalMap"), 1);
    } else {
        glUniform1i(glGetUniformLocation(blockShader->GetProgram(), "useNormalMap"), 0);
    }

    // Upload instance data and draw instanced
    UploadInstanceData(modelMatrices);
    DrawInstanced(modelMatrices.size());

    glUseProgram(0);
}

void BlockRenderer::CreateCubeMesh() {
    // Create a simple unit cube mesh using MeshFromVectors
    std::vector<Vector3> positions = {
        // Front face
        Vector3(-0.5f, -0.5f,  0.5f), Vector3( 0.5f, -0.5f,  0.5f),
        Vector3( 0.5f,  0.5f,  0.5f), Vector3(-0.5f,  0.5f,  0.5f),
        // Back face
        Vector3(-0.5f, -0.5f, -0.5f), Vector3(-0.5f,  0.5f, -0.5f),
        Vector3( 0.5f,  0.5f, -0.5f), Vector3( 0.5f, -0.5f, -0.5f),
        // Top face
        Vector3(-0.5f,  0.5f, -0.5f), Vector3(-0.5f,  0.5f,  0.5f),
        Vector3( 0.5f,  0.5f,  0.5f), Vector3( 0.5f,  0.5f, -0.5f),
        // Bottom face
        Vector3(-0.5f, -0.5f, -0.5f), Vector3( 0.5f, -0.5f, -0.5f),
        Vector3( 0.5f, -0.5f,  0.5f), Vector3(-0.5f, -0.5f,  0.5f),
        // Right face
        Vector3( 0.5f, -0.5f, -0.5f), Vector3( 0.5f,  0.5f, -0.5f),
        Vector3( 0.5f,  0.5f,  0.5f), Vector3( 0.5f, -0.5f,  0.5f),
        // Left face
        Vector3(-0.5f, -0.5f, -0.5f), Vector3(-0.5f, -0.5f,  0.5f),
        Vector3(-0.5f,  0.5f,  0.5f), Vector3(-0.5f,  0.5f, -0.5f)
    };

    std::vector<Vector4> colours(24, Vector4(1, 1, 1, 1));  // White color

    std::vector<Vector2> texCoords = {
        // Front
        Vector2(0, 0), Vector2(1, 0), Vector2(1, 1), Vector2(0, 1),
        // Back
        Vector2(1, 0), Vector2(1, 1), Vector2(0, 1), Vector2(0, 0),
        // Top
        Vector2(0, 1), Vector2(0, 0), Vector2(1, 0), Vector2(1, 1),
        // Bottom
        Vector2(1, 1), Vector2(0, 1), Vector2(0, 0), Vector2(1, 0),
        // Right
        Vector2(1, 0), Vector2(1, 1), Vector2(0, 1), Vector2(0, 0),
        // Left
        Vector2(0, 0), Vector2(1, 0), Vector2(1, 1), Vector2(0, 1)
    };

    std::vector<Vector3> normals = {
        // Front
        Vector3(0, 0, 1), Vector3(0, 0, 1), Vector3(0, 0, 1), Vector3(0, 0, 1),
        // Back
        Vector3(0, 0, -1), Vector3(0, 0, -1), Vector3(0, 0, -1), Vector3(0, 0, -1),
        // Top
        Vector3(0, 1, 0), Vector3(0, 1, 0), Vector3(0, 1, 0), Vector3(0, 1, 0),
        // Bottom
        Vector3(0, -1, 0), Vector3(0, -1, 0), Vector3(0, -1, 0), Vector3(0, -1, 0),
        // Right
        Vector3(1, 0, 0), Vector3(1, 0, 0), Vector3(1, 0, 0), Vector3(1, 0, 0),
        // Left
        Vector3(-1, 0, 0), Vector3(-1, 0, 0), Vector3(-1, 0, 0), Vector3(-1, 0, 0)
    };

    // Calculate tangents for normal mapping
    std::vector<Vector4> tangents(24);
    for (int i = 0; i < 6; ++i) {  // 6 faces
        int baseIdx = i * 4;
        Vector3 edge1 = positions[baseIdx + 1] - positions[baseIdx];
        Vector3 edge2 = positions[baseIdx + 2] - positions[baseIdx];
        Vector2 deltaUV1 = texCoords[baseIdx + 1] - texCoords[baseIdx];
        Vector2 deltaUV2 = texCoords[baseIdx + 2] - texCoords[baseIdx];

        float denom = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
        float f = (fabs(denom) > 1e-6f) ? 1.0f / denom : 0.0f;
        Vector3 tangent;
        tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangent.Normalise();

        for (int j = 0; j < 4; ++j) {
            tangents[baseIdx + j] = Vector4(tangent.x, tangent.y, tangent.z, 1.0f);
        }
    }

    std::vector<unsigned int> indices = {
        0, 1, 2, 0, 2, 3,       // Front
        4, 5, 6, 4, 6, 7,       // Back
        8, 9, 10, 8, 10, 11,    // Top
        12, 13, 14, 12, 14, 15, // Bottom
        16, 17, 18, 16, 18, 19, // Right
        20, 21, 22, 20, 22, 23  // Left
    };

    // Empty skin weights/indices (not used for non-skinned mesh)
    std::vector<Vector4> skinWeights;
    std::vector<Vector4i> skinIndices;
    std::vector<Mesh::SubMesh> meshSetup;

    // Create mesh using MeshFromVectors
    cubeMesh = std::unique_ptr<Mesh>(
        Mesh::MeshFromVectors(positions, colours, texCoords, normals, tangents,
                             skinWeights, skinIndices, indices, meshSetup)
    );
}

void BlockRenderer::UploadInstanceData(const std::vector<Matrix4>& modelMatrices) {
    if (modelMatrices.empty()) return;

    size_t dataSize = modelMatrices.size() * sizeof(Matrix4);

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // Reallocate if buffer is too small
    if (modelMatrices.size() > instanceBufferCapacity) {
        instanceBufferCapacity = modelMatrices.size();
        glBufferData(GL_ARRAY_BUFFER, dataSize, modelMatrices.data(), GL_DYNAMIC_DRAW);
    } else {
        glBufferSubData(GL_ARRAY_BUFFER, 0, dataSize, modelMatrices.data());
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void BlockRenderer::DrawInstanced(size_t instanceCount) {
    if (instanceCount == 0 || !cubeMesh) return;

    // Bind the cube mesh's VAO
    // We access the internal arrayObject via the public Draw mechanism,
    // but we need to set up instance attributes on it.
    // Since Mesh::Draw() binds/unbinds its own VAO, we replicate the bind here.
    GLuint vao = cubeMesh->GetVAO();
    glBindVertexArray(vao);

    // Bind instance buffer and set up per-instance attributes (mat4 = 4 x vec4)
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    for (int col = 0; col < 4; ++col) {
        GLuint attribLoc = 5 + col;  // locations 5, 6, 7, 8
        glEnableVertexAttribArray(attribLoc);
        glVertexAttribPointer(attribLoc, 4, GL_FLOAT, GL_FALSE,
                              sizeof(Matrix4),
                              (void*)(col * sizeof(Vector4)));
        glVertexAttribDivisor(attribLoc, 1);  // Advance per instance
    }

    // Draw all blocks with a single instanced draw call
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, (GLsizei)instanceCount);

    // Clean up: disable instance attributes and reset divisors
    for (int col = 0; col < 4; ++col) {
        GLuint attribLoc = 5 + col;
        glVertexAttribDivisor(attribLoc, 0);
        glDisableVertexAttribArray(attribLoc);
    }

    glBindVertexArray(0);
}

std::vector<Matrix4> BlockRenderer::BuildModelMatrices(const std::vector<BlockInstance>& blocks) const {
    std::vector<Matrix4> matrices;
    matrices.reserve(blocks.size());

    for (const auto& block : blocks) {
        Matrix4 model = Matrix4::Translation(block.position) * Matrix4::Scale(Vector3(block.size, block.size, block.size));
        matrices.push_back(model);
    }

    return matrices;
}

std::vector<BlockInstance> BlockRenderer::FilterByType(
    const std::vector<BlockInstance>& blocks,
    BlockType type
) const {
    std::vector<BlockInstance> filtered;
    for (const auto& block : blocks) {
        if (block.type == type) {
            filtered.push_back(block);
        }
    }
    return filtered;
}
