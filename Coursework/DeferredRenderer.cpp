#include "DeferredRenderer.h"
#include <iostream>

// #define DEFERRED_VERBOSE
#ifdef DEFERRED_VERBOSE
    #define DEFERRED_LOG(x) std::cout << x << std::endl
#else
    #define DEFERRED_LOG(x)
#endif

DeferredRenderer::DeferredRenderer(int width, int height)
    : width(width)
    , height(height)
    , geometryShader(nullptr)
    , lightingShader(nullptr)
    , screenQuadVAO(0)
    , screenQuadVBO(0)
    , initialized(false)
{
}

DeferredRenderer::~DeferredRenderer() {
    CleanupResources();
}

bool DeferredRenderer::Initialize() {
    if (initialized) {
        std::cerr << "[DeferredRenderer] Already initialized!" << std::endl;
        return true;
    }

    DEFERRED_LOG("[DeferredRenderer] Initializing deferred rendering system...");

    // Create G-Buffer
    gBuffer = std::make_unique<GBuffer>(width, height);
    if (!gBuffer->Initialize()) {
        std::cerr << "[DeferredRenderer] ERROR: Failed to initialize G-Buffer!" << std::endl;
        return false;
    }

    // Create fullscreen quad
    if (!CreateScreenQuad()) {
        std::cerr << "[DeferredRenderer] ERROR: Failed to create screen quad!" << std::endl;
        CleanupResources();
        return false;
    }

    initialized = true;
    DEFERRED_LOG("[DeferredRenderer] Successfully initialized!");

    return true;
}

bool DeferredRenderer::CreateScreenQuad() {
    // Fullscreen quad vertices (NDC coordinates, covers entire screen)
    // Position (xy) + TexCoord (zw)
    float quadVertices[] = {
        // Positions   // TexCoords
        -1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
        -1.0f, -1.0f,  0.0f, 0.0f,  // Bottom-left
         1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right

        -1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
         1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right
         1.0f,  1.0f,  1.0f, 1.0f   // Top-right
    };

    glGenVertexArrays(1, &screenQuadVAO);
    glGenBuffers(1, &screenQuadVBO);

    glBindVertexArray(screenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    // TexCoord attribute (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    DEFERRED_LOG("[DeferredRenderer] Screen quad created (VAO: " << screenQuadVAO << ")");

    return true;
}

void DeferredRenderer::CleanupResources() {
    if (screenQuadVAO) {
        glDeleteVertexArrays(1, &screenQuadVAO);
        screenQuadVAO = 0;
    }
    if (screenQuadVBO) {
        glDeleteBuffers(1, &screenQuadVBO);
        screenQuadVBO = 0;
    }

    gBuffer.reset();
    initialized = false;
}

void DeferredRenderer::BeginGeometryPass() {
    if (!initialized || !gBuffer) {
        std::cerr << "[DeferredRenderer] ERROR: Not initialized! Cannot begin geometry pass." << std::endl;
        return;
    }

    // Bind G-Buffer for rendering
    gBuffer->BindForGeometryPass();

    // Geometry pass requires depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);  // Allow depth writes

    // No blending needed
    glDisable(GL_BLEND);
}

void DeferredRenderer::EndGeometryPass() {
    if (!initialized || !gBuffer) {
        std::cerr << "[DeferredRenderer] ERROR: Not initialized! Cannot end geometry pass." << std::endl;
        return;
    }

    // Unbind G-Buffer
    gBuffer->UnbindFramebuffer();
}

void DeferredRenderer::ExecuteLightingPass(Camera* camera,
                                          const std::vector<DeferredPointLight>& lights,
                                          const Vector3& ambientColour,
                                          float ambientIntensity,
                                          GLuint targetFBO,
                                          const Vector3& sunDirection,
                                          const Vector3& sunColour,
                                          float sunIntensity)
{
    if (!initialized || !gBuffer || !lightingShader) {
        std::cerr << "[DeferredRenderer] ERROR: Cannot execute lighting pass! "
                  << "initialized=" << initialized
                  << ", gBuffer=" << (gBuffer != nullptr)
                  << ", lightingShader=" << (lightingShader != nullptr) << std::endl;
        return;
    }

    // Bind target framebuffer (specified by caller, usually sceneFBO)
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glViewport(0, 0, width, height);

    // Clear target framebuffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Lighting pass doesn't need depth testing (fullscreen quad)
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Enable blending (for combining multiple light sources)
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);  // Additive blending for lights

    // Use lighting shader
    glUseProgram(lightingShader->GetProgram());

    // Bind G-Buffer textures
    gBuffer->BindForLightingPass(0, 1, 2);  // Position=0, Normal=1, Albedo=2

    // Pass G-Buffer texture units to shader
    glUniform1i(glGetUniformLocation(lightingShader->GetProgram(), "gPosition"), 0);
    glUniform1i(glGetUniformLocation(lightingShader->GetProgram(), "gNormal"), 1);
    glUniform1i(glGetUniformLocation(lightingShader->GetProgram(), "gAlbedoSpec"), 2);

    // Pass camera position
    if (camera) {
        Vector3 camPos = camera->GetPosition();
        glUniform3f(glGetUniformLocation(lightingShader->GetProgram(), "cameraPos"),
                   camPos.x, camPos.y, camPos.z);
    }

    // Pass ambient light
    glUniform3f(glGetUniformLocation(lightingShader->GetProgram(), "ambientColour"),
               ambientColour.x, ambientColour.y, ambientColour.z);
    glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), "ambientIntensity"),
               ambientIntensity);

    // Pass directional light (sun)
    glUniform3f(glGetUniformLocation(lightingShader->GetProgram(), "sunDirection"),
               sunDirection.x, sunDirection.y, sunDirection.z);
    glUniform3f(glGetUniformLocation(lightingShader->GetProgram(), "sunColour"),
               sunColour.x, sunColour.y, sunColour.z);
    glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), "sunIntensity"),
               sunIntensity);

    // Pass number of point lights
    int numLights = static_cast<int>(lights.size());
    glUniform1i(glGetUniformLocation(lightingShader->GetProgram(), "numLights"), numLights);

    // Pass each point light data (maximum 32 lights supported)
    int maxLights = std::min(numLights, 32);
    for (int i = 0; i < maxLights; ++i) {
        const DeferredPointLight& light = lights[i];

        std::string prefix = "lights[" + std::to_string(i) + "].";

        glUniform3f(glGetUniformLocation(lightingShader->GetProgram(), (prefix + "position").c_str()),
                   light.position.x, light.position.y, light.position.z);

        glUniform3f(glGetUniformLocation(lightingShader->GetProgram(), (prefix + "colour").c_str()),
                   light.colour.x, light.colour.y, light.colour.z);

        glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), (prefix + "intensity").c_str()),
                   light.currentIntensity);  // Use animated intensity

        glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), (prefix + "radius").c_str()),
                   light.radius);

        glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), (prefix + "constant").c_str()),
                   light.constant);

        glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), (prefix + "linear").c_str()),
                   light.linear);

        glUniform1f(glGetUniformLocation(lightingShader->GetProgram(), (prefix + "quadratic").c_str()),
                   light.quadratic);
    }

    // Render fullscreen quad
    RenderScreenQuad();

    // Restore state
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    // Copy G-Buffer depth to target FBO so forward-rendered objects have correct depth
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer->GetFBO());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, targetFBO);
    glBlitFramebuffer(0, 0, width, height, 0, 0, width, height,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
}

void DeferredRenderer::RenderScreenQuad() {
    glBindVertexArray(screenQuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void DeferredRenderer::UpdateLights(float deltaTime) {
    for (auto& light : lights) {
        light.UpdateAnimation(deltaTime);
    }
}

bool DeferredRenderer::Resize(int newWidth, int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) {
        std::cerr << "[DeferredRenderer] ERROR: Invalid resize dimensions: "
                  << newWidth << "x" << newHeight << std::endl;
        return false;
    }

    if (newWidth == width && newHeight == height) {
        return true;  // Size unchanged
    }

    DEFERRED_LOG("[DeferredRenderer] Resizing from " << width << "x" << height
              << " to " << newWidth << "x" << newHeight);

    width = newWidth;
    height = newHeight;

    // Recreate G-Buffer
    if (gBuffer) {
        return gBuffer->Resize(newWidth, newHeight);
    }

    return false;
}
