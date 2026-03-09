#pragma once

#include "../nclgl/Vector3.h"
#include "../nclgl/Matrix4.h"
#include <vector>

// ========================================
// Block Types for Voxel Trees
// ========================================
enum class BlockType {
    WOOD,       // Tree trunk blocks
    LEAF        // Tree leaf blocks
};

// ========================================
// Single Block Instance Data
// ========================================
// Represents a single voxel block in 3D space
struct BlockInstance {
    Vector3 position;       // World position (in grid coordinates)
    BlockType type;         // Wood or Leaf
    float size;             // Block size (typically 1.0)

    BlockInstance() : position(0, 0, 0), type(BlockType::WOOD), size(1.0f) {}
    BlockInstance(const Vector3& pos, BlockType t, float s = 1.0f)
        : position(pos), type(t), size(s) {}
};

// ========================================
// Tree Configuration
// ========================================
// Controls the procedural generation parameters for a tree
struct TreeConfig {
    // Height parameters
    int minHeight = 4;          // Minimum tree trunk height (in blocks)
    int maxHeight = 8;          // Maximum tree trunk height (in blocks)

    // Canopy parameters
    int canopyRadius = 2;       // Leaf canopy radius (in blocks)
    int canopyHeight = 3;       // Leaf canopy height (in blocks)

    // Shape variation
    float canopyDensity = 0.7f; // Probability of a leaf block (0-1)
    bool sphericalCanopy = true; // true = sphere, false = cube

    // Size
    float blockSize = 1.0f;     // Size of each block (world units)

    // Random seed (for reproducible generation)
    unsigned int seed = 0;

    TreeConfig() = default;
};

// ========================================
// Tree Growth State (for future growth system)
// ========================================
struct TreeGrowthState {
    float growthProgress = 1.0f;    // 0.0 = seed, 1.0 = fully grown
    int currentHeight = 0;          // Current trunk height
    int targetHeight = 0;           // Target final height
    float age = 0.0f;               // Tree age in world time units

    TreeGrowthState() = default;
};

// ========================================
// Complete Tree Data
// ========================================
// Holds all blocks that make up a single tree
struct BlockTreeData {
    std::vector<BlockInstance> blocks;  // All blocks (wood + leaves)
    Vector3 rootPosition;               // Base position of the tree
    TreeConfig config;                  // Configuration used to generate
    TreeGrowthState growth;             // Growth state (if using growth system)

    BlockTreeData() : rootPosition(0, 0, 0) {}

    // Get only wood blocks
    std::vector<BlockInstance> GetWoodBlocks() const {
        std::vector<BlockInstance> wood;
        for (const auto& block : blocks) {
            if (block.type == BlockType::WOOD) {
                wood.push_back(block);
            }
        }
        return wood;
    }

    // Get only leaf blocks
    std::vector<BlockInstance> GetLeafBlocks() const {
        std::vector<BlockInstance> leaves;
        for (const auto& block : blocks) {
            if (block.type == BlockType::LEAF) {
                leaves.push_back(block);
            }
        }
        return leaves;
    }

    // Get total block count
    size_t GetBlockCount() const {
        return blocks.size();
    }
};

// ========================================
// Terrain Type Enumeration
// ========================================
enum class TerrainType {
    PLAIN,          // Flat grassland (NO trees)
    GRASSLAND,      // Flat grass (NO trees)
    HILLSIDE,       // Sloped terrain (GOOD for trees)
    MOUNTAIN,       // Steep terrain (MAYBE trees on lower slopes)
    LAKE,           // Water surface (NO trees)
    WATER,          // Water (NO trees)
    UNKNOWN
};

// ========================================
// Terrain Query Interface
// ========================================
// Abstract interface for querying terrain properties
// This allows VoxelTreeSystem to be decoupled from specific terrain implementation
class ITerrainQuery {
public:
    virtual ~ITerrainQuery() = default;

    // Get terrain height at world position (x, z)
    virtual float GetHeightAt(float x, float z) const = 0;

    // Get terrain normal at world position (x, z)
    virtual Vector3 GetNormalAt(float x, float z) const = 0;

    // Get terrain type at world position (x, z)
    virtual TerrainType GetTerrainTypeAt(float x, float z) const = 0;

    // Get terrain bounds (for sampling)
    virtual void GetTerrainBounds(float& minX, float& maxX, float& minZ, float& maxZ) const = 0;
};
