#pragma once

#include <memory>  // For smart pointers

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/Light.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/CustomTerrain.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Extra/GLTFLoader.h"

// New encapsulated rendering systems
#include "FBOManager.h"
#include "ShadowSystem.h"
#include "GrassRenderer.h"
#include "WaterRenderer.h"
#include "SkeletalAnimator.h"
#include "FPSCounter.h"
#include "ParticleSystem.h"  // Environmental effects (rain/snow)
#include "DayNightCycle.h"   // Day-night lighting system
#include "AnimalManager.h"   // GLTF animal models (Wolf, Bull, etc.)
#include "VoxelTreeSystem.h" // Minecraft-style voxel tree system
#include "CustomTerrainAdapter.h" // Terrain adapter for tree placement
#include "TreeConfig.h" // Configuration loader
#include "DeferredRenderer.h" // Deferred rendering system
#include "LightingConfig.h" // Lighting configuration loader

// Render Pipeline Architecture
#include "RenderPipeline.h"
#include "RenderPass.h"

class Renderer : public OGLRenderer {
public:
    Renderer(Window& parent);
    virtual ~Renderer(void);

    virtual void UpdateScene(float dt);
    virtual void RenderScene();
    virtual void Resize(int x, int y);

protected:
    // ========================================
    // SMART POINTERS (RAII Resource Management)
    // ========================================
    // Using std::unique_ptr for exclusive ownership
    // Automatic cleanup, exception-safe, no manual delete needed

    // Render Pipeline
    std::unique_ptr<RenderPipeline> pipeline;
    std::unique_ptr<RenderContext> context;

    // FPS tracking
    std::unique_ptr<FPSCounter> fpsCounter;

    // Utility functions
    GLuint LoadCubemap(const std::string& dir);
    GLuint LoadCubemapFromCross(const std::string& filename);  // Load from cross-layout PNG
    void SetupCameraTrack();

    // Window reference (for fullscreen toggle, etc.)
    Window& window;

    // Scene objects
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Light> light;

    // Terrain
    std::unique_ptr<HeightMap> heightMap;
    std::unique_ptr<CustomTerrain> customTerrain;  // Custom procedural terrain
    std::unique_ptr<Shader> terrainShader;
    GLuint terrainTex;       // Coast sand rocks (mountain base)
    GLuint terrainSandTex;   // Sand/beach texture
    GLuint terrainRockTex;   // Rock/mountain texture
    GLuint terrainMudTex;    // Brown mud texture (grass land around lake)

    // Skybox
    std::unique_ptr<Mesh> skyboxMesh;
    std::unique_ptr<Shader> skyboxShader;
    GLuint skyboxTex;        // Original rusted skybox (daytime)
    GLuint skyboxNight;      // sky111.png (night/evening)
    GLuint skyboxSunset;     // sky222.png (sunset/sunrise)

    // Scene graph
    std::unique_ptr<SceneNode> root;
    std::unique_ptr<Shader> sceneShader;

    // Frame time
    float frameTime;

    // ========================================
    // FBO System (Encapsulated)
    // ========================================
    std::unique_ptr<FBOManager> fboManager;     // Encapsulated FBO management

    // ========================================
    // Two-Worlds State System
    // ========================================
    bool isModern;              // Current world (false = ancient, true = modern)
    bool isInTransition;        // Whether we're currently transitioning
    float transitionTimer;      // Timer for transition animation (0.0 -> 2.0s)
    float totalTime;            // Total elapsed time for animations

    // SINGLE scene graph with dual resources (NOT two separate graphs!)
    // This is the correct architecture: "same place, different time"

    // ========================================
    // Shadow Mapping System
    // ========================================
    std::unique_ptr<ShadowSystem> shadowSystem;  // Encapsulated shadow management

    // ========================================
    // Grass Animation System
    // ========================================
    std::unique_ptr<GrassRenderer> grassRenderer;  // Encapsulated grass rendering

    // ========================================
    // Water Rendering System
    // ========================================
    std::unique_ptr<WaterRenderer> waterRenderer;  // Encapsulated water rendering

    // TODO: River rendering could also be encapsulated if needed
    std::unique_ptr<Mesh> riverMesh;
    Vector3 riverPosition;
    float riverWidth;
    float riverLength;
    void DrawRiver();  // Keep for now if river is separate from water

    // UNUSED: Helper functions (kept for potential future use)
    // SceneNode* CreatePalmTree(Vector3 position, float scale);
    // void CreateStairsToLake();

    // ========================================
    // GLTF Model Loading System
    // ========================================
    GLTFScene templeScene;          // GLTF scene for Chinese Temple
    std::unique_ptr<Shader> gltfShader;             // Shader for GLTF models

    // Temple textures for manual application
    GLuint templeRoofTex;           // Roof tiles texture
    GLuint templeWallTex;           // Stone wall texture
    GLuint templeWoodTex;           // Dark wood texture (columns)
    GLuint templeFloorTex;          // Stone floor texture

    // Helper functions for GLTF rendering
    void LoadTempleTextures();

    // ========================================
    // Ruins System (Modern Era Replacement)
    // ========================================
    std::unique_ptr<SceneNode> ruinsRoot;           // Root node for all ruin elements
    std::unique_ptr<Mesh> pillarMesh;               // Cylinder mesh for broken pillars
    std::unique_ptr<Mesh> debrisMesh;               // Cube mesh for debris/rubble
    GLuint crackedStoneTex;         // Weathered stone texture for ruins

    // Helper functions for ruins system
    void InitializeRuinsSystem();
    SceneNode* CreateBrokenPillar(Vector3 position, float height, float tiltAngle, float rotation);
    SceneNode* CreateDebris(Vector3 position, Vector3 scale, float rotation);

    // ========================================
    // Skeletal Animation System
    // ========================================
    std::unique_ptr<Mesh> characterMesh;                // Skinned character mesh (Role_T.msh)
    std::unique_ptr<SkeletalAnimator> skeletalAnimator; // Skeletal animation controller
    std::unique_ptr<Shader> skinnedShader;              // Shader for skinned meshes
    GLuint characterTexture;            // Character texture

    // Helper functions for skeletal animation
    void InitializeSkeletalAnimation();

    // ========================================
    // Particle System (Environmental Effects)
    // ========================================
    std::unique_ptr<ParticleSystem> particleSystem;  // Rain/snow particles

    // ========================================
    // Day-Night Cycle (Dynamic Lighting)
    // ========================================
    std::unique_ptr<DayNightCycle> dayNightCycle;  // Day-night lighting system
    std::unique_ptr<Mesh> sunQuad;                  // 3D sphere mesh for sun/moon rendering
    std::unique_ptr<Shader> sunShader;              // Shader for sun/moon

    void DrawSun();  // Render sun/moon in world space

    // ========================================
    // Animal Manager [GLTF Animals - Wolf, Bull, etc.]
    // ========================================
    std::unique_ptr<AnimalManager> animalManager;  // GLTF animal model manager

    // ========================================
    // Voxel Tree System (Procedural Geometry + GPU Instancing)
    // ========================================
    std::unique_ptr<VoxelTreeSystem> voxelTreeSystem;  // Minecraft-style voxel trees
    std::unique_ptr<CustomTerrainAdapter> terrainAdapter;  // Terrain query adapter

    // Tree textures
    GLuint woodAlbedoTex;   // Wood block texture
    GLuint woodNormalTex;   // Wood normal map
    GLuint leafAlbedoTex;   // Leaf block texture
    GLuint leafNormalTex;   // Leaf normal map

    // Tree growth configuration
    float treeGrowthSpeed;  // Growth speed from config

    // Helper functions for tree system
    void InitializeVoxelTreeSystem();
    void LoadTreeTextures();

    // ========================================
    // Deferred Rendering System (Multiple Point Lights)
    // ========================================
    std::unique_ptr<DeferredRenderer> deferredRenderer;  // Deferred rendering pipeline
    std::unique_ptr<Shader> deferredGeometryShader;      // Geometry pass shader
    std::unique_ptr<Shader> deferredLightingShader;      // Lighting pass shader

    // Lighting configuration
    bool useDeferredRendering;  // Enable/disable deferred rendering
    Vector3 ambientColour;       // Ambient light colour
    float ambientIntensity;      // Ambient light intensity

    // Debug visualization
    std::unique_ptr<Shader> debugUnlitShader;  // Simple unlit shader for debugging

    // Helper functions for deferred rendering
    void InitializeDeferredRendering();
};
