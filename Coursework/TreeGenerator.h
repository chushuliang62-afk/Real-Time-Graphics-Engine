#pragma once

#include "VoxelTreeTypes.h"
#include "../nclgl/Vector3.h"
#include <cmath>

// ========================================
// Tree Generator
// ========================================
// Procedurally generates Minecraft-style voxel trees
// Creates trunk (wood blocks) + canopy (leaf blocks)
class TreeGenerator {
public:
    TreeGenerator() = default;
    ~TreeGenerator() = default;

    // Generate a complete tree at given position
    // Returns BlockTreeData containing all blocks
    static BlockTreeData GenerateTree(
        const Vector3& rootPosition,
        const TreeConfig& config
    );

    // Generate only the trunk (wood blocks)
    static void GenerateTrunk(
        BlockTreeData& tree,
        int height
    );

    // Generate only the canopy (leaf blocks)
    static void GenerateCanopy(
        BlockTreeData& tree,
        int trunkHeight,
        int canopyRadius,
        int canopyHeight,
        float density,
        bool spherical,
        unsigned int seed
    );

private:
    // Helper: Check if position is within spherical bounds
    static bool IsInSphere(const Vector3& center, const Vector3& pos, float radius);

    // Helper: Random number generation
    static float RandomFloat(unsigned int& seed);
    static int RandomInt(int min, int max, unsigned int& seed);
};
