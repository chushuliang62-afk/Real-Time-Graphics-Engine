#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Light.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Mesh.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/CustomTerrain.h"
#include <vector>
#include <memory>

/**
 * GrassRenderer - GPU Instanced Grass Rendering
 *
 * High-performance grass rendering using instancing:
 * - Single draw call for thousands of grass blades
 * - Procedural animation with wind effects
 * - Per-instance variation (rotation, scale, color)
 * - Procedural texture generation in shaders
 */
class GrassRenderer {
public:
    GrassRenderer(OGLRenderer* renderer);
    ~GrassRenderer();

    // Initialize grass system
    bool Initialize(CustomTerrain* heightMap, int patchCount = 5000, int bladesPerPatch = 5);

    // Render grass
    void Render(Light* light, Camera* camera, float time, float windStrength, const Matrix4& viewMatrix, const Matrix4& projMatrix);

    // Enable/disable grass rendering
    void SetEnabled(bool enabled) { this->enabled = enabled; }
    bool IsEnabled() const { return enabled; }

    // Get instance count for statistics
    int GetInstanceCount() const { return grassInstanceCount; }

    // Density control for era transitions (0.0 = no grass, 1.0 = full density)
    void SetDensityMultiplier(float multiplier);
    float GetDensityMultiplier() const { return densityMultiplier; }

private:
    // Reference to renderer
    OGLRenderer* renderer;

    // Enabled flag
    bool enabled;

    // Grass shader (RAII: automatic cleanup)
    std::unique_ptr<Shader> grassShader;

    // Grass texture
    GLuint grassTexture;

    // VAO and VBOs for instanced rendering
    GLuint grassVAO;
    GLuint grassVertexVBO;
    GLuint grassInstanceVBO;

    // Instance data
    struct GrassInstanceData {
        Vector3 position;
        float rotation;
        float scale;
        float randomSeed;
    };
    std::vector<GrassInstanceData> grassInstances;
    int grassInstanceCount;

    // Density control
    float densityMultiplier;  // 1.0 = full density, 0.5 = half density
    int baseInstanceCount;    // Original full instance count

    // Helper functions
    bool LoadGrassShader();
    void GenerateGrassInstances(CustomTerrain* heightMap, int patchCount, int bladesPerPatch);
    void CreateGrassVAO();
};
