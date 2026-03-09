#pragma once

/*
 * DeferredScenePass.h
 *
 * Deferred Rendering Scene Pass
 *
 * Features:
 * - Render scene using deferred rendering pipeline
 * - Geometry Pass: render to G-Buffer
 * - Lighting Pass: calculate lighting from G-Buffer
 * - Integrated into RenderPipeline architecture
 *
 * Architecture Design:
 * - Inherit from ContextualRenderPass
 * - Encapsulate deferred rendering workflow
 * - No hardcoded values (read from config)
 */

#include "RenderPass.h"
#include "RenderConfig.h"
#include "DeferredRenderer.h"
#include "../nclgl/Mesh.h"
#include "../nclgl/Shader.h"
#include "../nclgl/CustomTerrain.h"
#include "GrassRenderer.h"
#include "WaterRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Extra/GLTFLoader.h"
#include "SkeletalAnimator.h"
#include "DayNightCycle.h"
#include "VoxelTreeSystem.h"

class DeferredScenePass : public ContextualRenderPass {
public:
    DeferredScenePass(RenderContext* context, DeferredRenderer* deferred)
        : ContextualRenderPass("DeferredScenePass", context)
        , deferredRenderer(deferred)
        , terrainShader(nullptr)
        , terrain(nullptr)
        , terrainTex(0), terrainSandTex(0), terrainRockTex(0), terrainMudTex(0)
        , skyboxShader(nullptr), skyboxMesh(nullptr)
        , skyboxDay(0), skyboxNight(0), skyboxSunset(0)
        , grassRenderer(nullptr), waterRenderer(nullptr)
        , sceneShader(nullptr)
        , gltfShader(nullptr), gltfScene(nullptr)
        , templeRoofTex(0), templeWallTex(0), templeWoodTex(0), templeFloorTex(0)
        , ruinsRoot(nullptr)
        , skinnedShader(nullptr), skeletalAnimator(nullptr), characterMesh(nullptr), characterTexture(0)
        , sunShader(nullptr), sunQuad(nullptr), dayNightCycle(nullptr)
        , voxelTreeSystem(nullptr)
        , customTerrain(nullptr)
    {
    }

    virtual ~DeferredScenePass() {}

    void SetCustomTerrain(CustomTerrain* t) { customTerrain = t; }

    /**
     * Set terrain resources
     */
    void SetTerrainResources(Shader* shader, CustomTerrain* terrainMesh,
                            GLuint tex, GLuint sandTex, GLuint rockTex, GLuint mudTex) {
        terrainShader = shader;
        terrain = terrainMesh;
        terrainTex = tex;
        terrainSandTex = sandTex;
        terrainRockTex = rockTex;
        terrainMudTex = mudTex;
    }

    /**
     * Set skybox resources
     */
    void SetSkyboxResources(Shader* shader, Mesh* mesh, GLuint dayTex, GLuint nightTex, GLuint sunsetTex) {
        skyboxShader = shader;
        skyboxMesh = mesh;
        skyboxDay = dayTex;
        skyboxNight = nightTex;
        skyboxSunset = sunsetTex;
    }

    /**
     * Set grass renderer
     */
    void SetGrassRenderer(GrassRenderer* renderer) {
        grassRenderer = renderer;
    }

    /**
     * Set water renderer
     */
    void SetWaterRenderer(WaterRenderer* renderer) {
        waterRenderer = renderer;
    }

    /**
     * Set scene graph resources
     */
    void SetSceneGraphResources(Shader* shader) {
        sceneShader = shader;
    }

    /**
     * Set GLTF resources
     */
    void SetGLTFResources(Shader* shader, GLTFScene* scene,
                         GLuint roofTex, GLuint wallTex, GLuint woodTex, GLuint floorTex) {
        gltfShader = shader;
        gltfScene = scene;
        templeRoofTex = roofTex;
        templeWallTex = wallTex;
        templeWoodTex = woodTex;
        templeFloorTex = floorTex;
    }

    /**
     * Set ruins resources
     */
    void SetRuinsResources(SceneNode* ruins) {
        ruinsRoot = ruins;
    }

    /**
     * Set skeletal animation resources
     */
    void SetSkeletalResources(Shader* shader, SkeletalAnimator* animator,
                             Mesh* mesh, GLuint texture) {
        skinnedShader = shader;
        skeletalAnimator = animator;
        characterMesh = mesh;
        characterTexture = texture;
    }

    /**
     * Set sun resources
     */
    void SetSunResources(Shader* shader, Mesh* mesh, DayNightCycle* cycle) {
        sunShader = shader;
        sunQuad = mesh;
        dayNightCycle = cycle;
    }

    /**
     * Set voxel tree system
     */
    void SetVoxelTreeSystem(VoxelTreeSystem* system) {
        voxelTreeSystem = system;
    }

    /**
     * Execute deferred rendering pass
     */
    virtual void Execute(OGLRenderer& renderer) override {
        if (!deferredRenderer || !enabled) {
            return;
        }

        // Stage 1: Geometry Pass - Render scene to G-Buffer
        deferredRenderer->BeginGeometryPass();

        if (terrain) {
            Shader* geoShader = deferredRenderer->GetGeometryShader();
            if (geoShader) {
                glUseProgram(geoShader->GetProgram());

                Matrix4 modelMatrix;
                modelMatrix.ToIdentity();
                glUniformMatrix4fv(glGetUniformLocation(geoShader->GetProgram(), "modelMatrix"),
                                  1, false, (float*)&modelMatrix);
                glUniformMatrix4fv(glGetUniformLocation(geoShader->GetProgram(), "viewMatrix"),
                                  1, false, (float*)&context->viewMatrix);
                glUniformMatrix4fv(glGetUniformLocation(geoShader->GetProgram(), "projMatrix"),
                                  1, false, (float*)&context->projMatrix);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, terrainTex);
                glUniform1i(glGetUniformLocation(geoShader->GetProgram(), "diffuseTex"), 0);
                glUniform1i(glGetUniformLocation(geoShader->GetProgram(), "useNormalMap"), 0);
                glUniform1f(glGetUniformLocation(geoShader->GetProgram(), "specularIntensity"), 0.5f);

                terrain->Draw();
                glUseProgram(0);
            }
        }

        deferredRenderer->EndGeometryPass();

        // Stage 2: Lighting Pass (with directional sun light)
        const auto& lights = deferredRenderer->GetLights();

        // Calculate sun direction from the scene's main light
        Vector3 sunDir(0, 1, 0);
        Vector3 sunCol(1, 1, 1);
        float sunInt = 1.0f;
        if (context->light) {
            // For DirectionalLight, GetPosition() returns -direction * 5000
            // We need the direction TO the light (normalized)
            sunDir = context->light->GetPosition();
            sunDir.Normalise();
            Vector4 lc = context->light->GetColour();
            sunCol = Vector3(lc.x, lc.y, lc.z);
            sunInt = lc.w;  // Use alpha as intensity
        }

        deferredRenderer->ExecuteLightingPass(
            context->camera,
            lights,
            Vector3(0.3f, 0.3f, 0.35f),
            1.0f,
            context->sceneFBO,
            sunDir, sunCol, sunInt
        );

        // Stage 3: Forward rendering (everything except terrain)
        glBindFramebuffer(GL_FRAMEBUFFER, context->sceneFBO);
        glViewport(0, 0, context->screenWidth, context->screenHeight);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);

        // Skybox - dynamic selection based on time of day
        if (skyboxShader && skyboxMesh && skyboxDay) {
            // Select skybox based on time of day (matching ScenePass logic)
            GLuint currentSkybox = skyboxDay;
            if (dayNightCycle) {
                float timeOfDay = dayNightCycle->GetTimeOfDay();
                if (timeOfDay < 5.0f || timeOfDay >= 19.0f) {
                    currentSkybox = skyboxNight;    // Night
                } else if ((timeOfDay >= 5.0f && timeOfDay < 7.0f) ||
                           (timeOfDay >= 17.0f && timeOfDay < 19.0f)) {
                    currentSkybox = skyboxSunset;   // Sunrise/Sunset
                } else {
                    currentSkybox = skyboxDay;      // Day
                }
            }

            glDepthMask(GL_FALSE);
            glUseProgram(skyboxShader->GetProgram());

            glUniformMatrix4fv(glGetUniformLocation(skyboxShader->GetProgram(), "viewMatrix"),
                              1, false, (float*)&context->viewMatrix);
            glUniformMatrix4fv(glGetUniformLocation(skyboxShader->GetProgram(), "projMatrix"),
                              1, false, (float*)&context->projMatrix);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, currentSkybox);
            glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"), 0);

            skyboxMesh->Draw();

            glDepthMask(GL_TRUE);
            glUseProgram(0);
        }

        // GLTF Temple (ancient era only)
        if (gltfShader && gltfScene && !gltfScene->sceneNodes.empty() && !context->isModern) {
            glDisable(GL_BLEND);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);

            glUseProgram(gltfShader->GetProgram());
            glUniform1i(glGetUniformLocation(gltfShader->GetProgram(), "diffuseTex"), 0);

            Vector3 templePos(RenderConfig::TEMPLE_POSITION_X, RenderConfig::TEMPLE_POSITION_Y,
                             RenderConfig::TEMPLE_POSITION_Z);
            Matrix4 baseTransform = Matrix4::Translation(templePos) *
                                   Matrix4::Rotation(RenderConfig::TEMPLE_ROTATION_Y, Vector3(0, 1, 0)) *
                                   Matrix4::Scale(Vector3(RenderConfig::TEMPLE_SCALE,
                                                         RenderConfig::TEMPLE_SCALE,
                                                         RenderConfig::TEMPLE_SCALE));

            glActiveTexture(GL_TEXTURE0);
            for (const GLTFNode& node : gltfScene->sceneNodes) {
                if (!node.mesh) continue;

                Matrix4 model = baseTransform * node.worldMatrix;
                glUniformMatrix4fv(glGetUniformLocation(gltfShader->GetProgram(), "modelMatrix"),
                                  1, false, (float*)&model);
                glUniformMatrix4fv(glGetUniformLocation(gltfShader->GetProgram(), "viewMatrix"),
                                  1, false, (float*)&context->viewMatrix);
                glUniformMatrix4fv(glGetUniformLocation(gltfShader->GetProgram(), "projMatrix"),
                                  1, false, (float*)&context->projMatrix);

                GLuint textureToUse = templeWallTex;
                switch (node.materialType) {
                    case GLTFMaterialType::ROOF:  textureToUse = templeRoofTex; break;
                    case GLTFMaterialType::WOOD:  textureToUse = templeWoodTex; break;
                    case GLTFMaterialType::FLOOR: textureToUse = templeFloorTex; break;
                    default: break;
                }
                glBindTexture(GL_TEXTURE_2D, textureToUse);
                node.mesh->Draw();
            }
            glUseProgram(0);
            glEnable(GL_BLEND);
        }

        // Ruins (modern era only)
        if (sceneShader && ruinsRoot && context->isModern) {
            ruinsRoot->Update(0.0f);
            glUseProgram(sceneShader->GetProgram());
            SetSceneShaderUniforms(sceneShader);
            DrawSceneNode(ruinsRoot, sceneShader);
            glUseProgram(0);
        }

        // Scene graph (with era filtering)
        if (context->sceneRoot && sceneShader) {
            glUseProgram(sceneShader->GetProgram());
            SetSceneShaderUniforms(sceneShader);
            DrawSceneNodeFiltered(context->sceneRoot, sceneShader);
            glUseProgram(0);
        }

        // Grass
        if (grassRenderer) {
            grassRenderer->Render(context->light, context->camera, context->totalTime,
                                 1.0f, context->viewMatrix, context->projMatrix);
        }

        // Water
        if (waterRenderer && skyboxDay) {
            waterRenderer->Render(context->light, context->camera, context->totalTime,
                                 terrainTex, terrainTex, skyboxDay,
                                 context->viewMatrix, context->projMatrix);
        }

        // Voxel trees
        if (voxelTreeSystem) {
            voxelTreeSystem->RenderTrees(context->camera, context->light,
                                        context->viewMatrix, context->projMatrix);
        }

        // Skeletal character
        if (skinnedShader && skeletalAnimator && characterMesh) {
            // Update animation
            skeletalAnimator->Update(context->totalTime);

            // Update wandering
            static float lastUpdateTime = 0.0f;
            float deltaTime = context->totalTime - lastUpdateTime;
            if (deltaTime > 0.0f && deltaTime < RenderConfig::CHARACTER_MAX_DELTA_TIME) {
                skeletalAnimator->UpdateWandering(deltaTime,
                    RenderConfig::CHARACTER_WANDER_MIN_X, RenderConfig::CHARACTER_WANDER_MAX_X,
                    RenderConfig::CHARACTER_WANDER_MIN_Z, RenderConfig::CHARACTER_WANDER_MAX_Z);
            }
            lastUpdateTime = context->totalTime;

            glUseProgram(skinnedShader->GetProgram());

            // Get character position from animator
            Vector3 charPos = skeletalAnimator->GetPosition();
            float characterY = customTerrain
                ? customTerrain->GetHeightAt(charPos.x, charPos.z) + RenderConfig::CHARACTER_GROUND_OFFSET
                : charPos.y;
            float characterRotation = skeletalAnimator->GetRotation();

            Matrix4 characterModel = Matrix4::Translation(Vector3(charPos.x, characterY, charPos.z)) *
                                    Matrix4::Rotation(characterRotation, Vector3(0, 1, 0)) *
                                    Matrix4::Scale(Vector3(RenderConfig::CHARACTER_SCALE,
                                                          RenderConfig::CHARACTER_SCALE,
                                                          RenderConfig::CHARACTER_SCALE));
            glUniformMatrix4fv(glGetUniformLocation(skinnedShader->GetProgram(), "modelMatrix"),
                              1, false, (float*)&characterModel);
            glUniformMatrix4fv(glGetUniformLocation(skinnedShader->GetProgram(), "viewMatrix"),
                              1, false, (float*)&context->viewMatrix);
            glUniformMatrix4fv(glGetUniformLocation(skinnedShader->GetProgram(), "projMatrix"),
                              1, false, (float*)&context->projMatrix);
            SetForwardLightUniforms(skinnedShader);

            glUniform1f(glGetUniformLocation(skinnedShader->GetProgram(), "time"), context->totalTime);

            // Upload joint matrices
            const std::vector<Matrix4>& frameMatrices = skeletalAnimator->GetFrameMatrices();
            int jointCount = (int)frameMatrices.size();
            if (jointCount > RenderConfig::MAX_JOINT_COUNT) {
                jointCount = RenderConfig::MAX_JOINT_COUNT;
            }
            if (jointCount > 0) {
                glUniformMatrix4fv(glGetUniformLocation(skinnedShader->GetProgram(), "joints[0]"),
                                  jointCount, false, (float*)frameMatrices.data());
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, characterTexture);
            glUniform1i(glGetUniformLocation(skinnedShader->GetProgram(), "diffuseTex"), 0);

            characterMesh->Draw();
            glUseProgram(0);
        }

        // Sun/Moon
        if (sunShader && sunQuad && dayNightCycle) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glUseProgram(sunShader->GetProgram());

            Vector3 sunDir = dayNightCycle->GetSunDirection();
            Vector3 sceneCenter(RenderConfig::SCENE_CENTER_X, 0.0f, RenderConfig::SCENE_CENTER_Z);
            Vector3 sunPosition = sceneCenter + (sunDir * RenderConfig::SUN_ORBIT_RADIUS);

            Matrix4 model = Matrix4::Translation(sunPosition) *
                           Matrix4::Scale(Vector3(RenderConfig::SUN_MESH_SCALE,
                                                 RenderConfig::SUN_MESH_SCALE,
                                                 RenderConfig::SUN_MESH_SCALE));

            glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "modelMatrix"),
                              1, false, (float*)&model);
            glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "viewMatrix"),
                              1, false, (float*)&context->viewMatrix);
            glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "projMatrix"),
                              1, false, (float*)&context->projMatrix);

            Vector4 sunColor = dayNightCycle->GetSunColor();
            glUniform4fv(glGetUniformLocation(sunShader->GetProgram(), "sunColor"),
                        1, (float*)&sunColor);
            glUniform1f(glGetUniformLocation(sunShader->GetProgram(), "timeOfDay"),
                       dayNightCycle->GetTimeOfDay());

            sunQuad->Draw();

            glDepthFunc(GL_LESS);
            glUseProgram(0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

private:
    // Set common lighting uniforms for forward-rendered objects
    void SetForwardLightUniforms(Shader* shader) {
        if (!shader || !context->light || !context->camera) return;
        GLuint prog = shader->GetProgram();
        glUniform3fv(glGetUniformLocation(prog, "lightPos"),
                    1, (float*)&context->light->GetPosition());
        glUniform4fv(glGetUniformLocation(prog, "lightColour"),
                    1, (float*)&context->light->GetColour());
        glUniform3fv(glGetUniformLocation(prog, "cameraPos"),
                    1, (float*)&context->camera->GetPosition());
        glUniform1f(glGetUniformLocation(prog, "lightRadius"),
                   context->light->GetRadius());
    }

    // Set up scene shader uniforms (shadow, cubemap, lighting) matching ScenePass::DrawSceneGraph
    void SetSceneShaderUniforms(Shader* shader) {
        if (!shader) return;
        GLuint prog = shader->GetProgram();

        // Texture unit assignments
        glUniform1i(glGetUniformLocation(prog, "diffuseTex"),
                    RenderConfig::TEXTURE_UNIT_DIFFUSE);
        glUniform1i(glGetUniformLocation(prog, "shadowTex"),
                    RenderConfig::TEXTURE_UNIT_SHADOW);
        glUniform1i(glGetUniformLocation(prog, "cubeTex"),
                    RenderConfig::TEXTURE_UNIT_CUBEMAP);

        // Bind shadow map
        glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_SHADOW);
        glBindTexture(GL_TEXTURE_2D, context->shadowTexture);

        // Bind cubemap (skybox for reflections)
        glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_CUBEMAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxDay);

        // Shadow matrix
        glUniformMatrix4fv(glGetUniformLocation(prog, "shadowMatrix"),
                          1, false, context->shadowMatrix.values);

        // Reflectivity
        glUniform1f(glGetUniformLocation(prog, "reflectivity"),
                    RenderConfig::DEFAULT_REFLECTIVITY);

        // Lighting uniforms
        SetForwardLightUniforms(shader);
    }

    // Recursively draw scene nodes (no era filtering)
    void DrawSceneNode(SceneNode* node, Shader* shader) {
        if (!node) return;

        if (node->GetMesh()) {
            Matrix4 model = node->GetWorldTransform() *
                           Matrix4::Scale(node->GetModelScale());

            glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "modelMatrix"),
                              1, false, (float*)&model);
            glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "viewMatrix"),
                              1, false, (float*)&context->viewMatrix);
            glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "projMatrix"),
                              1, false, (float*)&context->projMatrix);

            glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_DIFFUSE);
            glBindTexture(GL_TEXTURE_2D, node->GetTexture());

            glUniform4fv(glGetUniformLocation(shader->GetProgram(), "nodeColour"),
                        1, (float*)&node->GetColour());

            node->GetMesh()->Draw();
        }

        for (SceneNode* child : node->GetChildren()) {
            DrawSceneNode(child, shader);
        }
    }

    // Recursively draw scene nodes with era filtering (matching ScenePass::BuildNodeLists)
    void DrawSceneNodeFiltered(SceneNode* node, Shader* shader) {
        if (!node) return;

        // Skip nodes that don't exist in current era
        if (!node->ExistsInEra(context->isModern)) {
            return;
        }

        if (node->GetMesh()) {
            Matrix4 model = node->GetWorldTransform() *
                           Matrix4::Scale(node->GetModelScale());

            glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "modelMatrix"),
                              1, false, (float*)&model);
            glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "viewMatrix"),
                              1, false, (float*)&context->viewMatrix);
            glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "projMatrix"),
                              1, false, (float*)&context->projMatrix);

            glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_DIFFUSE);
            glBindTexture(GL_TEXTURE_2D, node->GetTexture());

            glUniform4fv(glGetUniformLocation(shader->GetProgram(), "nodeColour"),
                        1, (float*)&node->GetColour());

            node->GetMesh()->Draw();
        }

        for (SceneNode* child : node->GetChildren()) {
            DrawSceneNodeFiltered(child, shader);
        }
    }
    DeferredRenderer* deferredRenderer;  // Not owned

    // Terrain resources
    Shader* terrainShader;
    CustomTerrain* terrain;
    GLuint terrainTex, terrainSandTex, terrainRockTex, terrainMudTex;

    // Skybox resources
    Shader* skyboxShader;
    Mesh* skyboxMesh;
    GLuint skyboxDay, skyboxNight, skyboxSunset;

    // Other renderers
    GrassRenderer* grassRenderer;
    WaterRenderer* waterRenderer;

    // Scene graph resources
    Shader* sceneShader;

    // GLTF resources
    Shader* gltfShader;
    GLTFScene* gltfScene;
    GLuint templeRoofTex, templeWallTex, templeWoodTex, templeFloorTex;

    // Ruins resources
    SceneNode* ruinsRoot;

    // Skeletal animation resources
    Shader* skinnedShader;
    SkeletalAnimator* skeletalAnimator;
    Mesh* characterMesh;
    GLuint characterTexture;

    // Sun/Moon resources
    Shader* sunShader;
    Mesh* sunQuad;
    DayNightCycle* dayNightCycle;

    // Voxel tree system
    VoxelTreeSystem* voxelTreeSystem;

    // Terrain for height sampling
    CustomTerrain* customTerrain;
};
