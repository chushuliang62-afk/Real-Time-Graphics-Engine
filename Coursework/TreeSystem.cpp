#include "TreeSystem.h"
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>

TreeSystem::TreeSystem(OGLRenderer* renderer)
    : renderer(renderer), enabled(true), shader(nullptr),
      barkTexture(0), leavesTexture(0) {
}

TreeSystem::~TreeSystem() {
    // Meshes are automatically cleaned up by unique_ptr
    // Textures and shader are not owned, don't delete
}

bool TreeSystem::Initialize(Shader* sceneShader, GLuint barkTex, GLuint leavesTex) {
    this->shader = sceneShader;
    this->barkTexture = barkTex;
    this->leavesTexture = leavesTex;

    // Create shared meshes for all trees using simple quads
    // Trunk: Quad mesh scaled vertically (reusable for all tree trunks)
    trunkMesh = std::unique_ptr<Mesh>(Mesh::CreateQuad());
    if (!trunkMesh) {
        std::cerr << "TreeSystem: Failed to create trunk mesh!" << std::endl;
        return false;
    }

    // Foliage: Quad mesh (reusable for all tree canopies)
    leavesMesh = std::unique_ptr<Mesh>(Mesh::CreateQuad());
    if (!leavesMesh) {
        std::cerr << "TreeSystem: Failed to create leaves mesh!" << std::endl;
        return false;
    }

    // TreeSystem initialized
    return true;
}

void TreeSystem::PopulateScene(SceneNode* root, float minX, float maxX, float minZ, float maxZ, int treeCount, float baseScale) {
    if (!enabled || !root || !shader) {
        return;
    }

    // Seed random for variation
    static bool seeded = false;
    if (!seeded) {
        srand(static_cast<unsigned int>(time(nullptr)) + 12345);  // Different seed from other systems
        seeded = true;
    }

    // Generating trees in specified area

    // Calculate area dimensions
    float areaWidth = maxX - minX;
    float areaDepth = maxZ - minZ;

    // Generate trees with non-hardcoded random placement
    for (int i = 0; i < treeCount; ++i) {
        // Random position within bounds
        float x = minX + (static_cast<float>(rand()) / RAND_MAX) * areaWidth;
        float z = minZ + (static_cast<float>(rand()) / RAND_MAX) * areaDepth;
        float y = 0.0f;  // Ground level (adjust based on terrain if needed)

        // Random size variation (0.7 to 1.3 scale)
        float sizeVariation = 0.7f + (static_cast<float>(rand()) / RAND_MAX) * 0.6f;

        // Generate tree parameters with configurable base scale
        TreeParams params = GenerateTreeParams(sizeVariation * baseScale);

        // Create and add tree to scene
        SceneNode* tree = CreateTree(Vector3(x, y, z), params);
        if (tree) {
            root->AddChild(tree);
        }
    }

    // Trees added to scene
}

TreeSystem::TreeParams TreeSystem::GenerateTreeParams(float sizeVariation) {
    TreeParams params;

    // Base parameters (scaled by size variation)
    params.trunkHeight = (8.0f + (static_cast<float>(rand()) / RAND_MAX) * 4.0f) * sizeVariation;  // 8-12 units
    params.trunkRadius = (0.4f + (static_cast<float>(rand()) / RAND_MAX) * 0.3f) * sizeVariation;  // 0.4-0.7 units
    params.foliageScale = (3.5f + (static_cast<float>(rand()) / RAND_MAX) * 2.0f) * sizeVariation; // 3.5-5.5 units

    // Random rotation for variety
    params.rotation = static_cast<float>(rand()) / RAND_MAX * 360.0f;

    // Slight lean for natural look (0-5 degrees)
    params.leanAngle = (static_cast<float>(rand()) / RAND_MAX) * 5.0f;

    // Random lean direction
    float leanDir = (static_cast<float>(rand()) / RAND_MAX) * 2.0f * 3.14159f;
    params.leanDirection = Vector3(cos(leanDir), 0.0f, sin(leanDir));

    return params;
}

SceneNode* TreeSystem::CreateTree(Vector3 position, const TreeParams& params) {
    // Root node for entire tree
    SceneNode* treeRoot = new SceneNode();
    treeRoot->SetTransform(Matrix4::Translation(position) *
                          Matrix4::Rotation(params.rotation, Vector3(0, 1, 0)));
    treeRoot->SetColour(Vector4(1, 1, 1, 1));
    treeRoot->SetTexture(barkTexture);
    treeRoot->SetShader(shader);

    // === TRUNK ===
    SceneNode* trunk = new SceneNode(trunkMesh.get());

    // Apply lean if any
    Matrix4 leanTransform = Matrix4();
    if (params.leanAngle > 0.1f) {
        Vector3 leanAxis = Vector3(params.leanDirection.z, 0, -params.leanDirection.x).Normalised();
        leanTransform = Matrix4::Rotation(params.leanAngle, leanAxis);
    }

    // Position trunk: scale height, position center at half height
    trunk->SetTransform(Matrix4::Translation(Vector3(0, params.trunkHeight * 0.5f, 0)) *
                       leanTransform *
                       Matrix4::Scale(Vector3(params.trunkRadius, params.trunkHeight * 0.5f, params.trunkRadius)));
    trunk->SetColour(Vector4(0.4f, 0.25f, 0.15f, 1.0f));  // Brown color
    trunk->SetTexture(barkTexture);
    trunk->SetShader(shader);

    treeRoot->AddChild(trunk);

    // === FOLIAGE (multiple layers for fuller appearance) ===
    // Main canopy
    SceneNode* foliage1 = new SceneNode(leavesMesh.get());
    foliage1->SetTransform(Matrix4::Translation(Vector3(0, params.trunkHeight * 0.9f, 0)) *
                          leanTransform *
                          Matrix4::Scale(Vector3(params.foliageScale, params.foliageScale, params.foliageScale)));
    foliage1->SetColour(Vector4(0.2f, 0.6f, 0.2f, 1.0f));  // Green color
    foliage1->SetTexture(leavesTexture);
    foliage1->SetShader(shader);

    treeRoot->AddChild(foliage1);

    // Upper canopy (smaller, slightly offset for natural look)
    SceneNode* foliage2 = new SceneNode(leavesMesh.get());
    float upperScale = params.foliageScale * 0.7f;
    foliage2->SetTransform(Matrix4::Translation(Vector3(0, params.trunkHeight * 1.1f, 0)) *
                          leanTransform *
                          Matrix4::Scale(Vector3(upperScale, upperScale, upperScale)));
    foliage2->SetColour(Vector4(0.15f, 0.65f, 0.15f, 1.0f));  // Slightly different green
    foliage2->SetTexture(leavesTexture);
    foliage2->SetShader(shader);

    treeRoot->AddChild(foliage2);

    return treeRoot;
}
