#include "GrassRenderer.h"
#include "RenderConfig.h"  // For centralized constants
#include <iostream>
#include <algorithm>

// #define GRASS_VERBOSE
#ifdef GRASS_VERBOSE
    #define GRASS_LOG(x) std::cout << x << std::endl
#else
    #define GRASS_LOG(x)
#endif

GrassRenderer::GrassRenderer(OGLRenderer* renderer)
    : renderer(renderer), enabled(true),
      grassTexture(0), grassVAO(0), grassVertexVBO(0), grassInstanceVBO(0),
      grassInstanceCount(0), densityMultiplier(1.0f), baseInstanceCount(0) {
    // grassShader is automatically initialized to nullptr by unique_ptr
}

GrassRenderer::~GrassRenderer() {
    if (grassVAO) {
        glDeleteVertexArrays(1, &grassVAO);
    }
    if (grassVertexVBO) {
        glDeleteBuffers(1, &grassVertexVBO);
    }
    if (grassInstanceVBO) {
        glDeleteBuffers(1, &grassInstanceVBO);
    }
    if (grassTexture) {
        glDeleteTextures(1, &grassTexture);
    }
    // grassShader is automatically deleted by unique_ptr
}

bool GrassRenderer::Initialize(CustomTerrain* heightMap, int patchCount, int bladesPerPatch) {
    // Load shader
    if (!LoadGrassShader()) {
        return false;
    }

    // Load grass texture with RGBA format (preserve alpha channel)
    GRASS_LOG("Loading grass texture (greenblade.png) with alpha channel...");
    grassTexture = SOIL_load_OGL_texture(
        TEXTUREDIR"greenblade.png",
        SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_MULTIPLY_ALPHA
    );

    if (!grassTexture) {
        std::cerr << "ERROR: Failed to load grass texture!" << std::endl;
        std::cerr << "  Attempted path: " << TEXTUREDIR"greenblade.png" << std::endl;
        std::cerr << "  SOIL Error: " << SOIL_last_result() << std::endl;

        // Try alternative path
        GRASS_LOG("  Trying alternative path: ../../../Textures/greenblade.png");
        grassTexture = SOIL_load_OGL_texture(
            "../../../Textures/greenblade.png",
            SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID,
            SOIL_FLAG_MIPMAPS | SOIL_FLAG_MULTIPLY_ALPHA
        );

        if (!grassTexture) {
            std::cerr << "  Alternative path also failed!" << std::endl;
            return false;
        } else {
            GRASS_LOG("  SUCCESS: Loaded from alternative path with alpha!");
        }
    } else {
        GRASS_LOG("Grass texture loaded successfully with alpha (ID: " << grassTexture << ")");
    }

    // Set texture parameters for grass
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Generate grass instances
    GenerateGrassInstances(heightMap, patchCount, bladesPerPatch);

    if (grassInstanceCount == 0) {
        std::cerr << "No grass instances generated!" << std::endl;
        return false;
    }

    // Create VAO/VBO
    CreateGrassVAO();

    GRASS_LOG("=== INSTANCED GRASS OPTIMIZATION ===");
    GRASS_LOG("  Old method: " << grassInstanceCount << " draw calls");
    GRASS_LOG("  New method: 1 draw call (instanced)");
    GRASS_LOG("  Performance gain: " << grassInstanceCount << "x faster!");

    return true;
}

bool GrassRenderer::LoadGrassShader() {
    grassShader = std::make_unique<Shader>("grass_instanced.vert", "grass_instanced.frag");

    if (!grassShader->LoadSuccess()) {
        std::cerr << "Failed to load instanced grass shader!" << std::endl;
        grassShader.reset();  // Explicitly reset (automatic cleanup)
        return false;
    }

    GRASS_LOG("Instanced grass shader loaded successfully!");
    return true;
}

void GrassRenderer::GenerateGrassInstances(CustomTerrain* heightMap, int patchCount, int bladesPerPatch) {
    GRASS_LOG("Generating instanced grass data...");

    grassInstances.clear();

    CustomTerrain* customTerrain = heightMap;
    Vector3 mapSize = customTerrain->GetTerrainSize();

    // CRITICAL FIX: Terrain coordinates are from 0 to mapSize, NOT -mapSize/2 to +mapSize/2!
    // Hill/Temple is at (1000, 1500) based on terrain generation (nz=0.75, nx=0.5)
    Vector3 hillCenter(mapSize.x * 0.5f, 0.0f, mapSize.z * 0.75f);
    float hillExclusionRadius = RenderConfig::GRASS_HILL_EXCLUSION_RADIUS;  // Exclude entire hill area

    // Basin/Lake parameters
    // Basin center: (0.25, 0.2) normalized = (500, 400) world coords
    Vector3 basinCenter(mapSize.x * 0.25f, 0.0f, mapSize.z * 0.2f);  // (500, 400)
    float basinRadius = mapSize.x * RenderConfig::GRASS_BASIN_RADIUS_MULTIPLIER;  // 360 units radius
    float basinExclusionRadius = basinRadius + RenderConfig::GRASS_BASIN_BUFFER;  // Exclude basin + buffer around it

    // Map edge exclusion
    float edgeMargin = RenderConfig::GRASS_EDGE_MARGIN;  // No grass within edge margin

    for (int patch = 0; patch < patchCount; ++patch) {
        // Random patch position within terrain bounds [0, mapSize]
        float patchX = (rand() % 1000) / 1000.0f * mapSize.x;
        float patchZ = (rand() % 1000) / 1000.0f * mapSize.z;

        // 1. Skip if near map edges
        if (patchX < edgeMargin || patchX > mapSize.x - edgeMargin ||
            patchZ < edgeMargin || patchZ > mapSize.z - edgeMargin) {
            continue;  // No grass near map edges
        }

        // 2. Skip if in hill/temple area (calculate distance to hill center)
        float dx_hill = patchX - hillCenter.x;
        float dz_hill = patchZ - hillCenter.z;
        float distToHill = sqrt(dx_hill * dx_hill + dz_hill * dz_hill);
        if (distToHill < hillExclusionRadius) {
            continue;  // No grass on hill or temple
        }

        // 3. Skip if in or near basin/lake area
        float dx_basin = patchX - basinCenter.x;
        float dz_basin = patchZ - basinCenter.z;
        float distToBasin = sqrt(dx_basin * dx_basin + dz_basin * dz_basin);
        if (distToBasin < basinExclusionRadius) {
            continue;  // No grass in basin (underwater) or around lake shore
        }

        // Get terrain height at patch position
        float patchY = customTerrain->GetHeightAt(patchX, patchZ);

        // 4. Skip any remaining high areas (failsafe for edges of hill)
        if (patchY > RenderConfig::GRASS_MAX_HEIGHT) {
            continue;  // Grass only on flat plains (height 0-40)
        }

        // 5. Skip very low areas (should be underwater or lake shore)
        if (patchY < RenderConfig::GRASS_MIN_HEIGHT) {
            continue;  // No grass below lake shore (water level is 10, shore is ~18)
        }

        // Generate grass blades in this patch
        for (int blade = 0; blade < bladesPerPatch; ++blade) {
            GrassInstanceData instance;

            // Random offset within patch (30 units for natural clustering)
            float offsetX = ((rand() % 100) / 100.0f - 0.5f) * RenderConfig::GRASS_PATCH_SPREAD;
            float offsetZ = ((rand() % 100) / 100.0f - 0.5f) * RenderConfig::GRASS_PATCH_SPREAD;

            instance.position = Vector3(patchX + offsetX, patchY, patchZ + offsetZ);
            instance.rotation = (rand() % static_cast<int>(RenderConfig::GRASS_ROTATION_RANGE)) * 1.0f;
            instance.scale = RenderConfig::GRASS_SCALE_MIN +
                           (rand() % static_cast<int>(RenderConfig::GRASS_SCALE_VARIATION * 100)) / 100.0f;
            instance.randomSeed = (rand() % 1000) / 1000.0f;

            grassInstances.push_back(instance);
        }
    }

    grassInstanceCount = grassInstances.size();
    baseInstanceCount = grassInstanceCount;  // Store original count for density adjustment
    GRASS_LOG("Generated " << patchCount << " patches, " << grassInstanceCount << " total grass blade instances.");
}

void GrassRenderer::CreateGrassVAO() {
    // Create VAO
    glGenVertexArrays(1, &grassVAO);
    glBindVertexArray(grassVAO);

    // Define quad vertex data (2 triangles forming a grass blade)
    // Very large grass blades: 16 units wide x 16 units tall
    float quadVertices[] = {
        // positions        // texcoords   // normals
        -RenderConfig::GRASS_BLADE_HALF_WIDTH, 0.0f, 0.0f,  0.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // bottom-left
        -RenderConfig::GRASS_BLADE_HALF_WIDTH, RenderConfig::GRASS_BLADE_HEIGHT, 0.0f,  0.0f, 0.0f,   0.0f, 0.0f, 1.0f,  // top-left
         RenderConfig::GRASS_BLADE_HALF_WIDTH, RenderConfig::GRASS_BLADE_HEIGHT, 0.0f,  1.0f, 0.0f,   0.0f, 0.0f, 1.0f,  // top-right
        -RenderConfig::GRASS_BLADE_HALF_WIDTH, 0.0f, 0.0f,  0.0f, 1.0f,   0.0f, 0.0f, 1.0f,  // bottom-left
         RenderConfig::GRASS_BLADE_HALF_WIDTH, RenderConfig::GRASS_BLADE_HEIGHT, 0.0f,  1.0f, 0.0f,   0.0f, 0.0f, 1.0f,  // top-right
         RenderConfig::GRASS_BLADE_HALF_WIDTH, 0.0f, 0.0f,  1.0f, 1.0f,   0.0f, 0.0f, 1.0f   // bottom-right
    };

    // Upload vertex VBO
    glGenBuffers(1, &grassVertexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, grassVertexVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Vertex attributes (per-vertex data)
    // location 0: position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // location 1: texCoord
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    // location 2: normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

    // Upload instance VBO
    glGenBuffers(1, &grassInstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, grassInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 grassInstanceCount * sizeof(GrassInstanceData),
                 grassInstances.data(),
                 GL_STATIC_DRAW);

    // Instance attributes (per-instance data)
    // location 3: instancePosition
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE,
                         sizeof(GrassInstanceData),
                         (void*)offsetof(GrassInstanceData, position));
    glVertexAttribDivisor(3, 1);  // Advance per instance

    // location 4: instanceRotation
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE,
                         sizeof(GrassInstanceData),
                         (void*)offsetof(GrassInstanceData, rotation));
    glVertexAttribDivisor(4, 1);

    // location 5: instanceScale
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE,
                         sizeof(GrassInstanceData),
                         (void*)offsetof(GrassInstanceData, scale));
    glVertexAttribDivisor(5, 1);

    // location 6: instanceRandomSeed
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE,
                         sizeof(GrassInstanceData),
                         (void*)offsetof(GrassInstanceData, randomSeed));
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);

    GRASS_LOG("Grass VAO/VBO created successfully!");
}

void GrassRenderer::SetDensityMultiplier(float multiplier) {
    // Clamp multiplier to valid range [0.0, 1.0]
    densityMultiplier = std::max(0.0f, std::min(1.0f, multiplier));

    // Calculate new instance count based on density
    // This reduces the number of grass blades drawn without regenerating geometry
    grassInstanceCount = static_cast<int>(baseInstanceCount * densityMultiplier);

    GRASS_LOG("GrassRenderer: Density adjusted to " << (densityMultiplier * 100.0f)
              << "% (" << grassInstanceCount << " / " << baseInstanceCount << " instances)");
}

void GrassRenderer::Render(Light* light, Camera* camera, float time, float windStrength, const Matrix4& viewMatrix, const Matrix4& projMatrix) {
    if (!enabled || grassInstanceCount == 0) {
        return;
    }

    // Setup OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);  // Grass blades need to be visible from both sides
    glEnable(GL_DEPTH_TEST);

    // Bind shader directly
    glUseProgram(grassShader->GetProgram());

    // CRITICAL FIX: Reset texture unit 0 to avoid shadow sampler pollution
    // Shadow map leaves GL_TEXTURE_COMPARE_MODE enabled, causing errors
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind any depth texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);  // Disable comparison

    // Bind grass texture on texture unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glUniform1i(glGetUniformLocation(grassShader->GetProgram(), "grassTex"), 1);

    // Set matrix uniforms
    glUniformMatrix4fv(glGetUniformLocation(grassShader->GetProgram(), "viewMatrix"),
                       1, false, viewMatrix.values);
    glUniformMatrix4fv(glGetUniformLocation(grassShader->GetProgram(), "projMatrix"),
                       1, false, projMatrix.values);

    // Set lighting uniforms
    glUniform3fv(glGetUniformLocation(grassShader->GetProgram(), "lightPos"),
                 1, (float*)&light->GetPosition());
    glUniform4fv(glGetUniformLocation(grassShader->GetProgram(), "lightColour"),
                 1, (float*)&light->GetColour());
    glUniform1f(glGetUniformLocation(grassShader->GetProgram(), "lightRadius"),
                light->GetRadius());
    glUniform3fv(glGetUniformLocation(grassShader->GetProgram(), "cameraPos"),
                 1, (float*)&camera->GetPosition());
    glUniform1f(glGetUniformLocation(grassShader->GetProgram(), "time"), time);
    glUniform1f(glGetUniformLocation(grassShader->GetProgram(), "windStrength"), windStrength);

    // SINGLE INSTANCED DRAW CALL FOR ALL GRASS!
    glBindVertexArray(grassVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, grassInstanceCount);
    glBindVertexArray(0);

    // Restore OpenGL state
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}
