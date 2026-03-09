#pragma once

#include "RenderPass.h"
#include "GrassRenderer.h"
#include "WaterRenderer.h"
#include "SkeletalAnimator.h"
#include "ParticleSystem.h"
#include "DayNightCycle.h"
#include "VoxelTreeSystem.h"
#include "../nclgl/Extra/GLTFLoader.h"
// No longer need Renderer.h - OGLRenderer provides all necessary APIs

// ========================================
// SCENE PASS
// ========================================
// Renders all scene geometry to the scene FBO
// This is the main rendering pass that draws terrain, objects, grass, water, etc.

class ScenePass : public ContextualRenderPass {
public:
    ScenePass(RenderContext* context)
        : ContextualRenderPass("ScenePass", context),
          terrainShader(nullptr), skyboxShader(nullptr), sceneShader(nullptr),
          gltfShader(nullptr), skinnedShader(nullptr),
          skyboxMesh(nullptr), customTerrain(nullptr),
          grassRenderer(nullptr), waterRenderer(nullptr), skeletalAnimator(nullptr),
          particleSystem(nullptr), voxelTreeSystem(nullptr),
          templeScene(nullptr), ruinsRoot(nullptr),
          sunShader(nullptr), sunMesh(nullptr), dayNightCycle(nullptr) {}

    virtual ~ScenePass() {}

    // Set required resources (called by Renderer during initialization)
    void SetTerrainResources(Shader* shader, CustomTerrain* terrain,
                            GLuint tex1, GLuint tex2, GLuint tex3, GLuint tex4) {
        terrainShader = shader;
        customTerrain = terrain;
        terrainTex = tex1;
        terrainSandTex = tex2;
        terrainRockTex = tex3;
        terrainMudTex = tex4;
    }

    void SetSkyboxResources(Shader* shader, Mesh* mesh, GLuint daytime, GLuint night, GLuint sunset) {
        skyboxShader = shader;
        skyboxMesh = mesh;
        skyboxDay = daytime;
        skyboxNight = night;
        skyboxSunset = sunset;
    }

    void SetSceneGraphResources(Shader* shader) {
        sceneShader = shader;
    }

    void SetGrassRenderer(GrassRenderer* grass) {
        grassRenderer = grass;
    }

    void SetWaterRenderer(WaterRenderer* water) {
        waterRenderer = water;
    }

    void SetGLTFResources(Shader* shader, GLTFScene* scene,
                         GLuint roofTex, GLuint wallTex, GLuint woodTex, GLuint floorTex) {
        gltfShader = shader;
        templeScene = scene;
        templeRoofTex = roofTex;
        templeWallTex = wallTex;
        templeWoodTex = woodTex;
        templeFloorTex = floorTex;
    }

    void SetRuinsResources(SceneNode* ruins) {
        ruinsRoot = ruins;
    }

    void SetSkeletalResources(Shader* shader, SkeletalAnimator* animator,
                             Mesh* mesh, GLuint texture) {
        skinnedShader = shader;
        skeletalAnimator = animator;
        characterMesh = mesh;
        characterTexture = texture;
    }

    void SetParticleSystem(ParticleSystem* particles) {
        particleSystem = particles;
    }

    void SetSunResources(Shader* shader, Mesh* mesh, DayNightCycle* cycle) {
        sunShader = shader;
        sunMesh = mesh;
        dayNightCycle = cycle;
    }

    void SetVoxelTreeSystem(VoxelTreeSystem* trees) {
        voxelTreeSystem = trees;
    }

    virtual void Execute(OGLRenderer& rendererBase) override;  // Implemented in .cpp

private:
    // Rendering functions (now accept OGLRenderer& - no need for Renderer-specific features)
    void DrawSkybox(OGLRenderer& renderer);
    void DrawTerrain(OGLRenderer& renderer);
    void DrawSceneGraph(OGLRenderer& renderer);
    void DrawGLTFTemple(OGLRenderer& renderer);
    void DrawRuins(OGLRenderer& renderer);
    void DrawSkeletalCharacter(OGLRenderer& renderer);

    // Helper: build node lists (for scene graph)
    void BuildNodeLists(SceneNode* from);
    void SortNodeLists();
    void ClearNodeLists();
    void DrawNode(SceneNode* n, OGLRenderer& renderer);

    // Sun/Moon rendering
    void DrawSun(OGLRenderer& renderer);

    // ========================================
    // Helper: Set common lighting uniforms (DRY principle)
    // ========================================
    // This eliminates the repeated light/camera uniform code that appears multiple times
    void SetLightingUniforms(Shader* shader);

    std::vector<SceneNode*> transparentNodeList;
    std::vector<SceneNode*> nodeList;

    // Resources (NOT owned, just references)
    Shader* terrainShader;
    Shader* skyboxShader;
    Shader* sceneShader;
    Shader* gltfShader;
    Shader* skinnedShader;

    Mesh* skyboxMesh;
    CustomTerrain* customTerrain;
    GLTFScene* templeScene;
    SceneNode* ruinsRoot;
    Mesh* characterMesh;

    GrassRenderer* grassRenderer;
    WaterRenderer* waterRenderer;
    SkeletalAnimator* skeletalAnimator;
    ParticleSystem* particleSystem;
    VoxelTreeSystem* voxelTreeSystem;

    // Textures
    GLuint terrainTex, terrainSandTex, terrainRockTex, terrainMudTex;
    GLuint skyboxDay, skyboxNight, skyboxSunset;  // Dynamic skybox textures
    GLuint currentSkybox;  // Currently active skybox (selected in Execute)
    GLuint templeRoofTex, templeWallTex, templeWoodTex, templeFloorTex;
    GLuint characterTexture;

    // Sun/Moon resources
    Shader* sunShader;
    Mesh* sunMesh;
    DayNightCycle* dayNightCycle;
};
