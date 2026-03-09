#include "GBuffer.h"
#include <iostream>

// #define GBUFFER_VERBOSE
#ifdef GBUFFER_VERBOSE
    #define GBUFFER_LOG(x) std::cout << x << std::endl
#else
    #define GBUFFER_LOG(x)
#endif

GBuffer::GBuffer(int width, int height)
    : width(width)
    , height(height)
    , fbo(0)
    , positionTex(0)
    , normalTex(0)
    , albedoTex(0)
    , depthTex(0)
    , initialized(false)
{
}

GBuffer::~GBuffer() {
    CleanupResources();
}

bool GBuffer::Initialize() {
    if (initialized) {
        std::cerr << "[GBuffer] Already initialized!" << std::endl;
        return true;
    }

    GBUFFER_LOG("[GBuffer] Initializing " << width << "x" << height << " G-Buffer...");

    // Create textures and FBO
    if (!CreateTextures()) {
        std::cerr << "[GBuffer] ERROR: Failed to create textures!" << std::endl;
        CleanupResources();
        return false;
    }

    // Check FBO completeness
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[GBuffer] ERROR: Framebuffer incomplete! Status: 0x"
                  << std::hex << status << std::dec << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        CleanupResources();
        return false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    initialized = true;
    GBUFFER_LOG("[GBuffer] Successfully initialized!");
    GBUFFER_LOG("  - Position Texture (RGB16F): " << positionTex);
    GBUFFER_LOG("  - Normal Texture (RGB16F): " << normalTex);
    GBUFFER_LOG("  - Albedo Texture (RGBA8): " << albedoTex);
    GBUFFER_LOG("  - Depth Texture (D24): " << depthTex);

    return true;
}

bool GBuffer::CreateTextures() {
    // Generate FBO
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // 1. Position Texture (RGB16F - HDR)
    glGenTextures(1, &positionTex);
    glBindTexture(GL_TEXTURE_2D, positionTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, positionTex, 0);

    // 2. Normal Texture (RGB16F - High precision normals)
    glGenTextures(1, &normalTex);
    glBindTexture(GL_TEXTURE_2D, normalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTex, 0);

    // 3. Albedo + Specular Texture (RGBA8)
    glGenTextures(1, &albedoTex);
    glBindTexture(GL_TEXTURE_2D, albedoTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, albedoTex, 0);

    // 4. Depth Texture (DEPTH_COMPONENT24)
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

    // Specify render targets (3 color attachments)
    GLenum drawBuffers[3] = {
        GL_COLOR_ATTACHMENT0,  // Position
        GL_COLOR_ATTACHMENT1,  // Normal
        GL_COLOR_ATTACHMENT2   // Albedo + Specular
    };
    glDrawBuffers(3, drawBuffers);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return true;
}

void GBuffer::CleanupResources() {
    if (positionTex) {
        glDeleteTextures(1, &positionTex);
        positionTex = 0;
    }
    if (normalTex) {
        glDeleteTextures(1, &normalTex);
        normalTex = 0;
    }
    if (albedoTex) {
        glDeleteTextures(1, &albedoTex);
        albedoTex = 0;
    }
    if (depthTex) {
        glDeleteTextures(1, &depthTex);
        depthTex = 0;
    }
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }

    initialized = false;
}

void GBuffer::BindForGeometryPass() {
    if (!initialized) {
        std::cerr << "[GBuffer] ERROR: Not initialized! Cannot bind for geometry pass." << std::endl;
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, width, height);

    // Clear all color attachments and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
}

void GBuffer::BindForLightingPass(int positionUnit, int normalUnit, int albedoUnit) {
    if (!initialized) {
        std::cerr << "[GBuffer] ERROR: Not initialized! Cannot bind for lighting pass." << std::endl;
        return;
    }

    // Bind Position texture to specified unit
    glActiveTexture(GL_TEXTURE0 + positionUnit);
    glBindTexture(GL_TEXTURE_2D, positionTex);

    // Bind Normal texture to specified unit
    glActiveTexture(GL_TEXTURE0 + normalUnit);
    glBindTexture(GL_TEXTURE_2D, normalTex);

    // Bind Albedo texture to specified unit
    glActiveTexture(GL_TEXTURE0 + albedoUnit);
    glBindTexture(GL_TEXTURE_2D, albedoTex);

    // Reset to default texture unit
    glActiveTexture(GL_TEXTURE0);
}

void GBuffer::UnbindFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool GBuffer::Resize(int newWidth, int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) {
        std::cerr << "[GBuffer] ERROR: Invalid resize dimensions: "
                  << newWidth << "x" << newHeight << std::endl;
        return false;
    }

    if (newWidth == width && newHeight == height) {
        return true;  // Size unchanged
    }

    GBUFFER_LOG("[GBuffer] Resizing from " << width << "x" << height
              << " to " << newWidth << "x" << newHeight);

    // Cleanup old resources
    CleanupResources();

    // Update dimensions
    width = newWidth;
    height = newHeight;

    // Recreate
    return Initialize();
}
