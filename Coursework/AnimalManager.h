#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Shader.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Extra/GLTFLoader.h"
#include "../nclgl/CustomTerrain.h"
#include <memory>
#include <vector>
#include <string>

/**
 * AnimalManager - GLTF Animal Model Management System
 *
 * Encapsulated system for loading and rendering animated animal models.
 *
 * Features:
 * - Data-driven design: No hardcoded paths or positions
 * - Multiple animal species support (Wolf, Bull, etc.)
 * - Terrain-aware placement: Automatically places animals on grassland
 * - Configurable distribution and density
 * - Integration with existing GLTF rendering pipeline
 *
 * Design Principles (following project architecture):
 * - RAII: Smart pointers for automatic resource management
 * - Encapsulation: All animal logic contained within this class
 * - No hardcoding: All parameters configurable at runtime
 * - Terrain integration: Uses CustomTerrain for height sampling
 */
class AnimalManager {
public:
    // Animal species configuration
    struct AnimalSpecies {
        std::string modelPath;      // Path to GLTF model (e.g., "Wolf.glb")
        std::string name;            // Display name (e.g., "Wolf")
        float baseScale;             // Default scale multiplier
        float scaleVariation;        // Random scale variation (0.0-1.0)
        float rotationVariation;     // Random Y-rotation variation (degrees)
    };

    // Individual animal instance data
    struct AnimalInstance {
        int speciesIndex;            // Index into species array
        Vector3 position;            // World position
        float scale;                 // Instance scale
        float rotation;              // Y-axis rotation
        SceneNode* sceneNode;        // Scene graph node (not owned)
    };

    AnimalManager(OGLRenderer* renderer);
    ~AnimalManager();

    // Initialize animal resources
    // @param sceneShader: Shader for rendering animals (shared with scene)
    bool Initialize(Shader* sceneShader);

    // Register an animal species for spawning
    // @param species: Species configuration
    // @return: Species index for reference
    int RegisterSpecies(const AnimalSpecies& species);

    // Spawn animals in a specified area with terrain awareness
    // @param root: Scene graph root to attach animals to
    // @param terrain: Terrain for height sampling and grass detection
    // @param minX, maxX, minZ, maxZ: Spawn area bounds
    // @param count: Number of animals to spawn
    // @param speciesIndex: Which species to spawn (-1 for random mix)
    void SpawnAnimals(SceneNode* root, CustomTerrain* terrain,
                     float minX, float maxX, float minZ, float maxZ,
                     int count, int speciesIndex = -1);

    // Enable/disable animal rendering
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

    // Get statistics
    int GetSpeciesCount() const { return static_cast<int>(species.size()); }
    int GetInstanceCount() const { return static_cast<int>(instances.size()); }

    // Scale adjustment for era transitions (multiplier applied to all animals)
    void ApplyScaleMultiplier(float multiplier);
    float GetScaleMultiplier() const { return scaleMultiplier; }

private:
    // Reference to renderer
    OGLRenderer* renderer;

    // Enabled flag
    bool enabled;

    // Scene shader (not owned)
    Shader* shader;

    // Registered animal species
    std::vector<AnimalSpecies> species;

    // Loaded GLTF scenes for each species
    std::vector<GLTFScene> gltfScenes;

    // Spawned animal instances
    std::vector<AnimalInstance> instances;

    // Scale multiplier for era transitions (1.0 = normal, 3.0 = 3x larger)
    float scaleMultiplier;

    // Helper functions
    bool LoadAnimalModel(const std::string& modelPath, GLTFScene& scene);
    SceneNode* CreateAnimalInstance(int speciesIndex, const Vector3& position,
                                   float scale, float rotation);
    bool IsValidGrasslandPosition(CustomTerrain* terrain, float x, float z) const;
    float GetTerrainHeight(CustomTerrain* terrain, float x, float z) const;
};
