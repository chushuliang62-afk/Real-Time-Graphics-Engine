#pragma once

#include "VoxelTreeTypes.h"
#include "TerrainSampler.h"
#include "TreeGenerator.h"
#include "BlockRenderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include <vector>
#include <memory>

// ========================================
// Voxel Tree System
// ========================================
// Main system that manages all voxel trees in the scene
// Handles generation, rendering, and (optionally) growth
class VoxelTreeSystem {
public:
    VoxelTreeSystem(ITerrainQuery* terrain);
    ~VoxelTreeSystem();

    // Initialize system (loads resources, creates renderer)
    bool Initialize();

    // Generate trees on terrain
    void GenerateTrees(
        int count,              // Number of trees to generate
        float minSlope = 10.0f, // Minimum slope (degrees)
        float maxSlope = 45.0f, // Maximum slope (degrees)
        float minSpacing = 15.0f, // Minimum spacing between trees
        unsigned int seed = 12345,
        // Optional region filtering
        bool useRegionFilter = false,
        float regionMinX = 0.0f,
        float regionMaxX = 1.0f,
        float regionMinZ = 0.0f,
        float regionMaxZ = 1.0f,
        float regionMinHeight = 0.0f,
        float regionMaxHeight = 1000.0f
    );

    // Render all trees
    void RenderTrees(
        Camera* camera,
        Light* light,
        const Matrix4& viewMatrix,
        const Matrix4& projMatrix
    );

    // Update system (tree growth)
    void Update(float deltaTime, float worldTime, float growthSpeed = 0.1f);

    // Get tree count
    size_t GetTreeCount() const { return trees.size(); }

    // Get total block count
    size_t GetTotalBlockCount() const;

    // Set textures
    void SetWoodTexture(GLuint albedo, GLuint normal = 0);
    void SetLeafTexture(GLuint albedo, GLuint normal = 0);

    // Set initial growth for all trees
    void SetInitialGrowth(float growthProgress);

    // Set default tree configuration (from config file)
    void SetDefaultConfig(const TreeConfig& config);

    // Clear all trees
    void ClearTrees();

private:
    // Terrain query interface
    ITerrainQuery* terrain;

    // Terrain sampler for finding valid tree positions
    std::unique_ptr<TerrainSampler> sampler;

    // Block renderer (GPU instanced)
    std::unique_ptr<BlockRenderer> renderer;

    // All trees in the system
    std::vector<BlockTreeData> trees;

    // Default tree configuration
    TreeConfig defaultConfig;

    // Growth tracking (moved from static local variables for thread safety)
    float timeSinceLastGrowth = 0.0f;
    int updateCount = 0;
};
