#include "ScenePass.h"
#include "RenderConfig.h"  // For centralized constants
#include <algorithm>
#include <cctype>  // For ::tolower
// No longer need Renderer.h - OGLRenderer provides all necessary APIs

// ========================================
// SCENE PASS IMPLEMENTATION
// ========================================

void ScenePass::Execute(OGLRenderer& renderer) {
    if (!enabled || !context->sceneFBO) {
        return;
    }

    // No need for dynamic_cast - OGLRenderer now provides public API for render passes

    // ========================================
    // DYNAMIC SKYBOX SELECTION BASED ON TIME OF DAY
    // ========================================
    currentSkybox = skyboxDay;  // Default to daytime

    if (dayNightCycle) {
        float timeOfDay = dayNightCycle->GetTimeOfDay();

        // Time periods for skybox selection:
        // 0:00-5:00   = Night (sky111 - dark with moon)
        // 5:00-7:00   = Sunrise (sky222 - purple/pink dawn)
        // 7:00-17:00  = Day (rusted - bright blue daytime)
        // 17:00-19:00 = Sunset (sky222 - purple/pink dusk)
        // 19:00-24:00 = Night (sky111 - dark with moon)

        if (timeOfDay < 5.0f || timeOfDay >= 19.0f) {
            // Night time
            currentSkybox = skyboxNight;
        }
        else if ((timeOfDay >= 5.0f && timeOfDay < 7.0f) || (timeOfDay >= 17.0f && timeOfDay < 19.0f)) {
            // Sunrise or sunset
            currentSkybox = skyboxSunset;
        }
        else {
            // Daytime (7:00-17:00)
            currentSkybox = skyboxDay;
        }
    }

    // Bind scene FBO
    glBindFramebuffer(GL_FRAMEBUFFER, context->sceneFBO);
    glViewport(0, 0, context->screenWidth, context->screenHeight);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    // Render skybox
    if (skyboxShader && skyboxMesh) {
        DrawSkybox(renderer);
    }

    // Render terrain
    if (terrainShader && customTerrain) {
        DrawTerrain(renderer);
    }

    // Render scene graph nodes
    if (sceneShader && context->sceneRoot) {
        DrawSceneGraph(renderer);
    }

    // Render GLTF temple (ancient era only)
    if (gltfShader && templeScene && !context->isModern) {
        DrawGLTFTemple(renderer);
    }

    // Render ruins (modern era only)
    if (sceneShader && ruinsRoot && context->isModern) {
        DrawRuins(renderer);
    }

    // Render grass
    if (grassRenderer) {
        grassRenderer->Render(context->light, context->camera, context->totalTime,
                            1.0f, context->viewMatrix, context->projMatrix);
    }

    // Render water
    if (waterRenderer) {
        // Use terrain texture as water base (will be subtle with bump mapping)
        waterRenderer->Render(context->light, context->camera, context->totalTime,
                            terrainTex, terrainTex, currentSkybox,
                            context->viewMatrix, context->projMatrix);
    }

    // Render voxel trees (Minecraft-style block trees)
    if (voxelTreeSystem) {
        voxelTreeSystem->RenderTrees(context->camera, context->light,
                                    context->viewMatrix, context->projMatrix);
    }

    // Render skeletal animated character
    if (skinnedShader && skeletalAnimator && characterMesh) {
        DrawSkeletalCharacter(renderer);
    }

    // Render sun/moon (world-space 3D sphere)
    if (sunShader && sunMesh && dayNightCycle) {
        DrawSun(renderer);
    }

    // NOTE: Particles are now rendered in PostProcessPass (after all post-processing)
    // This ensures transparent particles are rendered on top of the final image
    // without being affected by FBO alpha blending issues

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ScenePass::DrawSkybox(OGLRenderer& renderer) {
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    renderer.BindShader(skyboxShader);

    // Set identity model matrix for skybox
    renderer.SetModelMatrix(Matrix4());
    renderer.UpdateShaderMatrices();

    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_DIFFUSE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, currentSkybox);
    glUniform1i(glGetUniformLocation(skyboxShader->GetProgram(), "cubeTex"),
                RenderConfig::TEXTURE_UNIT_DIFFUSE);

    skyboxMesh->Draw();

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
}

void ScenePass::DrawTerrain(OGLRenderer& renderer) {
    renderer.BindShader(terrainShader);

    // Set camera and light uniforms using helper function (DRY principle)
    SetLightingUniforms(terrainShader);

    glUniform1i(glGetUniformLocation(terrainShader->GetProgram(), "diffuseTex"),
                RenderConfig::TEXTURE_UNIT_TERRAIN_BASE);
    glUniform1i(glGetUniformLocation(terrainShader->GetProgram(), "sandTex"),
                RenderConfig::TEXTURE_UNIT_TERRAIN_SAND);
    glUniform1i(glGetUniformLocation(terrainShader->GetProgram(), "rockTex"),
                RenderConfig::TEXTURE_UNIT_TERRAIN_ROCK);
    glUniform1i(glGetUniformLocation(terrainShader->GetProgram(), "mudTex"),
                RenderConfig::TEXTURE_UNIT_TERRAIN_MUD);
    glUniform1i(glGetUniformLocation(terrainShader->GetProgram(), "shadowTex"),
                RenderConfig::TEXTURE_UNIT_TERRAIN_SHADOW);

    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_TERRAIN_BASE);
    glBindTexture(GL_TEXTURE_2D, terrainTex);
    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_TERRAIN_SAND);
    glBindTexture(GL_TEXTURE_2D, terrainSandTex);
    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_TERRAIN_ROCK);
    glBindTexture(GL_TEXTURE_2D, terrainRockTex);
    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_TERRAIN_MUD);
    glBindTexture(GL_TEXTURE_2D, terrainMudTex);
    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_TERRAIN_SHADOW);
    glBindTexture(GL_TEXTURE_2D, context->shadowTexture);

    Matrix4 terrainModel;
    renderer.SetModelMatrix(terrainModel);
    glUniformMatrix4fv(glGetUniformLocation(terrainShader->GetProgram(), "shadowMatrix"),
                      1, false, context->shadowMatrix.values);

    renderer.UpdateShaderMatrices();

    customTerrain->Draw();
}

void ScenePass::DrawSceneGraph(OGLRenderer& renderer) {
    BuildNodeLists(context->sceneRoot);
    SortNodeLists();

    renderer.BindShader(sceneShader);

    // Set camera and light uniforms using helper function (DRY principle)
    SetLightingUniforms(sceneShader);

    glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"),
                RenderConfig::TEXTURE_UNIT_DIFFUSE);
    glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"),
                RenderConfig::TEXTURE_UNIT_SHADOW);
    glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "cubeTex"),
                RenderConfig::TEXTURE_UNIT_CUBEMAP);

    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_SHADOW);
    glBindTexture(GL_TEXTURE_2D, context->shadowTexture);
    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_CUBEMAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, currentSkybox);

    glUniformMatrix4fv(glGetUniformLocation(sceneShader->GetProgram(), "shadowMatrix"),
                      1, false, context->shadowMatrix.values);

    glUniform1f(glGetUniformLocation(sceneShader->GetProgram(), "reflectivity"),
                RenderConfig::DEFAULT_REFLECTIVITY);

    // Draw opaque objects
    for (const auto& n : nodeList) {
        DrawNode(n, renderer);
    }

    // Draw transparent objects (back-to-front)
    for (const auto& n : transparentNodeList) {
        DrawNode(n, renderer);
    }

    ClearNodeLists();
}

void ScenePass::DrawGLTFTemple(OGLRenderer& renderer) {
    if (!templeScene || templeScene->sceneNodes.empty()) return;

    // Disable blend for opaque temple rendering
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    renderer.BindShader(gltfShader);
    glUniform1i(glGetUniformLocation(gltfShader->GetProgram(), "diffuseTex"),
                RenderConfig::TEXTURE_UNIT_DIFFUSE);

    // Position the temple on terrain
    Vector3 templePos(RenderConfig::TEMPLE_POSITION_X, 0, RenderConfig::TEMPLE_POSITION_Z);
    templePos.y = RenderConfig::TEMPLE_POSITION_Y;

    // Base transformation for the entire temple
    Matrix4 baseTransform = Matrix4::Translation(templePos) *
                           Matrix4::Rotation(RenderConfig::TEMPLE_ROTATION_Y, Vector3(0, 1, 0)) *
                           Matrix4::Scale(Vector3(RenderConfig::TEMPLE_SCALE,
                                                 RenderConfig::TEMPLE_SCALE,
                                                 RenderConfig::TEMPLE_SCALE));

    glActiveTexture(GL_TEXTURE0);

    // Render using scene node hierarchy with proper transforms
    for (const GLTFNode& node : templeScene->sceneNodes) {
        if (!node.mesh) {
            continue;  // Skip nodes without mesh
        }

        // Combine base transform with node's world matrix
        renderer.SetModelMatrix(baseTransform * node.worldMatrix);
        renderer.UpdateShaderMatrices();

        // ========================================
        // PERFORMANCE OPTIMIZATION
        // ========================================
        // Use pre-computed materialType instead of expensive string operations every frame
        // Enum switch is orders of magnitude faster than string comparisons
        GLuint textureToUse = templeWallTex;  // Default

        switch (node.materialType) {
            case GLTFMaterialType::ROOF:
                textureToUse = templeRoofTex;
                break;
            case GLTFMaterialType::WOOD:
                textureToUse = templeWoodTex;
                break;
            case GLTFMaterialType::FLOOR:
                textureToUse = templeFloorTex;
                break;
            case GLTFMaterialType::WALL:
            case GLTFMaterialType::UNKNOWN:
            default:
                textureToUse = templeWallTex;
                break;
        }

        glBindTexture(GL_TEXTURE_2D, textureToUse);
        node.mesh->Draw();
    }

    // Re-enable blend for subsequent rendering
    glEnable(GL_BLEND);
}

void ScenePass::DrawRuins(OGLRenderer& renderer) {
    if (!ruinsRoot) return;

    // CRITICAL: Update worldTransform for all ruins nodes!
    // Without this, worldTransform is uninitialized and pillars appear at wrong positions
    ruinsRoot->Update(0.0f);

    // Build node lists using the same system as scene graph
    BuildNodeLists(ruinsRoot);
    SortNodeLists();

    // Ruins use the same scene shader as other scene graph objects
    renderer.BindShader(sceneShader);

    // Set camera and light uniforms using helper function (DRY principle)
    SetLightingUniforms(sceneShader);

    glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"),
                RenderConfig::TEXTURE_UNIT_DIFFUSE);
    glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "shadowTex"),
                RenderConfig::TEXTURE_UNIT_SHADOW);
    glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "cubeTex"),
                RenderConfig::TEXTURE_UNIT_CUBEMAP);

    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_SHADOW);
    glBindTexture(GL_TEXTURE_2D, context->shadowTexture);
    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_CUBEMAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, currentSkybox);

    glUniformMatrix4fv(glGetUniformLocation(sceneShader->GetProgram(), "shadowMatrix"),
                      1, false, context->shadowMatrix.values);

    glUniform1f(glGetUniformLocation(sceneShader->GetProgram(), "reflectivity"),
                RenderConfig::DEFAULT_REFLECTIVITY);

    // Draw ruins nodes (using same system as scene graph)
    for (const auto& n : nodeList) {
        DrawNode(n, renderer);
    }

    for (const auto& n : transparentNodeList) {
        DrawNode(n, renderer);
    }

    ClearNodeLists();
}

void ScenePass::DrawSkeletalCharacter(OGLRenderer& renderer) {
    if (!characterMesh || !skeletalAnimator) return;

    // Update animation
    skeletalAnimator->Update(context->totalTime);

    // Update wandering behavior (random exploration in grassland area)
    static float lastUpdateTime = 0.0f;
    float deltaTime = context->totalTime - lastUpdateTime;
    if (deltaTime > 0.0f && deltaTime < RenderConfig::CHARACTER_MAX_DELTA_TIME) {
        skeletalAnimator->UpdateWandering(deltaTime,
                                          RenderConfig::CHARACTER_WANDER_MIN_X,
                                          RenderConfig::CHARACTER_WANDER_MAX_X,
                                          RenderConfig::CHARACTER_WANDER_MIN_Z,
                                          RenderConfig::CHARACTER_WANDER_MAX_Z);
    }
    lastUpdateTime = context->totalTime;

    // Set up shader
    renderer.BindShader(skinnedShader);

    // Get character position from animator
    Vector3 charPos = skeletalAnimator->GetPosition();
    float characterY = customTerrain->GetHeightAt(charPos.x, charPos.z) +
                      RenderConfig::CHARACTER_GROUND_OFFSET;
    float characterRotation = skeletalAnimator->GetRotation();

    renderer.SetModelMatrix(Matrix4::Translation(Vector3(charPos.x, characterY, charPos.z)) *
                           Matrix4::Rotation(characterRotation, Vector3(0, 1, 0)) *
                           Matrix4::Scale(Vector3(RenderConfig::CHARACTER_SCALE,
                                                 RenderConfig::CHARACTER_SCALE,
                                                 RenderConfig::CHARACTER_SCALE)));
    renderer.UpdateShaderMatrices();

    // Set uniforms
    glUniform1f(glGetUniformLocation(skinnedShader->GetProgram(), "time"), context->totalTime);

    glUniform3fv(glGetUniformLocation(skinnedShader->GetProgram(), "cameraPos"),
                 1, (float*)&context->camera->GetPosition());
    glUniform3fv(glGetUniformLocation(skinnedShader->GetProgram(), "lightPos"),
                 1, (float*)&context->light->GetPosition());
    glUniform4fv(glGetUniformLocation(skinnedShader->GetProgram(), "lightColour"),
                 1, (float*)&context->light->GetColour());
    glUniform1f(glGetUniformLocation(skinnedShader->GetProgram(), "lightRadius"),
                context->light->GetRadius());

    glUniformMatrix4fv(glGetUniformLocation(skinnedShader->GetProgram(), "shadowMatrix"),
                      1, false, context->shadowMatrix.values);

    // Bind textures
    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_DIFFUSE);
    glBindTexture(GL_TEXTURE_2D, characterTexture);
    glUniform1i(glGetUniformLocation(skinnedShader->GetProgram(), "diffuseTex"),
                RenderConfig::TEXTURE_UNIT_DIFFUSE);

    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_SHADOW);
    glBindTexture(GL_TEXTURE_2D, context->shadowTexture);
    glUniform1i(glGetUniformLocation(skinnedShader->GetProgram(), "shadowTex"),
                RenderConfig::TEXTURE_UNIT_SHADOW);

    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_CUBEMAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, currentSkybox);
    glUniform1i(glGetUniformLocation(skinnedShader->GetProgram(), "cubeTex"),
                RenderConfig::TEXTURE_UNIT_CUBEMAP);

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

    characterMesh->Draw();
}

// Helper: Build node lists (for scene graph rendering)
void ScenePass::BuildNodeLists(SceneNode* from) {
    if (!from) return;

    // Check era existence for time period transitions
    if (!from->ExistsInEra(context->isModern)) {
        return;  // Skip nodes that don't exist in current era
    }

    if (from->GetColour().w < 1.0f) {
        transparentNodeList.push_back(from);
    } else {
        nodeList.push_back(from);
    }

    for (SceneNode* child : from->GetChildren()) {
        BuildNodeLists(child);
    }
}

void ScenePass::SortNodeLists() {
    std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(),
              SceneNode::CompareByCameraDistance);
}

void ScenePass::ClearNodeLists() {
    transparentNodeList.clear();
    nodeList.clear();
}

void ScenePass::DrawNode(SceneNode* n, OGLRenderer& renderer) {
    if (!n || !n->GetMesh()) return;

    Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());

    renderer.SetModelMatrix(model);
    renderer.UpdateShaderMatrices();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, n->GetTexture());

    glUniform4fv(glGetUniformLocation(sceneShader->GetProgram(), "nodeColour"),
                1, (float*)&n->GetColour());

    n->GetMesh()->Draw();
}

// ========================================
// Helper: Set common lighting uniforms (DRY principle)
// ========================================
// This function encapsulates the repeated lighting uniform setup
// that was duplicated across DrawTerrain, DrawSceneGraph, and DrawRuins
void ScenePass::SetLightingUniforms(Shader* shader) {
    if (!shader || !context || !context->camera || !context->light) {
        return;  // Safety check
    }

    GLuint program = shader->GetProgram();

    // Set camera position
    glUniform3fv(glGetUniformLocation(program, "cameraPos"),
                 1, (float*)&context->camera->GetPosition());

    // Set light properties
    glUniform3fv(glGetUniformLocation(program, "lightPos"),
                 1, (float*)&context->light->GetPosition());
    glUniform4fv(glGetUniformLocation(program, "lightColour"),
                 1, (float*)&context->light->GetColour());
    glUniform1f(glGetUniformLocation(program, "lightRadius"),
                context->light->GetRadius());
}

void ScenePass::DrawSun(OGLRenderer& renderer) {
    if (!sunShader || !sunMesh || !dayNightCycle) {
        return;
    }

    // Enable depth test so sun can be occluded by terrain/objects
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);  // Allow sun at far plane

    // Enable alpha blending for smooth edges
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderer.BindShader(sunShader);

    // Get sun direction from day-night cycle
    Vector3 sunDir = dayNightCycle->GetSunDirection();

    // Position sun/moon in world space (orbiting around scene center)
    Vector3 sceneCenter(RenderConfig::SCENE_CENTER_X, 0.0f, RenderConfig::SCENE_CENTER_Z);
    Vector3 sunPosition = sceneCenter + (sunDir * RenderConfig::SUN_ORBIT_RADIUS);

    Matrix4 model = Matrix4::Translation(sunPosition) *
                    Matrix4::Scale(Vector3(RenderConfig::SUN_MESH_SCALE,
                                          RenderConfig::SUN_MESH_SCALE,
                                          RenderConfig::SUN_MESH_SCALE));

    // Set shader uniforms
    glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "modelMatrix"),
                      1, false, (float*)&model);
    glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "viewMatrix"),
                      1, false, (float*)&context->viewMatrix);
    glUniformMatrix4fv(glGetUniformLocation(sunShader->GetProgram(), "projMatrix"),
                      1, false, (float*)&context->projMatrix);

    // Pass sun color and time of day
    Vector4 sunColor = dayNightCycle->GetSunColor();
    glUniform4fv(glGetUniformLocation(sunShader->GetProgram(), "sunColor"),
                1, (float*)&sunColor);
    glUniform1f(glGetUniformLocation(sunShader->GetProgram(), "timeOfDay"),
               dayNightCycle->GetTimeOfDay());

    // Draw sun/moon sphere
    sunMesh->Draw();

    // Restore depth function
    glDepthFunc(GL_LESS);
}
