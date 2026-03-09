#include "TreeGenerator.h"
#include <cmath>
#include <algorithm>

BlockTreeData TreeGenerator::GenerateTree(
    const Vector3& rootPosition,
    const TreeConfig& config
) {
    BlockTreeData tree;
    tree.rootPosition = rootPosition;
    tree.config = config;

    // Randomize height within range
    unsigned int seed = config.seed;
    int height = RandomInt(config.minHeight, config.maxHeight, seed);

    tree.growth.targetHeight = height;
    tree.growth.currentHeight = height;  // Fully grown for now
    tree.growth.growthProgress = 1.0f;

    // Generate trunk (vertical wood blocks)
    GenerateTrunk(tree, height);

    // Generate canopy (leaf blocks around top of trunk)
    GenerateCanopy(
        tree,
        height,
        config.canopyRadius,
        config.canopyHeight,
        config.canopyDensity,
        config.sphericalCanopy,
        seed
    );

    return tree;
}

void TreeGenerator::GenerateTrunk(
    BlockTreeData& tree,
    int height
) {
    // Create vertical stack of wood blocks
    for (int y = 0; y < height; ++y) {
        Vector3 blockPos = tree.rootPosition + Vector3(0, (float)y * tree.config.blockSize, 0);
        tree.blocks.push_back(BlockInstance(blockPos, BlockType::WOOD, tree.config.blockSize));
    }
}

void TreeGenerator::GenerateCanopy(
    BlockTreeData& tree,
    int trunkHeight,
    int canopyRadius,
    int canopyHeight,
    float density,
    bool spherical,
    unsigned int seed
) {
    // Canopy center is at top of trunk
    Vector3 canopyCenter = tree.rootPosition + Vector3(0, (float)trunkHeight * tree.config.blockSize, 0);

    float blockSize = tree.config.blockSize;
    float radius = (float)canopyRadius * blockSize;

    // Generate leaf blocks in a volume around canopy center
    for (int dy = -canopyRadius; dy <= canopyHeight; ++dy) {
        for (int dx = -canopyRadius; dx <= canopyRadius; ++dx) {
            for (int dz = -canopyRadius; dz <= canopyRadius; ++dz) {
                Vector3 offset((float)dx * blockSize, (float)dy * blockSize, (float)dz * blockSize);
                Vector3 blockPos = canopyCenter + offset;

                // Skip if at trunk position
                if (dx == 0 && dz == 0 && dy <= 0) {
                    continue;  // Don't place leaves where trunk is
                }

                bool inBounds = false;

                if (spherical) {
                    // Spherical canopy
                    inBounds = IsInSphere(canopyCenter, blockPos, radius);
                } else {
                    // Cubic canopy
                    inBounds = (std::abs(dx) <= canopyRadius && std::abs(dz) <= canopyRadius && dy <= canopyHeight);
                }

                if (inBounds) {
                    // Random density check
                    if (RandomFloat(seed) < density) {
                        tree.blocks.push_back(BlockInstance(blockPos, BlockType::LEAF, blockSize));
                    }
                }
            }
        }
    }
}

bool TreeGenerator::IsInSphere(const Vector3& center, const Vector3& pos, float radius) {
    float dx = pos.x - center.x;
    float dy = pos.y - center.y;
    float dz = pos.z - center.z;
    float distSq = dx * dx + dy * dy + dz * dz;
    float radiusSq = radius * radius;
    return distSq <= radiusSq;
}

float TreeGenerator::RandomFloat(unsigned int& seed) {
    // Simple LCG random number generator
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return (float)seed / (float)0x7fffffff;
}

int TreeGenerator::RandomInt(int min, int max, unsigned int& seed) {
    if (min == max) return min;
    float t = RandomFloat(seed);
    return min + (int)(t * (float)(max - min + 1));
}
