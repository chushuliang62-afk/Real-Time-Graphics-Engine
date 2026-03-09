#include "AnimalManager.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>    // For sqrt
#include <utility>  // For std::move

// #define ANIMAL_VERBOSE
#ifdef ANIMAL_VERBOSE
    #define ANIMAL_LOG(x) std::cout << x << std::endl
#else
    #define ANIMAL_LOG(x)
#endif

AnimalManager::AnimalManager(OGLRenderer* renderer)
    : renderer(renderer), enabled(true), shader(nullptr), scaleMultiplier(1.0f) {
}

AnimalManager::~AnimalManager() {
    // GLTF scenes automatically cleaned up
    // SceneNodes are owned by the scene graph, not by us
    ANIMAL_LOG("AnimalManager: Cleaning up " << instances.size() << " animal instances.");
}

bool AnimalManager::Initialize(Shader* sceneShader) {
    this->shader = sceneShader;

    if (!shader) {
        std::cerr << "AnimalManager: ERROR - No shader provided!" << std::endl;
        return false;
    }

    ANIMAL_LOG("AnimalManager: Initialized successfully.");
    return true;
}

int AnimalManager::RegisterSpecies(const AnimalSpecies& animalSpecies) {
    if (animalSpecies.modelPath.empty()) {
        std::cerr << "AnimalManager: ERROR - Invalid species (empty model path)!" << std::endl;
        return -1;
    }

    ANIMAL_LOG("AnimalManager: Registering species '" << animalSpecies.name
              << "' from " << animalSpecies.modelPath);

    // Load GLTF model for this species
    GLTFScene scene;
    if (!LoadAnimalModel(animalSpecies.modelPath, scene)) {
        std::cerr << "AnimalManager: ERROR - Failed to load model for species '"
                  << animalSpecies.name << "'" << std::endl;
        return -1;
    }

    // Store species configuration and loaded scene
    species.push_back(animalSpecies);
    gltfScenes.push_back(std::move(scene));

    int speciesIndex = static_cast<int>(species.size()) - 1;
    ANIMAL_LOG("AnimalManager: Species '" << animalSpecies.name
              << "' registered successfully (index: " << speciesIndex << ")");
    ANIMAL_LOG("  Meshes: " << gltfScenes[speciesIndex].meshes.size()
              << ", Nodes: " << gltfScenes[speciesIndex].sceneNodes.size());

    return speciesIndex;
}

bool AnimalManager::LoadAnimalModel(const std::string& modelPath, GLTFScene& scene) {
    ANIMAL_LOG("AnimalManager: Loading GLTF model from " << modelPath);

    bool success = GLTFLoader::Load(modelPath, scene);

    if (!success || scene.meshes.empty()) {
        std::cerr << "AnimalManager: ERROR - Failed to load GLTF model: " << modelPath << std::endl;
        return false;
    }

    ANIMAL_LOG("AnimalManager: Model loaded successfully - "
              << scene.meshes.size() << " meshes, "
              << scene.sceneNodes.size() << " nodes");

    return true;
}

void AnimalManager::SpawnAnimals(SceneNode* root, CustomTerrain* terrain,
                                float minX, float maxX, float minZ, float maxZ,
                                int count, int speciesIndex) {
    if (!enabled || !root || !terrain || !shader) {
        std::cerr << "AnimalManager: Cannot spawn - invalid state!" << std::endl;
        return;
    }

    if (species.empty()) {
        std::cerr << "AnimalManager: ERROR - No species registered! Call RegisterSpecies() first." << std::endl;
        return;
    }

    // Validate species index
    if (speciesIndex >= static_cast<int>(species.size())) {
        std::cerr << "AnimalManager: ERROR - Invalid species index " << speciesIndex << std::endl;
        return;
    }

    // Seed random for variation
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(nullptr)) + 54321);  // Different seed from TreeSystem
        seeded = true;
    }

    ANIMAL_LOG("AnimalManager: Spawning " << count << " animals in area: "
              << "X[" << minX << ", " << maxX << "], Z[" << minZ << ", " << maxZ << "]");

    // Calculate area dimensions
    float areaWidth = maxX - minX;
    float areaDepth = maxZ - minZ;

    int successfulSpawns = 0;
    int attempts = 0;
    const int maxAttemptsPerAnimal = 10;  // Avoid infinite loops

    // Spawn animals with terrain-aware placement
    while (successfulSpawns < count && attempts < count * maxAttemptsPerAnimal) {
        attempts++;

        // Random position within bounds
        float x = minX + (static_cast<float>(rand()) / RAND_MAX) * areaWidth;
        float z = minZ + (static_cast<float>(rand()) / RAND_MAX) * areaDepth;

        // Check if position is valid grassland
        if (!IsValidGrasslandPosition(terrain, x, z)) {
            continue;  // Try another position
        }

        // Get terrain height at this position
        float y = GetTerrainHeight(terrain, x, z);

        // Select species (random if speciesIndex is -1)
        int selectedSpecies = speciesIndex;
        if (selectedSpecies == -1) {
            selectedSpecies = rand() % static_cast<int>(species.size());
        }

        // Random scale variation
        const AnimalSpecies& spec = species[selectedSpecies];
        float scaleVariation = 1.0f + (static_cast<float>(rand()) / RAND_MAX - 0.5f) * spec.scaleVariation;
        float finalScale = spec.baseScale * scaleVariation;

        // Random rotation
        float rotation = (static_cast<float>(rand()) / RAND_MAX) * spec.rotationVariation;

        // Create animal instance
        SceneNode* animalNode = CreateAnimalInstance(selectedSpecies,
                                                     Vector3(x, y, z),
                                                     finalScale,
                                                     rotation);

        if (animalNode) {
            root->AddChild(animalNode);

            // Store instance data
            AnimalInstance instance;
            instance.speciesIndex = selectedSpecies;
            instance.position = Vector3(x, y, z);
            instance.scale = finalScale;
            instance.rotation = rotation;
            instance.sceneNode = animalNode;
            instances.push_back(instance);

            successfulSpawns++;
        }
    }

    ANIMAL_LOG("AnimalManager: Successfully spawned " << successfulSpawns
              << " animals (attempts: " << attempts << ")");
}

SceneNode* AnimalManager::CreateAnimalInstance(int speciesIndex, const Vector3& position,
                                              float scale, float rotation) {
    if (speciesIndex < 0 || speciesIndex >= static_cast<int>(species.size())) {
        std::cerr << "AnimalManager: Invalid species index!" << std::endl;
        return nullptr;
    }

    const AnimalSpecies& spec = species[speciesIndex];
    GLTFScene& scene = gltfScenes[speciesIndex];

    if (scene.sceneNodes.empty() || scene.meshes.empty()) {
        std::cerr << "AnimalManager: Invalid GLTF scene for species " << spec.name << std::endl;
        return nullptr;
    }

    // Create root node for this animal instance
    SceneNode* animalRoot = new SceneNode();
    animalRoot->SetTransform(Matrix4::Translation(position) *
                            Matrix4::Rotation(rotation, Vector3(0, 1, 0)) *
                            Matrix4::Scale(Vector3(scale, scale, scale)));
    animalRoot->SetColour(Vector4(1, 1, 1, 1));
    animalRoot->SetShader(shader);

    // Build scene graph from GLTF nodes
    // Strategy: Create SceneNode for each GLTF node, maintaining hierarchy
    std::vector<SceneNode*> nodeMap(scene.sceneNodes.size(), nullptr);

    for (size_t i = 0; i < scene.sceneNodes.size(); ++i) {
        const GLTFNode& gltfNode = scene.sceneNodes[i];

        // Create scene node
        SceneNode* node = nullptr;
        if (gltfNode.mesh) {
            node = new SceneNode(gltfNode.mesh);
        } else {
            node = new SceneNode();
        }

        // Set transform
        node->SetTransform(gltfNode.localMatrix);
        node->SetColour(Vector4(1, 1, 1, 1));
        node->SetShader(shader);

        // Apply albedo texture from GLTF material if available
        if (!scene.materialLayers.empty()) {
            // Use first material layer's albedo texture for all meshes
            const auto& layer = scene.materialLayers[0];
            if (layer.albedo) {
                node->SetTexture(layer.albedo->GetObjectID());
            }
        }

        nodeMap[i] = node;
    }

    // Build hierarchy
    for (size_t i = 0; i < scene.sceneNodes.size(); ++i) {
        const GLTFNode& gltfNode = scene.sceneNodes[i];
        SceneNode* node = nodeMap[i];

        if (gltfNode.parent == -1) {
            // Root node - attach to animal root
            animalRoot->AddChild(node);
        } else {
            // Child node - attach to parent
            if (gltfNode.parent >= 0 && gltfNode.parent < static_cast<int>(nodeMap.size())) {
                nodeMap[gltfNode.parent]->AddChild(node);
            }
        }
    }

    return animalRoot;
}

bool AnimalManager::IsValidGrasslandPosition(CustomTerrain* terrain, float x, float z) const {
    if (!terrain) {
        return false;
    }

    // Get terrain size (2000x2000, coordinates range from 0 to terrainSize)
    Vector3 terrainSize = terrain->GetTerrainSize();

    // Check bounds (terrain coordinates are [0, terrainSize], NOT centered at origin)
    if (x < 0.0f || x > terrainSize.x || z < 0.0f || z > terrainSize.z) {
        return false;
    }

    // Get height at position
    float height = terrain->GetHeightAt(x, z);

    // Based on CustomTerrain.cpp analysis:
    // - Water level: 10 units
    // - Lake basin: -80 to 10 units (center at X=500, Z=400, radius=360)
    // - Lake shore: 10-30 units (grassland/mud)
    // - Flat plain/grassland: 18-30 units (surrounding area and transition zone)
    // - Hill slopes: 20-150 units (transition to mountain)
    // - Mountain peaks: 150-320 units (rocky)
    //
    // Grassland and hill slopes (avoiding water and high mountains):
    const float minValidHeight = 18.0f;   // Above water and lake shore
    const float maxValidHeight = 150.0f;  // Below mountain peaks

    // Additional check: avoid deep lake basin center
    // Lake center is at normalized (0.25, 0.2) = world (500, 400)
    float lakeCenterX = 500.0f;
    float lakeCenterZ = 400.0f;
    float lakeRadius = 360.0f;  // Basin radius from terrain code
    float distToLake = sqrt((x - lakeCenterX) * (x - lakeCenterX) +
                           (z - lakeCenterZ) * (z - lakeCenterZ));

    // Don't spawn in lake basin
    if (distToLake < lakeRadius) {
        return false;
    }

    return (height >= minValidHeight && height <= maxValidHeight);
}

float AnimalManager::GetTerrainHeight(CustomTerrain* terrain, float x, float z) const {
    if (!terrain) {
        return 0.0f;
    }
    return terrain->GetHeightAt(x, z);
}

void AnimalManager::ApplyScaleMultiplier(float multiplier) {
    scaleMultiplier = std::max(0.1f, multiplier);  // Minimum 0.1x to avoid invisibility

    ANIMAL_LOG("AnimalManager: Applying scale multiplier " << scaleMultiplier
              << "x to " << instances.size() << " animals...");

    // Update all animal instances with new scale
    for (auto& instance : instances) {
        if (!instance.sceneNode) continue;

        // Get base scale from species configuration
        const AnimalSpecies& spec = species[instance.speciesIndex];

        // Calculate new scale: original instance scale * global multiplier
        float newScale = instance.scale * scaleMultiplier;

        // Update scene node transform with new scale
        // Preserve position and rotation, only update scale
        instance.sceneNode->SetTransform(
            Matrix4::Translation(instance.position) *
            Matrix4::Rotation(instance.rotation, Vector3(0, 1, 0)) *
            Matrix4::Scale(Vector3(newScale, newScale, newScale))
        );
    }

    ANIMAL_LOG("AnimalManager: Scale adjustment complete. Animals are now "
              << scaleMultiplier << "x their original size.");
}
