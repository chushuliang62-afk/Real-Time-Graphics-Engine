#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Mesh.h"
#include "../nclgl/SceneNode.h"
#include <memory>
#include <vector>

/**
 * TreeSystem - Procedural Tree Generation and Rendering
 *
 * Encapsulated system for creating and rendering trees in the scene.
 * Features:
 * - Procedural tree generation (trunk + foliage)
 * - Configurable tree density and distribution
 * - Automatic placement based on terrain constraints
 * - No hardcoded values - all parameters configurable
 */
class TreeSystem {
public:
    TreeSystem(OGLRenderer* renderer);
    ~TreeSystem();

    // Initialize tree resources and generate trees
    bool Initialize(Shader* sceneShader, GLuint barkTex, GLuint leavesTex);

    // Add trees to a scene node (non-hardcoded placement)
    // baseScale: configurable scale multiplier for tree sizes (e.g., 1.0 = normal, 5.0 = 5x larger)
    void PopulateScene(SceneNode* root, float minX, float maxX, float minZ, float maxZ, int treeCount, float baseScale = 1.0f);

    // Enable/disable tree rendering
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

private:
    // Tree parameter structure (no hardcoding)
    struct TreeParams {
        float trunkHeight;      // Height of trunk
        float trunkRadius;      // Radius of trunk
        float foliageScale;     // Scale of foliage sphere
        float rotation;         // Y-axis rotation
        float leanAngle;        // Slight lean for natural look
        Vector3 leanDirection;  // Direction of lean
    };

    // Reference to renderer
    OGLRenderer* renderer;

    // Enabled flag
    bool enabled;

    // Shared resources for all trees
    std::unique_ptr<Mesh> trunkMesh;    // Quad mesh for trunks
    std::unique_ptr<Mesh> leavesMesh;   // Quad mesh for foliage
    Shader* shader;                      // Reference to scene shader (not owned)
    GLuint barkTexture;
    GLuint leavesTexture;

    // Helper functions
    SceneNode* CreateTree(Vector3 position, const TreeParams& params);

    // Generate random tree parameters within reasonable ranges
    TreeParams GenerateTreeParams(float sizeVariation);
};
