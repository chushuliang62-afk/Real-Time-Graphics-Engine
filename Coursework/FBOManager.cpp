#include "FBOManager.h"
#include <iostream>

FBOManager::FBOManager(int width, int height, OGLRenderer* renderer)
    : width(width), height(height), renderer(renderer),
      sceneFBO(0), sceneColourTex(0), sceneDepthTex(0),
      brightPassFBO(0), brightPassTex(0),
      screenQuad(nullptr), postProcessShader(nullptr),
      bloomEnabled(true), bloomThreshold(0.8f), bloomIntensity(0.5f), bloomRadius(1.0f) {
    blurFBO[0] = blurFBO[1] = 0;
    blurTex[0] = blurTex[1] = 0;
}

FBOManager::~FBOManager() {
    if (sceneFBO) {
        glDeleteFramebuffers(1, &sceneFBO);
    }
    if (sceneColourTex) {
        glDeleteTextures(1, &sceneColourTex);
    }
    if (sceneDepthTex) {
        glDeleteTextures(1, &sceneDepthTex);
    }
    if (brightPassFBO) {
        glDeleteFramebuffers(1, &brightPassFBO);
    }
    if (brightPassTex) {
        glDeleteTextures(1, &brightPassTex);
    }
    if (blurFBO[0]) {
        glDeleteFramebuffers(2, blurFBO);
    }
    if (blurTex[0]) {
        glDeleteTextures(2, blurTex);
    }
    // screenQuad, postProcessShader, brightPassShader, blurShader, compositeShader
    // are automatically cleaned up by unique_ptr
}

bool FBOManager::Initialize() {
    // Generate FBO
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    // Create color texture
    glGenTextures(1, &sceneColourTex);
    glBindTexture(GL_TEXTURE_2D, sceneColourTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColourTex, 0);

    // Create depth texture
    glGenTextures(1, &sceneDepthTex);
    glBindTexture(GL_TEXTURE_2D, sceneDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTex, 0);

    // Check FBO completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "FBO Error: Framebuffer is not complete! Status: " << status << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create screen quad and load shaders
    CreateScreenQuad();
    if (!LoadPostProcessShader()) {
        return false;
    }

    // Load Bloom shaders and create Bloom FBOs
    if (!LoadBloomShaders()) {
        std::cerr << "Warning: Failed to load Bloom shaders, Bloom disabled" << std::endl;
        bloomEnabled = false;
    } else if (!CreateBloomFBOs()) {
        std::cerr << "Warning: Failed to create Bloom FBOs, Bloom disabled" << std::endl;
        bloomEnabled = false;
    }

    // FBOManager initialized
    return true;
}

void FBOManager::CreateScreenQuad() {
    screenQuad = std::unique_ptr<Mesh>(Mesh::CreateQuad());
    if (!screenQuad) {
        std::cerr << "Failed to create screen quad!" << std::endl;
    }
}

bool FBOManager::LoadPostProcessShader() {
    postProcessShader = std::make_unique<Shader>("copy.vert", "copy.frag");

    if (!postProcessShader->LoadSuccess()) {
        std::cerr << "Failed to load post-process shader!" << std::endl;
        postProcessShader.reset();
        return false;
    }

    return true;
}

void FBOManager::BindSceneFBO() {
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void FBOManager::RenderToScreen(bool isTransitioning, float transitionProgress) {
    if (bloomEnabled) {
        // Multi-pass Bloom pipeline
        RenderBrightPass();
        RenderBlur();
        RenderComposite(isTransitioning, transitionProgress);
    } else {
        // Fallback to simple copy shader
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        glUseProgram(postProcessShader->GetProgram());

        glUniform1i(glGetUniformLocation(postProcessShader->GetProgram(), "sceneTex"), 0);
        glUniform1f(glGetUniformLocation(postProcessShader->GetProgram(), "isTransitioning"), isTransitioning ? 1.0f : 0.0f);
        glUniform1f(glGetUniformLocation(postProcessShader->GetProgram(), "transitionProgress"), transitionProgress);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneColourTex);

        screenQuad->Draw();

        glEnable(GL_DEPTH_TEST);
    }

}

bool FBOManager::LoadBloomShaders() {
    brightPassShader = std::make_unique<Shader>("copy.vert", "brightPass.frag");
    if (!brightPassShader->LoadSuccess()) {
        std::cerr << "Failed to load bright pass shader!" << std::endl;
        brightPassShader.reset();
        return false;
    }

    blurShader = std::make_unique<Shader>("copy.vert", "gaussianBlur.frag");
    if (!blurShader->LoadSuccess()) {
        std::cerr << "Failed to load blur shader!" << std::endl;
        blurShader.reset();
        return false;
    }

    compositeShader = std::make_unique<Shader>("copy.vert", "bloomComposite.frag");
    if (!compositeShader->LoadSuccess()) {
        std::cerr << "Failed to load composite shader!" << std::endl;
        compositeShader.reset();
        return false;
    }

    return true;
}

bool FBOManager::CreateBloomFBOs() {
    // Create bright pass FBO
    glGenFramebuffers(1, &brightPassFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, brightPassFBO);

    glGenTextures(1, &brightPassTex);
    glBindTexture(GL_TEXTURE_2D, brightPassTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brightPassTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Bright pass FBO not complete!" << std::endl;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    // Create ping-pong blur FBOs
    glGenFramebuffers(2, blurFBO);
    glGenTextures(2, blurTex);

    for (int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i]);

        glBindTexture(GL_TEXTURE_2D, blurTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurTex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Blur FBO " << i << " not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // Bloom FBOs created
    return true;
}

void FBOManager::RenderBrightPass() {
    // Extract bright areas from scene
    glBindFramebuffer(GL_FRAMEBUFFER, brightPassFBO);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glUseProgram(brightPassShader->GetProgram());
    glUniform1i(glGetUniformLocation(brightPassShader->GetProgram(), "sceneTex"), 0);
    glUniform1f(glGetUniformLocation(brightPassShader->GetProgram(), "bloomThreshold"), bloomThreshold);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColourTex);

    screenQuad->Draw();
}

void FBOManager::RenderBlur() {
    // Two-pass separable Gaussian blur (ping-pong between FBOs)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glUseProgram(blurShader->GetProgram());
    glUniform1f(glGetUniformLocation(blurShader->GetProgram(), "blurRadius"), bloomRadius);

    bool horizontal = true;
    int passes = 4; // 2 horizontal + 2 vertical passes for smoother bloom

    for (int i = 0; i < passes; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, blurFBO[i % 2]);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform1i(glGetUniformLocation(blurShader->GetProgram(), "horizontal"), horizontal ? 1 : 0);
        glUniform1i(glGetUniformLocation(blurShader->GetProgram(), "inputTex"), 0);

        glActiveTexture(GL_TEXTURE0);
        if (i == 0) {
            // First pass: blur bright pass output
            glBindTexture(GL_TEXTURE_2D, brightPassTex);
        } else {
            // Subsequent passes: blur previous output
            glBindTexture(GL_TEXTURE_2D, blurTex[(i - 1) % 2]);
        }

        screenQuad->Draw();

        horizontal = !horizontal;
    }
}

void FBOManager::RenderComposite(bool isTransitioning, float transitionProgress) {
    // Composite scene + bloom to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    glUseProgram(compositeShader->GetProgram());

    // Set textures
    glUniform1i(glGetUniformLocation(compositeShader->GetProgram(), "sceneTex"), 0);
    glUniform1i(glGetUniformLocation(compositeShader->GetProgram(), "bloomTex"), 1);

    // Set bloom parameters
    glUniform1f(glGetUniformLocation(compositeShader->GetProgram(), "bloomIntensity"), bloomIntensity);

    // Set transition parameters
    glUniform1f(glGetUniformLocation(compositeShader->GetProgram(), "isTransitioning"), isTransitioning ? 1.0f : 0.0f);
    glUniform1f(glGetUniformLocation(compositeShader->GetProgram(), "transitionProgress"), transitionProgress);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sceneColourTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, blurTex[(4 - 1) % 2]); // Last blur output (4 passes)

    screenQuad->Draw();

    glEnable(GL_DEPTH_TEST);
}
