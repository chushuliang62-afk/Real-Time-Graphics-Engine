#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Mesh.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include "VoxelTreeTypes.h"
#include <memory>
#include <vector>

// ========================================
// Block Renderer
// ========================================
// Renders voxel blocks using GPU instancing for efficiency
// Supports separate rendering of wood and leaf blocks with different materials
class BlockRenderer {
public:
    BlockRenderer();
    ~BlockRenderer();

    // Initialize renderer (creates cube mesh, loads shaders)
    bool Initialize();

    // Set textures for wood and leaf blocks
    void SetWoodTexture(GLuint albedo, GLuint normal = 0);
    void SetLeafTexture(GLuint albedo, GLuint normal = 0);

    // Render all blocks using instancing
    void RenderBlocks(
        const std::vector<BlockInstance>& blocks,
        Camera* camera,
        Light* light,
        const Matrix4& viewMatrix,
        const Matrix4& projMatrix
    );

    // Render wood blocks only
    void RenderWoodBlocks(
        const std::vector<BlockInstance>& blocks,
        Camera* camera,
        Light* light,
        const Matrix4& viewMatrix,
        const Matrix4& projMatrix
    );

    // Render leaf blocks only
    void RenderLeafBlocks(
        const std::vector<BlockInstance>& blocks,
        Camera* camera,
        Light* light,
        const Matrix4& viewMatrix,
        const Matrix4& projMatrix
    );

    // Get shader (for external use, e.g., shadow pass)
    Shader* GetShader() const { return blockShader.get(); }

private:
    // Cube mesh (shared by all blocks)
    std::unique_ptr<Mesh> cubeMesh;

    // Block shader (supports instancing, lighting, normal mapping)
    std::unique_ptr<Shader> blockShader;

    // Textures
    GLuint woodAlbedo;
    GLuint woodNormal;
    GLuint leafAlbedo;
    GLuint leafNormal;

    // Instance buffer for GPU instancing
    GLuint instanceVBO;
    size_t instanceBufferCapacity;

    // Helper: Create unit cube mesh
    void CreateCubeMesh();

    // Helper: Upload instance data to GPU
    void UploadInstanceData(const std::vector<Matrix4>& modelMatrices);

    // Helper: Draw all instances with a single instanced draw call
    void DrawInstanced(size_t instanceCount);

    // Helper: Build model matrices from block instances
    std::vector<Matrix4> BuildModelMatrices(const std::vector<BlockInstance>& blocks) const;

    // Helper: Filter blocks by type
    std::vector<BlockInstance> FilterByType(const std::vector<BlockInstance>& blocks, BlockType type) const;
};
