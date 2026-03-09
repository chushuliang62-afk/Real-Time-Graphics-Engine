#pragma once

#include "VoxelTreeTypes.h"
#include "../nclgl/Vector3.h"
#include <vector>
#include <cmath>

// ========================================
// Terrain Sampler
// ========================================
// Samples terrain to find suitable tree positions
// Only places trees on hillsides, avoiding plains and water
class TerrainSampler {
public:
    TerrainSampler(ITerrainQuery* terrain);
    ~TerrainSampler() = default;

    // Sample terrain to find valid tree positions
    // Returns list of positions where trees can be placed
    std::vector<Vector3> SampleTreePositions(
        int numSamples,         // Number of positions to try
        float minSlope,         // Minimum slope angle (degrees)
        float maxSlope,         // Maximum slope angle (degrees)
        float minSpacing,       // Minimum distance between trees
        unsigned int seed = 0,  // Random seed
        // Optional region filtering (normalized 0-1 coords)
        bool useRegionFilter = false,
        float regionMinX = 0.0f,
        float regionMaxX = 1.0f,
        float regionMinZ = 0.0f,
        float regionMaxZ = 1.0f,
        float regionMinHeight = 0.0f,
        float regionMaxHeight = 1000.0f
    );

    // Check if a single position is valid for tree placement
    bool IsValidTreePosition(
        const Vector3& position,
        float minSlope,
        float maxSlope
    ) const;

    // Get slope angle at position (in degrees)
    float GetSlopeAngle(const Vector3& position) const;

private:
    ITerrainQuery* terrain;

    // Check if position is too close to existing positions
    bool IsTooClose(const Vector3& pos, const std::vector<Vector3>& existing, float minSpacing) const;

    // Random number helpers
    float RandomFloat(unsigned int& seed) const;
    float RandomRange(float min, float max, unsigned int& seed) const;
};
