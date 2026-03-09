#include "VoxelTreeSystem.h"
#include <iostream>

// #define TREE_VERBOSE
#ifdef TREE_VERBOSE
    #define TREE_LOG(x) std::cout << x << std::endl
#else
    #define TREE_LOG(x)
#endif

VoxelTreeSystem::VoxelTreeSystem(ITerrainQuery* terrain)
    : terrain(terrain) {
    // Default config will be set from config file via SetDefaultConfig()
    // NO HARDCODING HERE!
}

VoxelTreeSystem::~VoxelTreeSystem() {
}

bool VoxelTreeSystem::Initialize() {
    TREE_LOG("Initializing VoxelTreeSystem...");

    // Create terrain sampler
    if (terrain) {
        sampler = std::make_unique<TerrainSampler>(terrain);
    } else {
        std::cerr << "ERROR: No terrain query interface provided!" << std::endl;
        return false;
    }

    // Create block renderer
    renderer = std::make_unique<BlockRenderer>();
    if (!renderer->Initialize()) {
        std::cerr << "ERROR: Failed to initialize BlockRenderer!" << std::endl;
        return false;
    }

    TREE_LOG("VoxelTreeSystem initialized successfully.");
    return true;
}

void VoxelTreeSystem::GenerateTrees(
    int count,
    float minSlope,
    float maxSlope,
    float minSpacing,
    unsigned int seed,
    bool useRegionFilter,
    float regionMinX,
    float regionMaxX,
    float regionMinZ,
    float regionMaxZ,
    float regionMinHeight,
    float regionMaxHeight
) {
    if (!sampler) {
        std::cerr << "ERROR: TerrainSampler not initialized!" << std::endl;
        return;
    }

    TREE_LOG("Generating " << count << " voxel trees...");

    // Clear existing trees
    trees.clear();

    // Sample valid tree positions from terrain
    std::vector<Vector3> positions = sampler->SampleTreePositions(
        count, minSlope, maxSlope, minSpacing, seed,
        useRegionFilter, regionMinX, regionMaxX, regionMinZ, regionMaxZ,
        regionMinHeight, regionMaxHeight
    );

    TREE_LOG("Found " << positions.size() << " valid tree positions.");

    // Generate a tree at each position
    for (size_t i = 0; i < positions.size(); ++i) {
        TreeConfig config = defaultConfig;
        config.seed = seed + (unsigned int)i;  // Unique seed for each tree

        BlockTreeData tree = TreeGenerator::GenerateTree(positions[i], config);

        // Initialize growth tracking (full size by default)
        // SetInitialGrowth() will be called later to set starting size
        tree.growth.growthProgress = 1.0f;  // Start at 100%, will be adjusted by SetInitialGrowth()
        tree.growth.currentHeight = tree.growth.targetHeight;

        trees.push_back(tree);
    }

    TREE_LOG("Generated " << trees.size() << " trees with "
              << GetTotalBlockCount() << " total blocks.");
}

void VoxelTreeSystem::RenderTrees(
    Camera* camera,
    Light* light,
    const Matrix4& viewMatrix,
    const Matrix4& projMatrix
) {
    if (!renderer || trees.empty()) {
        return;
    }

    // Collect all blocks from all trees
    std::vector<BlockInstance> allBlocks;
    for (const auto& tree : trees) {
        allBlocks.insert(allBlocks.end(), tree.blocks.begin(), tree.blocks.end());
    }

    // Render all blocks in one batch
    renderer->RenderBlocks(allBlocks, camera, light, viewMatrix, projMatrix);
}

void VoxelTreeSystem::Update(float deltaTime, float worldTime, float growthSpeed) {
    // Update tree growth
    timeSinceLastGrowth += deltaTime;
    updateCount++;

    int treesGrowing = 0;
    int treesFullyGrown = 0;

    for (auto& tree : trees) {
        if (tree.growth.growthProgress < 1.0f) {
            treesGrowing++;

            // Grow the tree based on world time
            // growthSpeed = how much to grow per day-night cycle
            float growthIncrement = deltaTime * growthSpeed;
            tree.growth.growthProgress += growthIncrement;

            if (tree.growth.growthProgress > 1.0f) {
                tree.growth.growthProgress = 1.0f;
            }

            // Regenerate tree at new growth stage
            int currentHeight = (int)(tree.growth.targetHeight * tree.growth.growthProgress);
            if (currentHeight != tree.growth.currentHeight) {
                tree.growth.currentHeight = currentHeight;

                // Clear old blocks
                tree.blocks.clear();

                // Regenerate at current growth stage
                if (currentHeight > 0) {
                    TreeGenerator::GenerateTrunk(tree, currentHeight);

                    // Only add canopy if tall enough
                    if (currentHeight >= 3) {
                        int canopyRadius = (int)(tree.config.canopyRadius * tree.growth.growthProgress);
                        int canopyHeight = (int)(tree.config.canopyHeight * tree.growth.growthProgress);

                        if (canopyRadius > 0) {
                            TreeGenerator::GenerateCanopy(
                                tree, currentHeight, canopyRadius, canopyHeight,
                                tree.config.canopyDensity, tree.config.sphericalCanopy,
                                tree.config.seed
                            );
                        }
                    }
                }

                // Log growth progress
                if (treesGrowing == 1) {
                    TREE_LOG("[Tree Growth] Progress: " << (int)(tree.growth.growthProgress * 100.0f)
                              << "%, Height: " << currentHeight << "/" << tree.growth.targetHeight
                              << ", Blocks: " << tree.GetBlockCount());
                }
            }
        } else {
            treesFullyGrown++;
        }
    }

    // Periodic status update (every 5 seconds)
    if (timeSinceLastGrowth >= 5.0f) {
        TREE_LOG("[Tree System] Update #" << updateCount
                  << " - Growing: " << treesGrowing
                  << ", Fully grown: " << treesFullyGrown
                  << ", Total blocks: " << GetTotalBlockCount());
        timeSinceLastGrowth = 0.0f;
    }
}

size_t VoxelTreeSystem::GetTotalBlockCount() const {
    size_t total = 0;
    for (const auto& tree : trees) {
        total += tree.GetBlockCount();
    }
    return total;
}

void VoxelTreeSystem::SetWoodTexture(GLuint albedo, GLuint normal) {
    if (renderer) {
        renderer->SetWoodTexture(albedo, normal);
    }
}

void VoxelTreeSystem::SetLeafTexture(GLuint albedo, GLuint normal) {
    if (renderer) {
        renderer->SetLeafTexture(albedo, normal);
    }
}

void VoxelTreeSystem::SetInitialGrowth(float growthProgress) {
    TREE_LOG("[SetInitialGrowth] Starting - Trees before: " << GetTotalBlockCount() << " blocks");
    TREE_LOG("[SetInitialGrowth] Config - CanopyRadius: " << defaultConfig.canopyRadius
              << ", CanopyHeight: " << defaultConfig.canopyHeight
              << ", BlockSize: " << defaultConfig.blockSize);

    for (size_t i = 0; i < trees.size(); ++i) {
        auto& tree = trees[i];

        tree.growth.growthProgress = growthProgress;
        tree.growth.currentHeight = (int)(tree.growth.targetHeight * growthProgress);

        // Regenerate tree at this growth stage
        tree.blocks.clear();
        if (tree.growth.currentHeight > 0) {
            TreeGenerator::GenerateTrunk(tree, tree.growth.currentHeight);

            if (tree.growth.currentHeight >= 3) {
                int canopyRadius = (int)(tree.config.canopyRadius * growthProgress);
                int canopyHeight = (int)(tree.config.canopyHeight * growthProgress);

                if (canopyRadius > 0) {
                    TreeGenerator::GenerateCanopy(
                        tree, tree.growth.currentHeight, canopyRadius, canopyHeight,
                        tree.config.canopyDensity, tree.config.sphericalCanopy,
                        tree.config.seed
                    );
                }
            }
        }
    }

    TREE_LOG("Set initial growth to " << (growthProgress * 100.0f) << "% for all trees.");
    TREE_LOG("[SetInitialGrowth] Complete - Total blocks: " << GetTotalBlockCount());
}

void VoxelTreeSystem::SetDefaultConfig(const TreeConfig& config) {
    defaultConfig = config;
    TREE_LOG("Default tree config updated: Height=" << config.minHeight << "-" << config.maxHeight
              << ", CanopyRadius=" << config.canopyRadius
              << ", BlockSize=" << config.blockSize);
}

void VoxelTreeSystem::ClearTrees() {
    trees.clear();
    TREE_LOG("All trees cleared.");
}
