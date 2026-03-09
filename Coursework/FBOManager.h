#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Mesh.h"
#include <memory>

/**
 * FBOManager - Framebuffer Object Management
 *
 * Handles all FBO-related operations including:
 * - Scene rendering to texture
 * - Post-processing effects (Bloom)
 * - Time transition effects
 */
class FBOManager {
public:
    FBOManager(int width, int height, OGLRenderer* renderer);
    ~FBOManager();

    // Initialize FBO resources
    bool Initialize();

    // Bind FBO for rendering
    void BindSceneFBO();

    // Render FBO to screen with post-processing
    void RenderToScreen(bool isTransitioning, float transitionProgress);

    // Bloom controls
    void SetBloomEnabled(bool enabled) { bloomEnabled = enabled; }
    void SetBloomThreshold(float threshold) { bloomThreshold = threshold; }
    void SetBloomIntensity(float intensity) { bloomIntensity = intensity; }
    void SetBloomRadius(float radius) { bloomRadius = radius; }

    // Getters
    GLuint GetSceneFBO() const { return sceneFBO; }
    GLuint GetSceneColorTexture() const { return sceneColourTex; }
    GLuint GetSceneDepthTexture() const { return sceneDepthTex; }
    bool IsBloomEnabled() const { return bloomEnabled; }

private:
    // Screen dimensions
    int width;
    int height;

    // Reference to renderer for shader binding
    OGLRenderer* renderer;

    // FBO resources (main scene)
    GLuint sceneFBO;
    GLuint sceneColourTex;
    GLuint sceneDepthTex;

    // Bloom FBO resources
    GLuint brightPassFBO;
    GLuint brightPassTex;
    GLuint blurFBO[2];          // Ping-pong FBOs for blur
    GLuint blurTex[2];          // Ping-pong textures

    // Screen quad for post-processing (RAII)
    std::unique_ptr<Mesh> screenQuad;
    std::unique_ptr<Shader> postProcessShader;

    // Bloom shaders (RAII)
    std::unique_ptr<Shader> brightPassShader;
    std::unique_ptr<Shader> blurShader;
    std::unique_ptr<Shader> compositeShader;

    // Bloom parameters
    bool bloomEnabled;
    float bloomThreshold;       // Brightness threshold (default: 0.8)
    float bloomIntensity;       // Bloom strength (default: 0.5)
    float bloomRadius;          // Blur radius (default: 1.0)

    // Helper functions
    void CreateScreenQuad();
    bool LoadPostProcessShader();
    bool LoadBloomShaders();
    bool CreateBloomFBOs();
    void RenderBrightPass();
    void RenderBlur();
    void RenderComposite(bool isTransitioning, float transitionProgress);
};
