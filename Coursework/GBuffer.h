#pragma once

/*
 * GBuffer.h
 *
 * Geometry Buffer for Deferred Rendering
 *
 * Features:
 * - Manage G-Buffer FBO and textures
 * - Store position, normal, albedo and depth information
 * - Support HDR rendering (RGB16F format)
 * - Provide binding interfaces for geometry pass and lighting pass
 *
 * Architecture Design:
 * - RAII resource management
 * - No hardcoded values (dimensions passed from external)
 * - Full encapsulation (hide OpenGL details)
 */

#include "../nclgl/OGLRenderer.h"
#include <iostream>

class GBuffer {
public:
    /**
     * Constructor
     * @param width G-Buffer width (usually screen width)
     * @param height G-Buffer height (usually screen height)
     */
    GBuffer(int width, int height);

    /**
     * Destructor - RAII automatic resource cleanup
     */
    ~GBuffer();

    /**
     * Initialize G-Buffer
     * @return true on success, false on failure
     */
    bool Initialize();

    /**
     * Bind FBO for geometry pass (write to G-Buffer)
     */
    void BindForGeometryPass();

    /**
     * Bind textures for lighting pass (read from G-Buffer)
     * @param positionUnit Position texture unit (default 0)
     * @param normalUnit Normal texture unit (default 1)
     * @param albedoUnit Albedo texture unit (default 2)
     */
    void BindForLightingPass(int positionUnit = 0, int normalUnit = 1, int albedoUnit = 2);

    /**
     * Unbind FBO, restore default framebuffer
     */
    void UnbindFramebuffer();

    /**
     * Resize G-Buffer (called when window size changes)
     * @param width New width
     * @param height New height
     * @return true on success, false on failure
     */
    bool Resize(int width, int height);

    // Getters - Get texture IDs
    GLuint GetPositionTexture() const { return positionTex; }
    GLuint GetNormalTexture() const { return normalTex; }
    GLuint GetAlbedoTexture() const { return albedoTex; }
    GLuint GetDepthTexture() const { return depthTex; }
    GLuint GetFBO() const { return fbo; }

    // Getters - Get dimensions
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

private:
    /**
     * Create all textures and FBO
     */
    bool CreateTextures();

    /**
     * Cleanup all OpenGL resources
     */
    void CleanupResources();

private:
    int width;
    int height;

    // OpenGL resources
    GLuint fbo;             // Framebuffer Object
    GLuint positionTex;     // Position (RGB16F) - World space position
    GLuint normalTex;       // Normal (RGB16F) - World space normal
    GLuint albedoTex;       // Albedo + Specular (RGBA8) - RGB: albedo, A: specular intensity
    GLuint depthTex;        // Depth (DEPTH_COMPONENT24) - Depth information

    bool initialized;
};
