#include "WaterRenderer.h"
#include "RenderConfig.h"  // For centralized constants
#include <iostream>

// #define WATER_VERBOSE
#ifdef WATER_VERBOSE
    #define WATER_LOG(x) std::cout << x << std::endl
#else
    #define WATER_LOG(x)
#endif

WaterRenderer::WaterRenderer(OGLRenderer* renderer)
    : renderer(renderer), enabled(true),
      waterHeight(RenderConfig::DEFAULT_WATER_HEIGHT),
      waterSize(RenderConfig::DEFAULT_WATER_SIZE),
      waterNormalTex(0), waterDudvTex(0) {
    // waterMesh and waterShader are automatically initialized to nullptr by unique_ptr
}

WaterRenderer::~WaterRenderer() {
    // waterMesh and waterShader are automatically deleted by unique_ptr
    if (waterNormalTex) {
        glDeleteTextures(1, &waterNormalTex);
    }
    if (waterDudvTex) {
        glDeleteTextures(1, &waterDudvTex);
    }
}

bool WaterRenderer::Initialize(float waterHeight, float waterSize) {
    this->waterHeight = waterHeight;
    this->waterSize = waterSize;

    // Load shader
    if (!LoadWaterShader()) {
        return false;
    }

    // Load textures
    if (!LoadWaterTextures()) {
        return false;
    }

    // Create water mesh
    CreateWaterMesh(waterSize);

    WATER_LOG("WaterRenderer initialized successfully!");
    return true;
}

bool WaterRenderer::LoadWaterShader() {
    waterShader = std::make_unique<Shader>("water.vert", "water.frag");

    if (!waterShader->LoadSuccess()) {
        std::cerr << "Failed to load water shader!" << std::endl;
        waterShader.reset();  // Explicitly reset (automatic cleanup)
        return false;
    }

    return true;
}

bool WaterRenderer::LoadWaterTextures() {
    // Load water normal map from Textures/water folder
    // Try multiple possible filenames for flexibility
    const char* normalMapPaths[] = {
        TEXTUREDIR"water/waterNormal.tga",
        TEXTUREDIR"water/waterNormal.png",
        TEXTUREDIR"water/normal.tga",
        TEXTUREDIR"water/normal.png",
        TEXTUREDIR"water/water_normal.tga",
        TEXTUREDIR"water/water_normal.png",
        TEXTUREDIR"water/DOT3.tga",
        TEXTUREDIR"water/DOT3.png"
    };

    waterNormalTex = 0;
    bool normalLoaded = false;

    // Try loading normal map from each possible path
    for (const char* path : normalMapPaths) {
        waterNormalTex = SOIL_load_OGL_texture(
            path,
            SOIL_LOAD_AUTO,
            SOIL_CREATE_NEW_ID,
            SOIL_FLAG_MIPMAPS
        );

        if (waterNormalTex) {
            WATER_LOG("Water normal map loaded from: " << path);

            // Set texture parameters for normal map
            glBindTexture(GL_TEXTURE_2D, waterNormalTex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

            normalLoaded = true;
            break;
        }
    }

    if (!normalLoaded) {
        WATER_LOG("Water normal map not found in Textures/water/, using flat normals");
        WATER_LOG("  Expected files: waterNormal.tga/png, normal.tga/png, DOT3.tga/png");
    }

    // DUDV map is optional (not required for this implementation)
    waterDudvTex = 0;

    return true;  // Don't fail initialization if normal map is missing
}

void WaterRenderer::CreateWaterMesh(float size) {
    // Create a large quad for water surface
    waterMesh = std::unique_ptr<Mesh>(Mesh::CreateWaterQuad());

    if (!waterMesh) {
        std::cerr << "Failed to create water mesh!" << std::endl;
    }
}

void WaterRenderer::Render(Light* light, Camera* camera, float time, GLuint waterTexture, GLuint bumpTexture, GLuint skyboxCubemap, const Matrix4& viewMatrix, const Matrix4& projMatrix) {
    if (!enabled || !waterMesh) {
        return;
    }

    // Setup OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    // Bind shader directly
    glUseProgram(waterShader->GetProgram());

    // CRITICAL FIX: Reset texture unit 0 to avoid shadow sampler pollution
    // Shadow map leaves GL_TEXTURE_COMPARE_MODE enabled, causing errors
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);  // Unbind any depth texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);  // Disable comparison

    // Set model matrix (position and scale water plane)
    // Water fills basin at lower-left quadrant
    // Basin center: (0.25, 0.2) of 2000x2000 terrain = (500, 400) in world coords
    Vector3 waterCenter(RenderConfig::WATER_CENTER_X, waterHeight, RenderConfig::WATER_CENTER_Z);
    Matrix4 waterModel = Matrix4::Translation(waterCenter) *
                         Matrix4::Scale(Vector3(waterSize, 1.0f, waterSize));
    glUniformMatrix4fv(glGetUniformLocation(waterShader->GetProgram(), "modelMatrix"),
                       1, false, waterModel.values);

    // Set view/proj matrices
    glUniformMatrix4fv(glGetUniformLocation(waterShader->GetProgram(), "viewMatrix"),
                       1, false, viewMatrix.values);
    glUniformMatrix4fv(glGetUniformLocation(waterShader->GetProgram(), "projMatrix"),
                       1, false, projMatrix.values);

    // Set lighting uniforms
    glUniform3fv(glGetUniformLocation(waterShader->GetProgram(), "lightPos"),
                 1, (float*)&light->GetPosition());
    glUniform4fv(glGetUniformLocation(waterShader->GetProgram(), "lightColour"),
                 1, (float*)&light->GetColour());
    glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "lightRadius"),
                light->GetRadius());

    // Set camera position for fresnel effect
    glUniform3fv(glGetUniformLocation(waterShader->GetProgram(), "cameraPos"),
                 1, (float*)&camera->GetPosition());

    // Set time for wave animation
    glUniform1f(glGetUniformLocation(waterShader->GetProgram(), "time"), time);

    // Bind textures
    // Use texture units 4-6 to avoid conflicts with shadow map (unit 0) and other renderers
    // Texture unit for waterTex (base water color/texture)
    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_WATER);
    glBindTexture(GL_TEXTURE_2D, waterTexture);
    glUniform1i(glGetUniformLocation(waterShader->GetProgram(), "waterTex"),
                RenderConfig::TEXTURE_UNIT_WATER);

    // Texture unit for waterBumpTex (normal/bump map)
    // Use our loaded normal map if available, otherwise fall back to parameter
    GLuint normalMapToUse = waterNormalTex != 0 ? waterNormalTex : bumpTexture;
    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_WATER_BUMP);
    glBindTexture(GL_TEXTURE_2D, normalMapToUse);
    glUniform1i(glGetUniformLocation(waterShader->GetProgram(), "waterBumpTex"),
                RenderConfig::TEXTURE_UNIT_WATER_BUMP);

    // Texture unit for cubeTex (skybox for reflections)
    glActiveTexture(GL_TEXTURE0 + RenderConfig::TEXTURE_UNIT_WATER_CUBEMAP);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxCubemap);
    glUniform1i(glGetUniformLocation(waterShader->GetProgram(), "cubeTex"),
                RenderConfig::TEXTURE_UNIT_WATER_CUBEMAP);

    // Draw water mesh
    waterMesh->Draw();

    // Restore OpenGL state
    glDisable(GL_BLEND);
}
