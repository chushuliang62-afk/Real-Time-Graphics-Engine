#include "Renderer.h"
#include "RenderConfig.h"
#include <algorithm>
#include <iostream>
#include <cmath>

// Render Pipeline includes
#include "ShadowPass.h"
#include "ScenePass.h"
#include "PostProcessPass.h"
#include "DeferredScenePass.h"

// Debug logging - disable for release builds
// #define RENDERER_VERBOSE
#ifdef RENDERER_VERBOSE
    #define LOG_INFO(x) std::cout << x << std::endl
#else
    #define LOG_INFO(x)
#endif
Renderer::Renderer(Window& parent) : OGLRenderer(parent), window(parent) {
    LOG_INFO("=== Renderer Initialization Started ===");
    LOG_INFO("=== Using RAII Smart Pointers ===");

    // ========================================
    // SMART POINTERS: No more manual initialization to nullptr!
    // unique_ptr automatically initializes to nullptr
    // ========================================

    // Initialize non-pointer members
    terrainTex = 0;
    skyboxTex = 0;
    frameTime = 0.0f;

    // Initialize Two-Worlds state
    isModern = false;
    isInTransition = false;
    transitionTimer = 0.0f;
    totalTime = 0.0f;

    // Initialize texture IDs
    crackedStoneTex = 0;
    characterTexture = 0;
    riverWidth = 0.0f;
    riverLength = 0.0f;

    // Initialize voxel tree system texture IDs
    woodAlbedoTex = 0;
    woodNormalTex = 0;
    leafAlbedoTex = 0;
    leafNormalTex = 0;

    // ========================================
    // Create managed resources using std::make_unique
    // Benefits: Exception-safe, automatic cleanup, no memory leaks!
    // ========================================

    // Initialize FPS counter
    LOG_INFO("Creating FPS counter...");
    fpsCounter = std::make_unique<FPSCounter>(&parent);

    // Create camera with initial position (positioned to view the scene)
    LOG_INFO("Creating camera...");
    camera = std::make_unique<Camera>(RenderConfig::CAMERA_INITIAL_PITCH,
                                      RenderConfig::CAMERA_INITIAL_YAW,
                                      Vector3(500.0f, 150.0f, 200.0f));

    // Create custom procedural terrain (flat plain + hill)
    LOG_INFO("Generating custom terrain...");
    customTerrain = std::make_unique<CustomTerrain>(RenderConfig::TERRAIN_GRID_WIDTH,
                                                     RenderConfig::TERRAIN_GRID_HEIGHT,
                                                     RenderConfig::TERRAIN_GRID_SCALE);

    // Create sun light (using DirectionalLight for proper lighting)
    LOG_INFO("Creating light...");
    Vector3 sunDirection = Vector3(RenderConfig::SUN_DIRECTION_X,
                                   RenderConfig::SUN_DIRECTION_Y,
                                   RenderConfig::SUN_DIRECTION_Z).Normalised();
    light = std::make_unique<DirectionalLight>(
        sunDirection,
        Vector4(RenderConfig::SUN_COLOR_R, RenderConfig::SUN_COLOR_G,
                RenderConfig::SUN_COLOR_B, RenderConfig::SUN_COLOR_A)  // Warm sunlight
    );

    // Setup camera track for auto mode (after heightMap is loaded)
    LOG_INFO("Setting up camera track...");
    SetupCameraTrack();

    LOG_INFO("Loading terrain texture (coast sand rocks)...");
    LOG_INFO("  Texture path: " << TEXTUREDIR"coast_sand_rocks_02_diff_1k.jpg");

    // Load coast sand rocks texture (main terrain) - realistic ground texture
    terrainTex = SOIL_load_OGL_texture(
        TEXTUREDIR"coast_sand_rocks_02_diff_1k.jpg",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (!terrainTex) {
        std::cerr << "ERROR: Failed to load terrain texture!" << std::endl;
        std::cerr << "  Attempted path: " << TEXTUREDIR"coast_sand_rocks_02_diff_1k.jpg" << std::endl;
        std::cerr << "  SOIL Error: " << SOIL_last_result() << std::endl;

        // Try alternative path
        LOG_INFO("  Trying alternative path: ../../../Textures/coast_sand_rocks_02_diff_1k.jpg");
        terrainTex = SOIL_load_OGL_texture(
            "../../../Textures/coast_sand_rocks_02_diff_1k.jpg",
            SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
            SOIL_FLAG_MIPMAPS
        );

        if (!terrainTex) {
            std::cerr << "  Alternative path also failed!" << std::endl;
            return;
        } else {
            LOG_INFO("  SUCCESS: Loaded from alternative path!");
        }
    }
    LOG_INFO("Terrain texture (coast sand rocks) loaded successfully.");

    // Load sand texture (beach/low areas) - use Barren Reds as sand
    terrainSandTex = SOIL_load_OGL_texture(
        TEXTUREDIR"Barren Reds.JPG",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (!terrainSandTex) {
        std::cerr << "WARNING: Failed to load sand texture, using grass as fallback" << std::endl;
        terrainSandTex = terrainTex;
    } else {
        LOG_INFO("Terrain sand texture loaded successfully.");
    }

    // Load rock texture (mountains) - use brick as rock
    terrainRockTex = SOIL_load_OGL_texture(
        TEXTUREDIR"brick.tga",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (!terrainRockTex) {
        std::cerr << "WARNING: Failed to load rock texture, using grass as fallback" << std::endl;
        terrainRockTex = terrainTex;
    } else {
        LOG_INFO("Terrain rock texture loaded successfully.");
    }

    // Load brown mud texture (grass land around lake)
    terrainMudTex = SOIL_load_OGL_texture(
        TEXTUREDIR"brown_mud_03_diff_1k.jpg",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (!terrainMudTex) {
        std::cerr << "WARNING: Failed to load mud texture from primary path" << std::endl;
        std::cerr << "  Attempted path: " << TEXTUREDIR"brown_mud_03_diff_1k.jpg" << std::endl;

        // Try alternative path
        LOG_INFO("  Trying alternative path: ../../../Textures/brown_mud_03_diff_1k.jpg");
        terrainMudTex = SOIL_load_OGL_texture(
            "../../../Textures/brown_mud_03_diff_1k.jpg",
            SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
            SOIL_FLAG_MIPMAPS
        );

        if (!terrainMudTex) {
            std::cerr << "  Alternative path also failed! Using main texture as fallback." << std::endl;
            terrainMudTex = terrainTex;
        } else {
            LOG_INFO("  SUCCESS: Mud texture loaded from alternative path!");
        }
    } else {
        LOG_INFO("Terrain mud texture loaded successfully.");
    }

    // Set all terrain textures to repeat
    glBindTexture(GL_TEXTURE_2D, terrainTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, terrainSandTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, terrainRockTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, terrainMudTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Load terrain shader
    LOG_INFO("Loading terrain shader...");
    terrainShader = std::make_unique<Shader>(
        "terrain.vert",
        "terrain.frag"
    );

    if (!terrainShader->LoadSuccess()) {
        std::cerr << "ERROR: Failed to load terrain shader!" << std::endl;
        init = false;
        return;
    }
    LOG_INFO("Terrain shader loaded successfully.");

    // Load skybox
    LOG_INFO("Creating skybox mesh...");
    skyboxMesh = std::unique_ptr<Mesh>(Mesh::CreateQuad());
    LOG_INFO("Loading skybox shader...");
    skyboxShader = std::make_unique<Shader>(
        "skybox.vert",
        "skybox.frag"
    );

    if (!skyboxShader->LoadSuccess()) {
        std::cerr << "ERROR: Failed to load skybox shader!" << std::endl;
        init = false;
        return;
    }
    LOG_INFO("Skybox shader loaded successfully.");

    // Load skybox cubemap textures
    LOG_INFO("Loading skybox cubemaps...");

    // Load daytime skybox (original)
    skyboxTex = LoadCubemap("rusted_");
    if (!skyboxTex) {
        std::cerr << "ERROR: Failed to load daytime skybox cubemap!" << std::endl;
        init = false;
        return;
    }
    LOG_INFO("Daytime skybox loaded successfully.");

    // Load night skybox (sky111.png)
    skyboxNight = LoadCubemapFromCross("sky111.png");
    if (!skyboxNight) {
        std::cerr << "ERROR: Failed to load night skybox cubemap!" << std::endl;
        init = false;
        return;
    }
    LOG_INFO("Night skybox loaded successfully.");

    // Load sunset skybox (sky222.png)
    skyboxSunset = LoadCubemapFromCross("sky222.png");
    if (!skyboxSunset) {
        std::cerr << "ERROR: Failed to load sunset skybox cubemap!" << std::endl;
        init = false;
        return;
    }
    LOG_INFO("Sunset skybox loaded successfully.");

    // Load scene node shader
    LOG_INFO("Loading scene node shader...");
    sceneShader = std::make_unique<Shader>(
        "sceneNode.vert",
        "sceneNode.frag"
    );

    if (!sceneShader->LoadSuccess()) {
        std::cerr << "ERROR: Failed to load scene node shader!" << std::endl;
        init = false;
        return;
    }
    LOG_INFO("Scene node shader loaded successfully.");

    // ========================================
    // Create SINGLE scene graph with DUAL RESOURCES (as recommended in 1.txt)
    // ========================================
    LOG_INFO("Creating unified scene graph (single graph, dual resources)...");
    root = std::make_unique<SceneNode>();

    // Load textures for both time periods
    LOG_INFO("Loading ancient texture (brick.tga)...");
    GLuint ancientTex = SOIL_load_OGL_texture(
        TEXTUREDIR"brick.tga",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (!ancientTex) {
        std::cerr << "ERROR: Failed to load brick.tga! Using terrain texture as fallback." << std::endl;
        ancientTex = terrainTex;
    } else {
        LOG_INFO("Ancient texture loaded successfully (ID: " << ancientTex << ")");
    }

    LOG_INFO("Loading modern texture (brickDOT3.tga)...");
    GLuint modernTex = SOIL_load_OGL_texture(
        TEXTUREDIR"brickDOT3.tga",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (!modernTex) {
        LOG_INFO("WARNING: brickDOT3.tga not found, using terrain texture for modern world");
        modernTex = terrainTex;
    } else {
        LOG_INFO("Modern texture loaded successfully (ID: " << modernTex << ")");
    }

    // Load mesh
    LOG_INFO("Loading Cube.msh for scene objects...");
    Mesh* cubeMesh = Mesh::LoadFromMeshFile("Cube.msh");

    if (!cubeMesh) {
        std::cerr << "ERROR: Failed to load Cube.msh!" << std::endl;
        std::cerr << "  Trying fallback: Sphere.msh..." << std::endl;
        cubeMesh = Mesh::LoadFromMeshFile("Sphere.msh");

        if (!cubeMesh) {
            std::cerr << "ERROR: Fallback Sphere.msh also failed!" << std::endl;
            std::cerr << "  Scene objects will NOT be visible!" << std::endl;
        } else {
            LOG_INFO("SUCCESS: Using Sphere.msh as fallback!");
        }
    } else {
        LOG_INFO("Cube.msh loaded successfully!");
    }

    if (cubeMesh) {
        Vector3 terrainSize = customTerrain->GetTerrainSize();

        // REMOVED: The original 5 floating cubes have been deleted
        // Ruins system now handles all broken pillars in the modern era
        LOG_INFO("Skipping old floating cubes - ruins system will handle modern era pillars.");

        // Trees removed - clean landscape
        LOG_INFO("Scene initialized without trees (clean landscape).");

        // Stairs removed - natural landscape
        // CreateStairsToLake();
    } else {
        std::cerr << "WARNING: Scene will be empty (no mesh available)." << std::endl;
    }

    // OpenGL state setup
    LOG_INFO("Setting up OpenGL state...");
    projMatrix = Matrix4::Perspective(RenderConfig::CAMERA_NEAR_PLANE,
                                     RenderConfig::CAMERA_FAR_PLANE,
        (float)width / (float)height, camera->GetFOV());

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(RenderConfig::BACKGROUND_COLOR_R, RenderConfig::BACKGROUND_COLOR_G,
                 RenderConfig::BACKGROUND_COLOR_B, RenderConfig::BACKGROUND_COLOR_A);

    // Initialize FBO system (Phase 1)
    // ========================================
    // Phase 1: Initialize FBO System (ENCAPSULATED)
    // ========================================
    LOG_INFO("Initializing FBO system...");
    fboManager = std::make_unique<FBOManager>(width, height, this);
    if (!fboManager->Initialize()) {
        std::cerr << "ERROR: Failed to initialize FBO system!" << std::endl;
        return;
    }
    LOG_INFO("FBO system initialized successfully.");

    // ========================================
    // Phase 2: Initialize Shadow System (ENCAPSULATED)
    // ========================================
    LOG_INFO("Initializing Shadow System (Phase 2)...");
    shadowSystem = std::make_unique<ShadowSystem>(RenderConfig::SHADOW_MAP_SIZE);
    if (!shadowSystem->Initialize()) {
        std::cerr << "ERROR: Failed to initialize Shadow System!" << std::endl;
        return;
    }
    LOG_INFO("Shadow system initialized successfully.");

    // ========================================
    // Phase 3: Initialize Grass System (ENCAPSULATED)
    // ========================================
    LOG_INFO("Initializing Grass Animation System (Phase 3)...");
    grassRenderer = std::make_unique<GrassRenderer>(this);
    // Grass patch count x blades per patch = total grass blades (extremely dense coverage)
    if (!grassRenderer->Initialize(customTerrain.get(),
                                   RenderConfig::GRASS_PATCH_COUNT,
                                   RenderConfig::GRASS_BLADES_PER_PATCH)) {
        std::cerr << "ERROR: Failed to initialize Grass Renderer!" << std::endl;
        return;
    }
    LOG_INFO("Grass system initialized successfully.");

    // ========================================
    // Phase 4: Initialize Water System (ENCAPSULATED)
    // ========================================
    LOG_INFO("Initializing Water Rendering System (Phase 4)...");
    waterRenderer = std::make_unique<WaterRenderer>(this);
    // Water body fills the VERY deep basin
    // Basin center: normalized (0.25, 0.2) = world (500, 400)
    // Basin radius: 0.18 * 2000 = 360 units (REDUCED to stay within basin)
    // Water surface height: 10.0 (matches basin edge height)
    // Basin depth: -80 to 10 (90 units VERY deep water body)
    if (!waterRenderer->Initialize(RenderConfig::WATER_SURFACE_HEIGHT,
                                   RenderConfig::WATER_RADIUS)) {
        std::cerr << "ERROR: Failed to initialize Water Renderer!" << std::endl;
        return;
    }
    LOG_INFO("Water system initialized successfully.");

    // ========================================
    // GLTF Model Loading System
    // ========================================
    LOG_INFO("Loading GLTF shader...");
    gltfShader = std::make_unique<Shader>(
        "staticVertex.glsl",
        "TexturedFragment.glsl"
    );

    if (!gltfShader->LoadSuccess()) {
        std::cerr << "ERROR: Failed to load GLTF shader!" << std::endl;
        gltfShader = nullptr;
    } else {
        LOG_INFO("GLTF shader loaded successfully.");
    }

    // Load temple textures first
    LoadTempleTextures();

    LOG_INFO("Loading Chinese Temple GLTF model...");
    bool gltfLoaded = GLTFLoader::Load("Chinese Temple.glb", templeScene);

    if (!gltfLoaded || templeScene.meshes.empty()) {
        std::cerr << "ERROR: Failed to load Chinese Temple.glb!" << std::endl;
        std::cerr << "Make sure Chinese Temple.glb is in the project root directory." << std::endl;
    } else {
        LOG_INFO("Chinese Temple loaded successfully!");
        LOG_INFO("  Meshes: " << templeScene.meshes.size());
        LOG_INFO("  Materials: " << templeScene.materials.size());
        LOG_INFO("  Textures: " << templeScene.textures.size());
        LOG_INFO("  Scene Nodes: " << templeScene.sceneNodes.size());

        // ========================================
        // PERFORMANCE OPTIMIZATION: Pre-process material types
        // ========================================
        // Do string operations ONCE during load, not every frame
        LOG_INFO("Pre-processing GLTF node material types...");
        for (GLTFNode& node : templeScene.sceneNodes) {
            if (!node.mesh) continue;

            // Convert name to lowercase for case-insensitive matching
            std::string nodeName = node.name;
            std::transform(nodeName.begin(), nodeName.end(), nodeName.begin(), ::tolower);

            // Determine material type based on node name
            if (nodeName.find("roof") != std::string::npos ||
                nodeName.find("tile") != std::string::npos) {
                node.materialType = GLTFMaterialType::ROOF;
                LOG_INFO("  [ROOF] " << node.name);
            } else if (nodeName.find("wood") != std::string::npos ||
                       nodeName.find("column") != std::string::npos ||
                       nodeName.find("pillar") != std::string::npos ||
                       nodeName.find("cylinder") != std::string::npos ||  // Pillars are often modeled as cylinders
                       nodeName.find("zhu") != std::string::npos) {  // "zhu" = pillar (Chinese model naming)
                node.materialType = GLTFMaterialType::WOOD;
                LOG_INFO("  [WOOD] " << node.name);
            } else if (nodeName.find("floor") != std::string::npos ||
                       nodeName.find("plane") != std::string::npos) {
                node.materialType = GLTFMaterialType::FLOOR;
                LOG_INFO("  [FLOOR] " << node.name);
            } else if (nodeName.find("wall") != std::string::npos ||
                       nodeName.find("cube") != std::string::npos) {  // Most Cube objects are walls
                node.materialType = GLTFMaterialType::WALL;
                LOG_INFO("  [WALL] " << node.name);
            } else {
                node.materialType = GLTFMaterialType::WALL;  // Default to wall texture
                LOG_INFO("  [WALL-default] " << node.name);
            }
        }
        LOG_INFO("Material types pre-processed successfully.");
    }

    // ========================================
    // Initialize Ruins System (Modern Era)
    // ========================================
    LOG_INFO("Initializing Ruins System (Modern Era replacement)...");
    InitializeRuinsSystem();
    LOG_INFO("Ruins system initialized successfully.");

    // ========================================
    // Initialize Skeletal Animation System (Advanced Feature)
    // ========================================
    LOG_INFO("Initializing Skeletal Animation System...");
    InitializeSkeletalAnimation();
    LOG_INFO("Skeletal Animation system initialized successfully.");

    // ========================================
    // Phase 5: Initialize Particle System [Advanced Feature]
    // ========================================
    LOG_INFO("Initializing Particle System (Environmental Effects)...");
    particleSystem = std::make_unique<ParticleSystem>(this, ParticleType::RAIN);

    // Position emitter to cover camera view
    // Camera track Y ranges from 60-300, looking downward (pitch -5 to -30)
    // Emitter should spawn particles high up and let them fall through camera view
    Vector3 emitterCenter(900.0f, 500.0f, 900.0f);  // High above scene
    Vector3 emitterSize(1500.0f, 800.0f, 1500.0f);   // Large Y range to cover from sky to ground

    if (!particleSystem->Initialize(5000, emitterCenter, emitterSize)) {  // 5000 rain particles
        std::cerr << "WARNING: Failed to initialize Particle System! Continuing without particles..." << std::endl;
        particleSystem.reset();  // Disable particle system but continue initialization
    } else {
        LOG_INFO("Particle system initialized successfully.");
    }

    // ========================================
    // Phase 6: Initialize Day-Night Cycle [Advanced Feature - Dynamic Lighting]
    // ========================================
    LOG_INFO("Initializing 24-Hour Day-Night Cycle System...");
    dayNightCycle = std::make_unique<DayNightCycle>();
    dayNightCycle->SetTimeOfDay(12.0f);  // Start at noon for bright lighting
    dayNightCycle->SetCycleSpeed(0.02f);  // 50 seconds per full cycle
    LOG_INFO("Day-Night Cycle initialized successfully.");
    LOG_INFO("  Cycle speed: 0.02 (50 seconds per 24-hour cycle)");
    LOG_INFO("  Press 1-4 keys to jump to different times:");
    LOG_INFO("    1 = 6:00 AM (Sunrise)");
    LOG_INFO("    2 = 12:00 PM (Noon)");
    LOG_INFO("    3 = 18:00 PM (Sunset)");
    LOG_INFO("    4 = 00:00 AM (Midnight)");

    // ========================================
    // Phase 7: Initialize Sun/Moon Rendering
    // ========================================
    LOG_INFO("Initializing Sun/Moon rendering system...");
    sunQuad = std::unique_ptr<Mesh>(Mesh::CreateSphere(30, 30));
    sunShader = std::make_unique<Shader>("sun.vert", "sun.frag");
    if (!sunShader->LoadSuccess()) {
        std::cerr << "ERROR: Failed to load sun shader!" << std::endl;
    } else {
        LOG_INFO("Sun/Moon shader loaded successfully.");
    }

    // ========================================
    // Phase 8: Initialize Animal Manager [GLTF Animals]
    // ========================================
    LOG_INFO("\n=== Initializing Animal Manager ===");
    animalManager = std::make_unique<AnimalManager>(this);

    if (!animalManager->Initialize(sceneShader.get())) {
        std::cerr << "WARNING: Failed to initialize Animal Manager!" << std::endl;
        animalManager.reset();
    } else {
        LOG_INFO("Animal Manager initialized successfully.");

        // Register Wolf species
        AnimalManager::AnimalSpecies wolf;
        wolf.modelPath = "Wolf.glb";
        wolf.name = "Wolf";
        wolf.baseScale = 10.0f;          // 2x larger (was 5.0f)
        wolf.scaleVariation = 0.3f;      // 30% size variation
        wolf.rotationVariation = 360.0f; // Full rotation variation
        int wolfIndex = animalManager->RegisterSpecies(wolf);

        // Register Bull species
        AnimalManager::AnimalSpecies bull;
        bull.modelPath = "Bull.glb";
        bull.name = "Bull";
        bull.baseScale = 10.0f;          // 2x larger (was 5.0f)
        bull.scaleVariation = 0.2f;      // 20% size variation
        bull.rotationVariation = 360.0f; // Full rotation variation
        int bullIndex = animalManager->RegisterSpecies(bull);

        if (wolfIndex >= 0 && bullIndex >= 0) {
            LOG_INFO("Animals registered successfully:");
            LOG_INFO("  Wolf (index " << wolfIndex << ")");
            LOG_INFO("  Bull (index " << bullIndex << ")");

            // Spawn animals on grassland and hill slopes
            // Based on CustomTerrain.cpp analysis:
            // - Terrain coordinates: [0, 2000] x [0, 2000] (NOT centered at origin!)
            // - Lake basin: center (500, 400), radius 360 - AVOID
            // - Grassland/shore: surrounding lake (height 18-30)
            // - Hill slopes: Z > 800, mountain center (1000, 1500) (height 20-150)
            // - Mountain peaks: avoid (height > 150)
            LOG_INFO("Spawning animals on grassland and hill slopes...");

            // Strategy: Spawn in multiple zones for natural distribution
            // Zone 1: Grassland east of lake (avoiding lake basin)
            animalManager->SpawnAnimals(
                root.get(),           // Scene root
                customTerrain.get(),  // Terrain for height sampling
                900.0f, 1600.0f,      // X range (east side, avoiding lake center at 500)
                200.0f, 700.0f,       // Z range (lower grassland area)
                8,                    // Animals in this zone
                -1                    // -1 = random mix of species
            );

            // Zone 2: Hill slopes (lower part of mountain)
            animalManager->SpawnAnimals(
                root.get(),
                customTerrain.get(),
                700.0f, 1300.0f,      // X range (around hill center X=1000)
                900.0f, 1400.0f,      // Z range (hill area, below peak at Z=1500)
                7,                    // Animals on slopes
                -1
            );

            LOG_INFO("Animals spawned successfully!");
            LOG_INFO("  Total animals: " << animalManager->GetInstanceCount());
        } else {
            std::cerr << "WARNING: Failed to register animal species!" << std::endl;
        }
    }
    LOG_INFO("=== Animal Manager Setup Complete ===");

    // ========================================
    // Initialize Render Pipeline
    // ========================================
    LOG_INFO("\n=== Initializing Render Pipeline ===");

    // ========================================
    // Phase 9: Initialize Voxel Tree System [Advanced Feature - Procedural Geometry]
    // ========================================
    LOG_INFO("\n=== Initializing Voxel Tree System ===");
    InitializeVoxelTreeSystem();
    LOG_INFO("Voxel Tree System initialized successfully.");

    // ========================================
    // Phase 10: Initialize Deferred Rendering System [Advanced Feature - Multiple Point Lights]
    // ========================================
    LOG_INFO("\n=== Initializing Deferred Rendering System ===");
    InitializeDeferredRendering();
    LOG_INFO("Deferred Rendering System initialized successfully.");

    // Create shared rendering context
    context = std::make_unique<RenderContext>();
    context->camera = camera.get();  // Get raw pointer from unique_ptr
    context->light = light.get();
    context->sceneRoot = root.get();
    context->sceneFBO = fboManager->GetSceneFBO();
    context->sceneTexture = fboManager->GetSceneColorTexture();
    context->shadowFBO = shadowSystem->GetShadowFBO();
    context->shadowTexture = shadowSystem->GetShadowTexture();
    context->shadowMatrix = Matrix4();
    context->viewMatrix = Matrix4();
    context->projMatrix = Matrix4();
    context->deltaTime = 0.0f;
    context->totalTime = 0.0f;
    context->isModern = false;
    context->isInTransition = false;
    context->transitionProgress = 0.0f;
    context->screenWidth = width;
    context->screenHeight = height;

    // Create render pipeline
    pipeline = std::make_unique<RenderPipeline>();

    // Pass 1: Shadow Mapping
    LOG_INFO("  Adding ShadowPass...");
    pipeline->AddPass(std::make_unique<ShadowPass>(context.get(), shadowSystem.get()));

    // Pass 2: Scene Rendering (Deferred or Forward)
    if (useDeferredRendering && deferredRenderer) {
        // Use deferred rendering
        LOG_INFO("  Adding DeferredScenePass (deferred rendering enabled)...");
        auto deferredScenePass = std::make_unique<DeferredScenePass>(context.get(), deferredRenderer.get());

        // Set terrain resources
        deferredScenePass->SetTerrainResources(terrainShader.get(), customTerrain.get(),
                                              terrainTex, terrainSandTex, terrainRockTex, terrainMudTex);

        // Set skybox resources
        deferredScenePass->SetSkyboxResources(skyboxShader.get(), skyboxMesh.get(),
                                              skyboxTex, skyboxNight, skyboxSunset);

        // Set other renderers
        deferredScenePass->SetGrassRenderer(grassRenderer.get());
        deferredScenePass->SetWaterRenderer(waterRenderer.get());

        // Set scene graph resources
        deferredScenePass->SetSceneGraphResources(sceneShader.get());

        // Set GLTF resources
        deferredScenePass->SetGLTFResources(gltfShader.get(), &templeScene,
                                           templeRoofTex, templeWallTex, templeWoodTex, templeFloorTex);

        // Set ruins resources
        deferredScenePass->SetRuinsResources(ruinsRoot.get());

        // Set skeletal animation resources
        deferredScenePass->SetSkeletalResources(skinnedShader.get(), skeletalAnimator.get(),
                                               characterMesh.get(), characterTexture);

        // Set sun resources
        deferredScenePass->SetSunResources(sunShader.get(), sunQuad.get(), dayNightCycle.get());

        // Set voxel tree system
        deferredScenePass->SetVoxelTreeSystem(voxelTreeSystem.get());

        // Set terrain for height sampling (skeletal character needs it)
        deferredScenePass->SetCustomTerrain(customTerrain.get());

        // Add to pipeline
        pipeline->AddPass(std::move(deferredScenePass));
    } else {
        // Use forward rendering (original ScenePass)
        LOG_INFO("  Adding ScenePass (forward rendering)...");
        auto scenePass = std::make_unique<ScenePass>(context.get());  // Pass raw pointer from context
        scenePass->SetTerrainResources(terrainShader.get(), customTerrain.get(),
                                       terrainTex, terrainSandTex, terrainRockTex, terrainMudTex);
        scenePass->SetSkyboxResources(skyboxShader.get(), skyboxMesh.get(), skyboxTex, skyboxNight, skyboxSunset);
        scenePass->SetSceneGraphResources(sceneShader.get());
        scenePass->SetGrassRenderer(grassRenderer.get());
        scenePass->SetWaterRenderer(waterRenderer.get());
        scenePass->SetGLTFResources(gltfShader.get(), &templeScene,
                                   templeRoofTex, templeWallTex, templeWoodTex, templeFloorTex);
        scenePass->SetRuinsResources(ruinsRoot.get());
        scenePass->SetSkeletalResources(skinnedShader.get(), skeletalAnimator.get(),
                                       characterMesh.get(), characterTexture);
        scenePass->SetParticleSystem(particleSystem.get());
        scenePass->SetSunResources(sunShader.get(), sunQuad.get(), dayNightCycle.get());
        scenePass->SetVoxelTreeSystem(voxelTreeSystem.get());
        pipeline->AddPass(std::move(scenePass));
    }

    // Pass 3: Post-Processing
    LOG_INFO("  Adding PostProcessPass...");
    auto postProcessPass = std::make_unique<PostProcessPass>(context.get(), fboManager.get());
    postProcessPass->SetParticleSystem(particleSystem.get());
    pipeline->AddPass(std::move(postProcessPass));

    // Initialize all passes
    pipeline->Initialize(*this);

    // Print pipeline configuration
    pipeline->PrintPipeline();
    LOG_INFO("=== Render Pipeline Initialized ===");

    init = true;
    LOG_INFO("\n=== Renderer Initialization Complete! ===");
    LOG_INFO("\n=== CONTROLS ===");
    LOG_INFO("F   - Toggle camera mode (AUTO waypoint track / MANUAL FPS)");
    LOG_INFO("T   - Trigger time jump (Ancient <-> Modern)");
    LOG_INFO("F5  - Reload all shaders");
    LOG_INFO("ESC - Exit program");
    LOG_INFO("\nMANUAL MODE CONTROLS:");
    LOG_INFO("W/A/S/D     - Move camera");
    LOG_INFO("SHIFT/SPACE - Move up/down");
    LOG_INFO("MOUSE       - Look around");
    LOG_INFO("\nStarting in AUTO mode - camera will follow cinematic track!");
    LOG_INFO("=====================================\n");

    // Print rendering summary
    LOG_INFO("\n=== RENDERING SYSTEMS SUMMARY ===");
    LOG_INFO("Terrain size: " << customTerrain->GetTerrainSize().x << "x" << customTerrain->GetTerrainSize().z);
    LOG_INFO("Grass renderer: " << (grassRenderer ? "ACTIVE" : "DISABLED"));
    LOG_INFO("Water renderer: " << (waterRenderer ? "ACTIVE" : "DISABLED"));
    LOG_INFO("Shadow system: " << (shadowSystem ? "ACTIVE" : "DISABLED"));
    LOG_INFO("==================================\n");
}

Renderer::~Renderer(void) {
    // ========================================
    // SMART POINTERS: Automatic cleanup with RAII
    // ========================================
    // All unique_ptr members automatically delete their resources
    // No manual delete needed for:
    //   camera, light, customTerrain, skyboxMesh, root,
    //   terrainShader, skyboxShader, sceneShader, gltfShader,
    //   fboManager, shadowSystem, grassRenderer, waterRenderer, fpsCounter,
    //   skeletalAnimator, characterMesh, skinnedShader, riverMesh,
    //   ruinsRoot, pillarMesh, debrisMesh, pipeline, context
    //
    // Benefits:
    //   1. Exception-safe: If constructor throws, already-constructed members are cleaned up
    //   2. Correct order: Destructors called in reverse order of construction
    //   3. No memory leaks: Impossible to forget a delete
    //   4. No double-free: unique_ptr prevents copying

    // Only need to manually cleanup OpenGL textures (they're just GLuint IDs)
    glDeleteTextures(1, &terrainTex);
    glDeleteTextures(1, &terrainSandTex);
    glDeleteTextures(1, &terrainRockTex);
    glDeleteTextures(1, &terrainMudTex);
    glDeleteTextures(1, &skyboxTex);
    glDeleteTextures(1, &characterTexture);
    glDeleteTextures(1, &crackedStoneTex);
    glDeleteTextures(1, &templeRoofTex);
    glDeleteTextures(1, &templeWallTex);
    glDeleteTextures(1, &templeWoodTex);
    glDeleteTextures(1, &templeFloorTex);

    LOG_INFO("Renderer cleaned up successfully (RAII in action!).");
}

void Renderer::UpdateScene(float dt) {
    // ========================================
    // FPS Counter
    // ========================================
    fpsCounter->Update(dt);

    // Toggle camera mode with F key (skip first few frames to avoid initialization issues)
    if (fpsCounter->GetFrameCount() > 10 && Window::GetKeyboard()->KeyTriggered(KEYBOARD_F)) {
        camera->ToggleMode();
        LOG_INFO("Camera mode: " << (camera->IsAutoMode() ? "AUTO (waypoint track)" : "MANUAL (FPS)"));
    }

    // Toggle fullscreen mode with F11 key
    if (fpsCounter->GetFrameCount() > 10 && Window::GetKeyboard()->KeyTriggered(KEYBOARD_F11)) {
        window.ToggleFullscreen();
    }

    // Update camera (handles both auto and manual modes internally)
    camera->UpdateCamera(dt);
    frameTime = dt;

    // Update view and projection matrices based on camera state
    // (viewMatrix for position/rotation, projMatrix for FOV changes)
    viewMatrix = camera->BuildViewMatrix();
    projMatrix = Matrix4::Perspective(
        RenderConfig::CAMERA_NEAR_PLANE,
        RenderConfig::CAMERA_FAR_PLANE,
        (float)width / (float)height,
        camera->GetFOV()
    );

    // Update total time for animations
    totalTime += dt;

    // Handle time jump with T key (Phase 1: T1.2) - also skip first few frames
    if (fpsCounter->GetFrameCount() > 10 && Window::GetKeyboard()->KeyTriggered(KEYBOARD_T)) {
        if (!isInTransition) {
            isInTransition = true;
            transitionTimer = 0.0f;
            LOG_INFO("Time jump initiated! Transitioning to " << (isModern ? "Ancient" : "Modern") << " era...");
        }
    }

    // Update transition timer
    if (isInTransition) {
        transitionTimer += dt;

        // Debug output every 0.5 seconds
        static float lastPrintTime = 0.0f;
        if (transitionTimer - lastPrintTime >= 0.5f) {
            LOG_INFO("  Transition progress: " << (transitionTimer / 2.0f * 100.0f) << "%");
            lastPrintTime = transitionTimer;
        }

        // Transition duration is 2 seconds
        if (transitionTimer >= 2.0f) {
            isInTransition = false;
            isModern = !isModern;  // Toggle between ancient and modern

            // Switch all scene nodes to new world state
            root->UpdateWorldState(isModern);

            // ========================================
            // ERA TRANSITION EFFECTS (No hardcoding!)
            // ========================================
            if (isModern) {
                // Modern era: Environmental degradation
                LOG_INFO("\n=== TRANSITIONING TO MODERN ERA ===");

                // Reduce grass density (deforestation/urbanization effect)
                if (grassRenderer) {
                    grassRenderer->SetDensityMultiplier(0.5f);  // 50% grass remains
                }

                // Animals grow larger (perhaps due to genetic modification or survivors)
                if (animalManager) {
                    animalManager->ApplyScaleMultiplier(3.0f);  // 3x larger
                }

                LOG_INFO("Modern era effects applied:");
                LOG_INFO("  - Grass reduced to 50% (environmental impact)");
                LOG_INFO("  - Animals scaled to 3x size (adaptation/mutation)");
                LOG_INFO("==================================\n");
            } else {
                // Ancient era: Pristine nature restored
                LOG_INFO("\n=== TRANSITIONING TO ANCIENT ERA ===");

                // Restore full grass density
                if (grassRenderer) {
                    grassRenderer->SetDensityMultiplier(1.0f);  // 100% grass
                }

                // Restore original animal sizes
                if (animalManager) {
                    animalManager->ApplyScaleMultiplier(1.0f);  // Normal size
                }

                LOG_INFO("Ancient era effects applied:");
                LOG_INFO("  - Grass restored to 100% (pristine nature)");
                LOG_INFO("  - Animals restored to normal size");
                LOG_INFO("==================================\n");
            }

            LOG_INFO("Time jump complete! Now in " << (isModern ? "Modern" : "Ancient") << " era.");
            lastPrintTime = 0.0f; // Reset for next transition
        }
    }

    // Update scene graph
    viewMatrix = camera->BuildViewMatrix();

    // Update the single unified scene graph
    root->Update(dt);

    // ========================================
    // Update Day-Night Cycle (Dynamic Lighting)
    // ========================================
    // ========================================
    // Update Voxel Tree System (Growth)
    // ========================================
    if (voxelTreeSystem && dayNightCycle) {
        float worldTime = dayNightCycle->GetTimeOfDay();
        voxelTreeSystem->Update(dt, worldTime, treeGrowthSpeed);
    }

    // ========================================
    // Update Deferred Rendering System (Light Animations)
    // ========================================
    if (deferredRenderer) {
        deferredRenderer->UpdateLights(dt);
    }

    if (dayNightCycle) {
        // Update the cycle
        dayNightCycle->Update(dt);

        // Apply sun direction and color to directional light
        DirectionalLight* dirLight = dynamic_cast<DirectionalLight*>(light.get());
        if (dirLight) {
            Vector3 sunDir = dayNightCycle->GetSunDirection();
            dirLight->SetDirection(sunDir);

            Vector4 sunColor = dayNightCycle->GetSunColor();
            dirLight->SetColour(sunColor);
        }

        // Time jump shortcuts (1-4 keys)
        if (fpsCounter->GetFrameCount() > 10) {
            if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_1)) {
                dayNightCycle->SetTimeOfDay(6.0f);  // Sunrise
                LOG_INFO("Time set to: 6:00 AM (Sunrise)");
            }
            else if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_2)) {
                dayNightCycle->SetTimeOfDay(12.0f);  // Noon
                LOG_INFO("Time set to: 12:00 PM (Noon)");
            }
            else if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_3)) {
                dayNightCycle->SetTimeOfDay(18.0f);  // Sunset
                LOG_INFO("Time set to: 18:00 PM (Sunset)");
            }
            else if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_4)) {
                dayNightCycle->SetTimeOfDay(0.0f);  // Midnight
                LOG_INFO("Time set to: 00:00 AM (Midnight)");
            }

            // Speed control: = to speed up, - to slow down
            if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_PLUS)) {  // = key (same as + without shift)
                float currentSpeed = dayNightCycle->GetCycleSpeed();
                float newSpeed = currentSpeed * 2.0f;
                if (newSpeed > 10.0f) newSpeed = 10.0f;  // Max 10x speed
                dayNightCycle->SetCycleSpeed(newSpeed);
                LOG_INFO("Day-Night cycle speed: " << newSpeed << "x (faster)");
            }
            else if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_MINUS)) {
                float currentSpeed = dayNightCycle->GetCycleSpeed();
                float newSpeed = currentSpeed * 0.5f;
                if (newSpeed < 0.005f) newSpeed = 0.005f;  // Min 0.005x speed
                dayNightCycle->SetCycleSpeed(newSpeed);
                LOG_INFO("Day-Night cycle speed: " << newSpeed << "x (slower)");
            }
        }
    }

    // ========================================
    // Update Particle System (P0-1: Environmental Effects)
    // ========================================
    if (particleSystem) {
        // Toggle particles with R key (Rain)
        if (fpsCounter->GetFrameCount() > 10 && Window::GetKeyboard()->KeyTriggered(KEYBOARD_R)) {
            particleSystem->ToggleEnabled();
            LOG_INFO("Particle System (Rain): " << (particleSystem->IsEnabled() ? "ENABLED" : "DISABLED"));
        }

        particleSystem->Update(dt);
    }
}

void Renderer::RenderScene() {
    // ========================================
    // Simple Pipeline Execution
    // ========================================
    // Renderer no longer knows rendering details
    // All rendering logic is now encapsulated in RenderPass classes

    // Update shared rendering context
    context->viewMatrix = viewMatrix;
    context->projMatrix = projMatrix;
    context->deltaTime = frameTime;
    context->totalTime = totalTime;
    context->isModern = isModern;
    context->isInTransition = isInTransition;
    context->transitionProgress = isInTransition ? (transitionTimer / 2.0f) : 0.0f;

    // Execute entire render pipeline (ShadowPass -> ScenePass -> PostProcessPass)
    pipeline->Execute(*this);

    // Draw sun/moon after scene rendering (before post-processing)
    // This needs to be done outside pipeline because sun uses world-space positioning
    if (dayNightCycle && sunQuad && sunShader) {
        // Bind scene FBO to render sun into scene texture
        glBindFramebuffer(GL_FRAMEBUFFER, context->sceneFBO);
        glViewport(0, 0, context->screenWidth, context->screenHeight);

        DrawSun();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Note: Deferred rendering visualization now happens in DeferredScenePass
    // Light positions are visible through their lighting effects on the terrain
}

void Renderer::Resize(int x, int y) {
    // Call base class implementation to update width, height, and viewport
    OGLRenderer::Resize(x, y);

    // Update render context with new screen dimensions
    if (context) {
        context->screenWidth = width;
        context->screenHeight = height;
    }

    // Update projection matrix for new aspect ratio (use camera's FOV)
    projMatrix = Matrix4::Perspective(RenderConfig::CAMERA_NEAR_PLANE,
                                     RenderConfig::CAMERA_FAR_PLANE,
        (float)width / (float)height, camera->GetFOV());
}

// DELETED: DrawSkybox, DrawTerrain, DrawNodes, DrawNode, BuildNodeLists, SortNodeLists, ClearNodeLists
// These functions are now in ScenePass.cpp (moved to render pipeline architecture)

GLuint Renderer::LoadCubemap(const std::string& basename) {
    std::string faces[6] = {
        TEXTUREDIR + basename + "west.jpg",  // Right
        TEXTUREDIR + basename + "east.jpg",  // Left
        TEXTUREDIR + basename + "up.jpg",    // Top
        TEXTUREDIR + basename + "down.jpg",  // Bottom
        TEXTUREDIR + basename + "south.jpg", // Front
        TEXTUREDIR + basename + "north.jpg"  // Back
    };

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    for (int i = 0; i < 6; ++i) {
        int width, height, channels;
        unsigned char* data = SOIL_load_image(
            faces[i].c_str(),
            &width, &height, &channels,
            SOIL_LOAD_RGB
        );

        if (data) {
            glTexImage2D(
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGB, width, height, 0,
                GL_RGB, GL_UNSIGNED_BYTE, data
            );
            SOIL_free_image_data(data);
        }
        else {
            glDeleteTextures(1, &texID);
            return 0;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return texID;
}

// Load cubemap from cross-layout PNG (horizontal cross format)
GLuint Renderer::LoadCubemapFromCross(const std::string& filename) {
    std::string fullPath = TEXTUREDIR + filename;

    // Load the cross-layout image
    int width, height, channels;
    unsigned char* data = SOIL_load_image(
        fullPath.c_str(),
        &width, &height, &channels,
        SOIL_LOAD_RGBA
    );

    if (!data) {
        std::cerr << "Failed to load cubemap cross image: " << fullPath << std::endl;
        return 0;
    }

    LOG_INFO("Loading cubemap from cross layout: " << filename);
    LOG_INFO("  Image size: " << width << "x" << height);

    // Calculate face size (cross layout is 4x3, each face is width/4 by height/3)
    int faceWidth = width / 4;
    int faceHeight = height / 3;

    LOG_INFO("  Face size: " << faceWidth << "x" << faceHeight);

    // Create cubemap texture
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

    // Cross layout positions (x, y) in the image grid:
    // Row 0: [empty] [top] [empty] [empty]
    // Row 1: [left] [front] [right] [back]
    // Row 2: [empty] [bottom] [empty] [empty]

    struct FaceInfo {
        GLenum target;
        int gridX, gridY;
    };

    FaceInfo faces[6] = {
        { GL_TEXTURE_CUBE_MAP_POSITIVE_X, 2, 1 },  // Right
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, 1 },  // Left
        { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 1, 0 },  // Top
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 1, 2 },  // Bottom
        { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 1, 1 },  // Front
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 3, 1 }   // Back
    };

    // Extract each face and upload to GPU
    for (int i = 0; i < 6; ++i) {
        // Allocate memory for this face
        unsigned char* faceData = new unsigned char[faceWidth * faceHeight * 4];

        // Extract face from cross layout
        int startX = faces[i].gridX * faceWidth;
        int startY = faces[i].gridY * faceHeight;

        for (int y = 0; y < faceHeight; ++y) {
            for (int x = 0; x < faceWidth; ++x) {
                int srcIndex = ((startY + y) * width + (startX + x)) * 4;
                int dstIndex = (y * faceWidth + x) * 4;

                faceData[dstIndex + 0] = data[srcIndex + 0];  // R
                faceData[dstIndex + 1] = data[srcIndex + 1];  // G
                faceData[dstIndex + 2] = data[srcIndex + 2];  // B
                faceData[dstIndex + 3] = data[srcIndex + 3];  // A
            }
        }

        // Upload face to GPU
        glTexImage2D(
            faces[i].target,
            0, GL_RGBA, faceWidth, faceHeight, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, faceData
        );

        delete[] faceData;
    }

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    SOIL_free_image_data(data);
    LOG_INFO("Cubemap loaded successfully from cross layout!");

    return texID;
}

void Renderer::SetupCameraTrack() {
    // Define a camera track that shows off the custom terrain
    // Terrain is 2000x2000, hill is in the upper portion
    Vector3 terrainSize = customTerrain->GetTerrainSize();

    // Clear any existing waypoints
    camera->ClearWaypoints();

    // ========================================
    // Cinematic Camera Track - Showcases all features
    // ========================================

    // Waypoint 1: Opening shot - Overview of the entire scene from high above
    // Shows terrain, lake, hill, temple in one sweeping view
    camera->AddWaypoint(CameraWaypoint(
        Vector3(600.0f, 350.0f, 400.0f),   // Higher for better overview
        -35.0f,                             // Steeper angle to see more terrain
        135.0f,                             // Angle towards temple/hill
        6.0f                                // 6 seconds - slow opening
    ));

    // Waypoint 2: Focus on the Lake Basin - View the lake in lower-left quadrant
    // Lake is at (500, 17, 400) with 300 unit radius
    camera->AddWaypoint(CameraWaypoint(
        Vector3(700.0f, 80.0f, 200.0f),    // Slightly higher for better water view
        -25.0f,                             // Better angle to see water reflections
        250.0f,                             // Looking west/southwest towards lake
        5.0f                                // 5 seconds
    ));

    // Waypoint 3: TEMPLE/RUINS DIRECT VIEW - Mid-distance frontal view
    // CRITICAL: Temple at (1000, 159, 1500) - camera must face it!
    camera->AddWaypoint(CameraWaypoint(
        Vector3(700.0f, 220.0f, 1500.0f),  // Higher to see temple details
        -10.0f,                             // Shallower angle for better architecture view
        90.0f,                              // Looking EAST (90deg = towards +X = temple)
        7.0f                                // 7 seconds - more time for main feature
    ));

    // Waypoint 4: TEMPLE/RUINS CLOSE-UP - Elevated diagonal view
    // Shows details of temple architecture (Ancient) or broken pillars (Modern)
    camera->AddWaypoint(CameraWaypoint(
        Vector3(1250.0f, 240.0f, 1250.0f), // Closer and higher for detail shot
        -20.0f,                             // Less steep to see architecture better
        225.0f,                             // Looking northwest towards temple center
        6.0f                                // 6 seconds for detail appreciation
    ));

    // Waypoint 5: Hilltop Overview - Highest point looking back
    // Shows the entire scene from opposite angle
    camera->AddWaypoint(CameraWaypoint(
        Vector3(1300.0f, 200.0f, 1700.0f), // Top of hill
        -30.0f,                             // Looking down across scene
        220.0f,                             // View back towards lake
        6.0f                                // 6 seconds
    ));

    // Waypoint 6: Grass Field - Ground-level immersive view
    // Shows grass animation and terrain details up close
    camera->AddWaypoint(CameraWaypoint(
        Vector3(1200.0f, 100.0f, 1200.0f), // In the grass field
        -5.0f,                              // Nearly level view
        270.0f,                             // Looking across terrain
        5.0f                                // 5 seconds
    ));

    // Waypoint 7: Return View - Transition back to start
    // Creates smooth loop back to waypoint 1
    camera->AddWaypoint(CameraWaypoint(
        Vector3(700.0f, 250.0f, 600.0f),   // Mid-height, mid-scene
        -20.0f,                             // Medium downward angle
        160.0f,                             // Angle back towards start
        5.0f                                // 5 seconds
    ));

    // Reset to first waypoint
    camera->ResetTrack();

    LOG_INFO("Camera track initialized with " << 7 << " waypoints.");
    LOG_INFO("Total track duration: ~38 seconds (loops continuously)");
}


// ========================================
// UNUSED FUNCTIONS (kept for potential future use)
// ========================================

// ========================================
// Temple Texture Loading
// ========================================

void Renderer::LoadTempleTextures() {
    LOG_INFO("Loading temple textures...");

    // Try loading roof tiles (PNG file - should work better than JPG)
    templeRoofTex = SOIL_load_OGL_texture(
        TEXTUREDIR"roof_tiles_chinese/RoofingTiles005_1K-PNG_Color.png",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_TEXTURE_REPEATS
    );
    if (templeRoofTex == 0) {
        LOG_INFO("WARNING: Failed to load roof texture, using fallback");
        templeRoofTex = terrainRockTex;
    } else {
        LOG_INFO("SUCCESS: Roof texture loaded, ID: " << templeRoofTex);
        glBindTexture(GL_TEXTURE_2D, templeRoofTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    // Try loading wall texture (JPG diffuse map)
    templeWallTex = SOIL_load_OGL_texture(
        TEXTUREDIR"temple_wall_stone/textures/stone_wall_05_diff_1k.jpg",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_TEXTURE_REPEATS
    );
    if (templeWallTex == 0) {
        LOG_INFO("WARNING: Failed to load wall texture, using fallback");
        templeWallTex = terrainRockTex;
    } else {
        LOG_INFO("SUCCESS: Wall texture loaded, ID: " << templeWallTex);
        glBindTexture(GL_TEXTURE_2D, templeWallTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    // Try loading wood texture (JPG)
    templeWoodTex = SOIL_load_OGL_texture(
        TEXTUREDIR"temple_wood_dark/textures/temple_wood_dark.jpg",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_TEXTURE_REPEATS
    );
    if (templeWoodTex == 0) {
        LOG_INFO("WARNING: Failed to load wood texture, using fallback");
        templeWoodTex = terrainSandTex;
    } else {
        LOG_INFO("SUCCESS: Wood texture loaded, ID: " << templeWoodTex);
        glBindTexture(GL_TEXTURE_2D, templeWoodTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    // Try loading floor texture (JPG diffuse map)
    templeFloorTex = SOIL_load_OGL_texture(
        TEXTUREDIR"temple_stone_floor/textures/stone_wall_05_diff_1k.jpg",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_TEXTURE_REPEATS
    );
    if (templeFloorTex == 0) {
        LOG_INFO("WARNING: Failed to load floor texture, using fallback");
        templeFloorTex = terrainTex;
    } else {
        LOG_INFO("SUCCESS: Floor texture loaded, ID: " << templeFloorTex);
        glBindTexture(GL_TEXTURE_2D, templeFloorTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    LOG_INFO("Temple textures loaded:");
    LOG_INFO("  Roof texture ID: " << templeRoofTex);
    LOG_INFO("  Wall texture ID: " << templeWallTex);
    LOG_INFO("  Wood texture ID: " << templeWoodTex);
    LOG_INFO("  Floor texture ID: " << templeFloorTex);
}

// DELETED: DrawGLTFScene() - now in ScenePass.cpp (as DrawGLTFTemple)

// ========================================
// Ruins System Implementation (Modern Era)
// ========================================

void Renderer::InitializeRuinsSystem() {
    // Load meshes for ruins
    pillarMesh = std::unique_ptr<Mesh>(Mesh::LoadFromMeshFile("Cylinder.msh"));
    debrisMesh = std::unique_ptr<Mesh>(Mesh::LoadFromMeshFile("Cube.msh"));

    if (!pillarMesh) {
        std::cerr << "ERROR: Failed to load Cylinder.msh for pillars!" << std::endl;
    } else {
        LOG_INFO("Pillar mesh loaded successfully.");
    }

    if (!debrisMesh) {
        std::cerr << "ERROR: Failed to load Cube.msh for debris!" << std::endl;
    } else {
        LOG_INFO("Debris mesh loaded successfully.");
    }

    // Load cracked/weathered stone texture for ruins
    crackedStoneTex = SOIL_load_OGL_texture(
        TEXTUREDIR"brick.tga",  // Use brick as weathered stone
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (!crackedStoneTex) {
        std::cerr << "WARNING: Failed to load cracked stone texture, using fallback" << std::endl;
        crackedStoneTex = terrainRockTex;
    } else {
        LOG_INFO("Cracked stone texture loaded (ID: " << crackedStoneTex << ")");
    }

    // Create ruins scene graph (NOT added to main root, rendered separately)
    ruinsRoot = std::make_unique<SceneNode>();

    // Get temple position (same as where temple was)
    // IMPORTANT: Must match DrawGLTFScene temple position EXACTLY
    float templeX = 1000.0f;
    float templeZ = 1500.0f;
    float templeY = customTerrain->GetHeightAt(templeX, templeZ);

    // CRITICAL: Add the same +40 lift that the temple has!
    // This ensures ruins appear at EXACT temple location
    templeY += 40.0f;

    // DEBUG: Print actual temple coordinates
    LOG_INFO("========================================");
    LOG_INFO("TEMPLE POSITION (for ruins placement):");
    LOG_INFO("  X = " << templeX);
    LOG_INFO("  Y = " << templeY << " (terrain height + 40 lift)");
    LOG_INFO("  Z = " << templeZ);
    LOG_INFO("========================================");

    // ========================================
    // Create broken pillars (remains of temple columns)
    // IMPORTANT: All positioned tightly around temple footprint (x=1000, z=1500)
    // Temple was ~700 units wide (27x scale), so pillars within ±300 units
    // ========================================
    LOG_INFO("Creating broken pillars at temple ruins (concentrated at original site)...");

    // Front-left corner pillar (short, heavily tilted)
    SceneNode* pillar1 = CreateBrokenPillar(
        Vector3(templeX - 200.0f, templeY, templeZ - 150.0f),  // Front-left corner
        140.0f,   // Height (broken off)
        30.0f,    // Heavy tilt
        45.0f     // Rotation
    );
    ruinsRoot->AddChild(pillar1);

    // Front-right corner pillar (medium height)
    SceneNode* pillar2 = CreateBrokenPillar(
        Vector3(templeX + 220.0f, templeY, templeZ - 130.0f),  // Front-right corner
        200.0f,   // Height
        18.0f,    // Moderate tilt
        -30.0f    // Rotation
    );
    ruinsRoot->AddChild(pillar2);

    // Front-center pillar (tallest, most intact)
    SceneNode* pillar3 = CreateBrokenPillar(
        Vector3(templeX - 20.0f, templeY, templeZ - 180.0f),   // Front-center
        300.0f,   // Height (tallest remaining)
        10.0f,    // Slight tilt
        0.0f      // Facing forward
    );
    ruinsRoot->AddChild(pillar3);

    // Center pillar (medium, tilted dramatically)
    SceneNode* pillar4 = CreateBrokenPillar(
        Vector3(templeX + 80.0f, templeY, templeZ + 20.0f),    // Center of temple
        180.0f,   // Medium height
        35.0f,    // Heavy tilt
        120.0f    // Rotation
    );
    ruinsRoot->AddChild(pillar4);

    // Back-left corner pillar (short, collapsed)
    SceneNode* pillar5 = CreateBrokenPillar(
        Vector3(templeX - 180.0f, templeY, templeZ + 160.0f),  // Back-left corner
        90.0f,    // Very short (mostly collapsed)
        45.0f,    // Very heavy tilt
        -60.0f    // Rotation
    );
    ruinsRoot->AddChild(pillar5);

    // Back-right corner pillar (fallen sideways)
    SceneNode* pillar6 = CreateBrokenPillar(
        Vector3(templeX + 200.0f, templeY + 10.0f, templeZ + 150.0f),  // Back-right corner
        220.0f,   // Length when horizontal
        80.0f,    // Nearly horizontal (fallen)
        90.0f     // Rotation
    );
    ruinsRoot->AddChild(pillar6);

    // Extra pillars for more realistic ruin density
    // Side pillar 1
    SceneNode* pillar7 = CreateBrokenPillar(
        Vector3(templeX - 250.0f, templeY, templeZ + 10.0f),   // Left side
        160.0f,   // Medium height
        25.0f,    // Moderate tilt
        15.0f     // Rotation
    );
    ruinsRoot->AddChild(pillar7);

    // Side pillar 2
    SceneNode* pillar8 = CreateBrokenPillar(
        Vector3(templeX + 260.0f, templeY, templeZ + 50.0f),   // Right side
        120.0f,   // Short
        40.0f,    // Heavy tilt
        -45.0f    // Rotation
    );
    ruinsRoot->AddChild(pillar8);

    LOG_INFO("Created 8 broken pillars concentrated at temple site.");

    // ========================================
    // Add debris/rubble (fallen temple pieces)
    // IMPORTANT: Scattered rocks and broken pieces around pillars
    // ========================================
    LOG_INFO("Adding rubble and debris to temple ruins...");

    // Debris cluster 1: Near front-left pillar
    SceneNode* debris1 = CreateDebris(
        Vector3(templeX - 180.0f, templeY - 20.0f, templeZ - 120.0f),
        Vector3(40, 25, 50),  // Irregular rock shape
        30.0f
    );
    ruinsRoot->AddChild(debris1);

    // Debris cluster 2: Front-center area (large fallen block)
    SceneNode* debris2 = CreateDebris(
        Vector3(templeX - 50.0f, templeY - 15.0f, templeZ - 100.0f),
        Vector3(60, 35, 45),  // Large block
        -15.0f
    );
    ruinsRoot->AddChild(debris2);

    // Debris cluster 3: Center temple area (scattered small rocks)
    SceneNode* debris3 = CreateDebris(
        Vector3(templeX + 50.0f, templeY - 25.0f, templeZ),
        Vector3(30, 20, 35),
        75.0f
    );
    ruinsRoot->AddChild(debris3);

    // Debris cluster 4: Near front-right pillar
    SceneNode* debris4 = CreateDebris(
        Vector3(templeX + 190.0f, templeY - 18.0f, templeZ - 90.0f),
        Vector3(45, 30, 40),
        -60.0f
    );
    ruinsRoot->AddChild(debris4);

    // Debris cluster 5: Back-left area
    SceneNode* debris5 = CreateDebris(
        Vector3(templeX - 130.0f, templeY - 22.0f, templeZ + 140.0f),
        Vector3(50, 28, 38),
        120.0f
    );
    ruinsRoot->AddChild(debris5);

    // Debris cluster 6: Back-right corner
    SceneNode* debris6 = CreateDebris(
        Vector3(templeX + 170.0f, templeY - 20.0f, templeZ + 130.0f),
        Vector3(42, 32, 48),
        -90.0f
    );
    ruinsRoot->AddChild(debris6);

    // Additional small debris scattered around
    SceneNode* debris7 = CreateDebris(
        Vector3(templeX - 20.0f, templeY - 28.0f, templeZ + 60.0f),
        Vector3(25, 18, 30),
        45.0f
    );
    ruinsRoot->AddChild(debris7);

    SceneNode* debris8 = CreateDebris(
        Vector3(templeX + 100.0f, templeY - 24.0f, templeZ - 40.0f),
        Vector3(35, 22, 28),
        -135.0f
    );
    ruinsRoot->AddChild(debris8);

    LOG_INFO("Created 8 debris piles scattered in temple ruins.");
}

SceneNode* Renderer::CreateBrokenPillar(Vector3 position, float height, float tiltAngle, float rotation) {
    if (!pillarMesh) {
        return nullptr;
    }

    // DEBUG: Print pillar position
    LOG_INFO("  Creating pillar at: (" << position.x << ", " << position.y << ", " << position.z << ")");

    SceneNode* pillar = new SceneNode();
    pillar->SetMesh(pillarMesh.get());  // Pass raw pointer from unique_ptr
    pillar->SetShader(sceneShader.get());
    pillar->SetTexture(crackedStoneTex);

    // Weathered gray-brown stone color
    pillar->SetColour(Vector4(0.5f, 0.45f, 0.4f, 1.0f));

    // Pillar dimensions (thicker than original temple columns - ruins are more robust)
    float radius = 25.0f;
    pillar->SetModelScale(Vector3(radius, height, radius));

    // CRITICAL FIX: Separate world position from local rotations!
    // Do NOT mix Translation(position) with local transforms - it causes wrong placement!

    // Step 1: Set world position directly (no transform matrix)
    pillar->SetTransform(Matrix4::Translation(position));

    // Step 2: Add rotations at LOCAL origin (this won't affect world position)
    Matrix4 localRotation =
        Matrix4::Rotation(rotation, Vector3(0, 1, 0)) *  // Rotate around Y
        Matrix4::Rotation(tiltAngle, Vector3(0, 0, 1));   // Tilt

    // Step 3: Combine position with rotation (position LAST so it's not affected)
    Matrix4 finalTransform = Matrix4::Translation(position) * localRotation;

    pillar->SetTransform(finalTransform);
    pillar->SetBoundingRadius(height);

    LOG_INFO("    Pillar placed at world position: (" << position.x << ", " << position.y << ", " << position.z << ")");

    return pillar;
}

SceneNode* Renderer::CreateDebris(Vector3 position, Vector3 scale, float rotation) {
    if (!debrisMesh) {
        return nullptr;
    }

    SceneNode* debris = new SceneNode();
    debris->SetMesh(debrisMesh.get());  // Pass raw pointer from unique_ptr
    debris->SetShader(sceneShader.get());
    debris->SetTexture(crackedStoneTex);  // Weathered stone texture

    // Dark gray/brown weathered stone color
    debris->SetColour(Vector4(0.4f, 0.38f, 0.35f, 1.0f));

    // Use provided scale for irregular shapes
    debris->SetModelScale(scale);

    // Position with rotation for natural randomness
    Matrix4 transform =
        Matrix4::Translation(position) *
        Matrix4::Rotation(rotation, Vector3(0, 1, 0)) *
        Matrix4::Rotation(rotation * 0.5f, Vector3(1, 0, 0));  // Slight X tilt too

    debris->SetTransform(transform);
    debris->SetBoundingRadius(scale.Length());

    return debris;
}

// DELETED: DrawRuins() - now in ScenePass.cpp

// ========================================
// Skeletal Animation System Implementation
// ========================================

void Renderer::InitializeSkeletalAnimation() {
    // Load skinned shader
    skinnedShader = std::make_unique<Shader>("skinned.vert", "skinned.frag");
    if (!skinnedShader->LoadSuccess()) {
        std::cerr << "ERROR: Failed to load skinned shader!" << std::endl;
        skinnedShader.reset();  // Reset unique_ptr (same as setting to nullptr)
        return;
    }
    LOG_INFO("Skinned shader loaded successfully.");

    // Load character mesh (Role_T.msh has skeletal data)
    characterMesh = std::unique_ptr<Mesh>(Mesh::LoadFromMeshFile("Role_T.msh"));
    if (!characterMesh) {
        std::cerr << "ERROR: Failed to load Role_T.msh!" << std::endl;
        return;
    }

    // Check if mesh has skeletal data
    if (characterMesh->GetJointCount() == 0) {
        std::cerr << "WARNING: Role_T.msh has no skeletal data!" << std::endl;
        std::cerr << "Character will render as static mesh." << std::endl;
    } else {
        LOG_INFO("Character mesh loaded with " << characterMesh->GetJointCount() << " joints.");
    }

    // Create skeletal animator
    skeletalAnimator = std::make_unique<SkeletalAnimator>(characterMesh.get());

    // Load character texture (use brick texture as placeholder)
    characterTexture = SOIL_load_OGL_texture(
        TEXTUREDIR"brick.tga",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (!characterTexture) {
        std::cerr << "WARNING: Failed to load character texture, using terrain texture" << std::endl;
        characterTexture = terrainTex;
    } else {
        LOG_INFO("Character texture loaded successfully.");
    }
}

// DELETED: DrawSkinnedCharacter() - now in ScenePass.cpp

void Renderer::DrawSun() {
    if (!sunQuad || !sunShader || !dayNightCycle) {
        return;  // Sun system not initialized
    }

    // Enable depth test so sun can be occluded by terrain/objects
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);  // Allow sun at far plane

    // Enable alpha blending for smooth edges
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    BindShader(sunShader.get());

    // Get sun direction from day-night cycle
    Vector3 sunDir = dayNightCycle->GetSunDirection();

    // Position sun/moon in world space (orbiting around scene center)
    Vector3 sceneCenter(1000.0f, 0.0f, 1000.0f);  // Center of 2000x2000 terrain
    float orbitRadius = 8000.0f;  // Large orbit radius so sun is far away

    // Sun position = scene center + (sun direction * orbit radius)
    Vector3 sunPosition = sceneCenter + (sunDir * orbitRadius);

    // Create model matrix: translate to sun position, then scale
    Matrix4 model = Matrix4::Translation(sunPosition) *
                    Matrix4::Scale(Vector3(150.0f, 150.0f, 150.0f));  // Sun/moon size

    // Set shader uniforms
    glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "modelMatrix"),
                      1, false, (float*)&model);
    glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "viewMatrix"),
                      1, false, (float*)&viewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "projMatrix"),
                      1, false, (float*)&projMatrix);

    // Pass sun color and time of day
    Vector4 sunColor = dayNightCycle->GetSunColor();
    glUniform4fv(glGetUniformLocation(sunShader->GetProgram(), "sunColor"),
                1, (float*)&sunColor);
    glUniform1f(glGetUniformLocation(sunShader->GetProgram(), "timeOfDay"),
               dayNightCycle->GetTimeOfDay());

    // Draw sun/moon sphere
    sunQuad->Draw();

    // Restore depth function
    glDepthFunc(GL_LESS);
}

// ========================================
// Voxel Tree System Initialization
// ========================================
void Renderer::InitializeVoxelTreeSystem() {
    // Load configuration from file (NO HARDCODING!)
    TreeConfigLoader config;
    config.LoadFromFile("tree_config.txt");

    // Create terrain adapter for tree placement
    terrainAdapter = std::make_unique<CustomTerrainAdapter>(customTerrain.get());

    // Create voxel tree system
    voxelTreeSystem = std::make_unique<VoxelTreeSystem>(terrainAdapter.get());

    // Initialize the system
    if (!voxelTreeSystem->Initialize()) {
        std::cerr << "ERROR: Failed to initialize VoxelTreeSystem!" << std::endl;
        voxelTreeSystem.reset();
        return;
    }

    // Load tree textures
    LoadTreeTextures();

    // Set textures in the system
    voxelTreeSystem->SetWoodTexture(woodAlbedoTex, woodNormalTex);
    voxelTreeSystem->SetLeafTexture(leafAlbedoTex, leafNormalTex);

    // Configure tree shape from file (NO HARDCODING!)
    TreeConfig treeShapeConfig;
    treeShapeConfig.minHeight = config.minHeight;
    treeShapeConfig.maxHeight = config.maxHeight;
    treeShapeConfig.canopyRadius = config.canopyRadius;
    treeShapeConfig.canopyHeight = config.canopyHeight;
    treeShapeConfig.canopyDensity = config.canopyDensity;
    treeShapeConfig.sphericalCanopy = config.sphericalCanopy;
    treeShapeConfig.blockSize = config.blockSize;

    // Set default config for all trees (from config file!)
    voxelTreeSystem->SetDefaultConfig(treeShapeConfig);

    // Generate trees using configuration
    LOG_INFO("Generating trees with config:");
    LOG_INFO("  Target: " << config.treeCount << " trees");
    LOG_INFO("  Slope range: " << config.minSlope << "-" << config.maxSlope << " degrees");
    LOG_INFO("  Min spacing: " << config.minSpacing << " units");

    if (config.enableRegionFilter) {
        LOG_INFO("  Region filter: X=[" << config.regionMinX << ", " << config.regionMaxX << "], "
                  << "Z=[" << config.regionMinZ << ", " << config.regionMaxZ << "]");
        LOG_INFO("  Height range: [" << config.regionMinHeight << ", " << config.regionMaxHeight << "]");
    }

    voxelTreeSystem->GenerateTrees(
        config.treeCount,
        config.minSlope,
        config.maxSlope,
        config.minSpacing,
        config.randomSeed,
        config.enableRegionFilter,
        config.regionMinX,
        config.regionMaxX,
        config.regionMinZ,
        config.regionMaxZ,
        config.regionMinHeight,
        config.regionMaxHeight
    );

    // Store growth speed for Update() calls
    treeGrowthSpeed = config.growthSpeed;

    // Set initial growth progress for all trees
    if (config.enableGrowth) {
        voxelTreeSystem->SetInitialGrowth(config.initialGrowth);
        LOG_INFO("Growth system enabled. Trees will grow over time.");
        LOG_INFO("  Initial growth: " << (config.initialGrowth * 100.0f) << "%");
        LOG_INFO("  Growth speed: " << config.growthSpeed << " per day");
    }

    LOG_INFO("Generated " << voxelTreeSystem->GetTreeCount() << " voxel trees with "
              << voxelTreeSystem->GetTotalBlockCount() << " total blocks.");
}

void Renderer::LoadTreeTextures() {
    LOG_INFO("Loading tree block textures...");

    // Load wood albedo texture (use brick as placeholder for wood texture)
    woodAlbedoTex = SOIL_load_OGL_texture(
        TEXTUREDIR"brick.tga",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (!woodAlbedoTex) {
        std::cerr << "WARNING: Failed to load wood albedo texture!" << std::endl;
    } else {
        LOG_INFO("Wood albedo texture loaded.");
        glBindTexture(GL_TEXTURE_2D, woodAlbedoTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    // Load leaf albedo texture (use Barren Reds as placeholder for leaves)
    leafAlbedoTex = SOIL_load_OGL_texture(
        TEXTUREDIR"Barren Reds.JPG",
        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS
    );

    if (!leafAlbedoTex) {
        std::cerr << "WARNING: Failed to load leaf albedo texture!" << std::endl;
    } else {
        LOG_INFO("Leaf albedo texture loaded.");
        glBindTexture(GL_TEXTURE_2D, leafAlbedoTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    LOG_INFO("Tree textures loaded successfully.");
}

// ========================================
// Deferred Rendering System Initialization
// ========================================
void Renderer::InitializeDeferredRendering() {
    // Load configuration from file (NO HARDCODING!)
    LightingConfig config;
    if (!config.LoadFromFile("deferred_config.txt")) {
        std::cerr << "WARNING: Failed to load deferred_config.txt, using defaults." << std::endl;
        // Will continue with default values from LightingConfig
    }

    // Store configuration
    useDeferredRendering = config.enableDeferred;
    ambientColour = config.ambientColour;
    ambientIntensity = config.ambientIntensity;

    if (!useDeferredRendering) {
        LOG_INFO("Deferred rendering is DISABLED in config.");
        return;
    }

    LOG_INFO("Deferred rendering is ENABLED.");

    // Create deferred renderer (pass screen dimensions)
    deferredRenderer = std::make_unique<DeferredRenderer>(width, height);

    // Initialize the deferred renderer
    if (!deferredRenderer->Initialize()) {
        std::cerr << "ERROR: Failed to initialize DeferredRenderer!" << std::endl;
        deferredRenderer.reset();
        useDeferredRendering = false;
        return;
    }

    // Load deferred rendering shaders
    LOG_INFO("Loading deferred rendering shaders...");

    // Geometry Pass Shader
    deferredGeometryShader = std::make_unique<Shader>(
        "deferred_geometry.vert",
        "deferred_geometry.frag"
    );

    if (!deferredGeometryShader->LoadSuccess()) {
        std::cerr << "ERROR: Failed to load deferred geometry shader!" << std::endl;
        deferredRenderer.reset();
        useDeferredRendering = false;
        return;
    }

    // Lighting Pass Shader
    deferredLightingShader = std::make_unique<Shader>(
        "deferred_lighting.vert",
        "deferred_lighting.frag"
    );

    if (!deferredLightingShader->LoadSuccess()) {
        std::cerr << "ERROR: Failed to load deferred lighting shader!" << std::endl;
        deferredRenderer.reset();
        useDeferredRendering = false;
        return;
    }

    LOG_INFO("Deferred rendering shaders loaded successfully.");

    // Load debug visualization shader
    debugUnlitShader = std::make_unique<Shader>("debug_unlit.vert", "debug_unlit.frag");
    if (!debugUnlitShader->LoadSuccess()) {
        LOG_INFO("WARNING: Failed to load debug unlit shader (light visualization will not work)");
        debugUnlitShader.reset();
    }

    // Set shaders to deferred renderer
    deferredRenderer->SetGeometryShader(deferredGeometryShader.get());
    deferredRenderer->SetLightingShader(deferredLightingShader.get());

    // Load point lights from configuration
    LOG_INFO("Loading " << config.pointLights.size() << " point lights...");
    for (const auto& light : config.pointLights) {
        deferredRenderer->AddLight(light);
        LOG_INFO("  - Light at (" << light.position.x << ", "
                  << light.position.y << ", " << light.position.z << ") "
                  << "Color: (" << light.colour.x << ", " << light.colour.y << ", " << light.colour.z << ") "
                  << "Intensity: " << light.intensity << ", Radius: " << light.radius);
    }

    LOG_INFO("Deferred rendering system initialized with " << config.pointLights.size() << " point lights.");
}
