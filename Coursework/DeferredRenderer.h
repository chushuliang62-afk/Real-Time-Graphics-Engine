#pragma once

/*
 * DeferredRenderer.h
 *
 * Deferred Rendering System
 *
 * Features:
 * - Manage deferred rendering pipeline (Geometry Pass + Lighting Pass)
 * - Support multiple point light sources
 * - Provide fullscreen quad for screen-space rendering
 * - Fully configurable (no hardcoded values)
 *
 * Architecture Design:
 * - Encapsulate G-Buffer management
 * - RAII resource management
 * - Dependency injection (Shaders provided externally)
 * - Follow existing Renderer architecture
 */

#include "GBuffer.h"
#include "PointLight.h"
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include <memory>
#include <vector>

class DeferredRenderer {
public:
    /**
     * Constructor
     * @param width Render target width
     * @param height Render target height
     */
    DeferredRenderer(int width, int height);

    /**
     * Destructor - RAII automatic cleanup
     */
    ~DeferredRenderer();

    /**
     * Initialize deferred rendering system
     * @return true on success, false on failure
     */
    bool Initialize();

    /**
     * Set geometry pass shader
     * @param shader Geometry pass shader (created by Renderer)
     */
    void SetGeometryShader(Shader* shader) { geometryShader = shader; }

    /**
     * Get geometry pass shader
     * @return Geometry pass shader pointer
     */
    Shader* GetGeometryShader() const { return geometryShader; }

    /**
     * Set lighting pass shader
     * @param shader Lighting pass shader (created by Renderer)
     */
    void SetLightingShader(Shader* shader) { lightingShader = shader; }

    /**
     * Begin geometry pass
     * Bind G-Buffer, prepare to render scene geometry
     */
    void BeginGeometryPass();

    /**
     * End geometry pass
     * Unbind G-Buffer
     */
    void EndGeometryPass();

    /**
     * Execute lighting pass
     * Calculate final lighting using G-Buffer data and point lights
     * @param camera Camera (used for view space position calculation)
     * @param lights Point light array
     * @param ambientColour Ambient light color
     * @param ambientIntensity Ambient light intensity
     * @param targetFBO Target framebuffer (0=default screen, other=custom FBO)
     */
    void ExecuteLightingPass(Camera* camera,
                            const std::vector<DeferredPointLight>& lights,
                            const Vector3& ambientColour,
                            float ambientIntensity,
                            GLuint targetFBO = 0,
                            const Vector3& sunDirection = Vector3(0, 1, 0),
                            const Vector3& sunColour = Vector3(1, 1, 1),
                            float sunIntensity = 1.0f);

    /**
     * Add point light
     * @param light Point light
     */
    void AddLight(const DeferredPointLight& light) { lights.push_back(light); }

    /**
     * Clear all point lights
     */
    void ClearLights() { lights.clear(); }

    /**
     * Get point light array (for external modification)
     */
    std::vector<DeferredPointLight>& GetLights() { return lights; }

    /**
     * Update all light animations
     * @param deltaTime Frame time (seconds)
     */
    void UpdateLights(float deltaTime);

    /**
     * Resize render target
     * @param width New width
     * @param height New height
     */
    bool Resize(int width, int height);

    /**
     * Get G-Buffer (for debug visualization)
     */
    GBuffer* GetGBuffer() { return gBuffer.get(); }

private:
    /**
     * Create fullscreen quad (for lighting pass)
     */
    bool CreateScreenQuad();

    /**
     * Render fullscreen quad
     */
    void RenderScreenQuad();

    /**
     * Cleanup OpenGL resources
     */
    void CleanupResources();

private:
    int width;
    int height;

    // G-Buffer management
    std::unique_ptr<GBuffer> gBuffer;

    // Shaders (provided by external Renderer)
    Shader* geometryShader;     // Geometry pass shader (does not own)
    Shader* lightingShader;     // Lighting pass shader (does not own)

    // Fullscreen quad (for lighting pass)
    GLuint screenQuadVAO;
    GLuint screenQuadVBO;

    // Point light array
    std::vector<DeferredPointLight> lights;

    bool initialized;
};
