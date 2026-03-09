#pragma once

#include "RenderPass.h"
#include <vector>
#include <memory>
#include <algorithm>

// ========================================
// RENDER PIPELINE
// ========================================
// Manages a dynamic list of render passes
// Passes can be added, removed, reordered, and enabled/disabled at runtime

class RenderPipeline {
public:
    RenderPipeline() {}
    ~RenderPipeline() {
        // Clean up all passes
        passes.clear();
    }

    // Add a render pass to the end of the pipeline
    // Pipeline takes ownership of the pass (unique_ptr)
    void AddPass(std::unique_ptr<RenderPass> pass) {
        if (pass) {
            passes.push_back(std::move(pass));
        }
    }

    // Remove a pass by name
    void RemovePass(const std::string& name) {
        auto it = std::find_if(passes.begin(), passes.end(),
            [&name](const std::unique_ptr<RenderPass>& pass) {
                return pass && pass->GetName() == name;
            });

        if (it != passes.end()) {
            passes.erase(it);
        }
    }

    // Get a pass by name (for configuration)
    RenderPass* GetPass(const std::string& name) {
        for (auto& pass : passes) {
            if (pass && pass->GetName() == name) {
                return pass.get();
            }
        }
        return nullptr;
    }

    // Enable/disable a pass without removing it
    void SetPassEnabled(const std::string& name, bool enabled) {
        RenderPass* pass = GetPass(name);
        if (pass) {
            pass->SetEnabled(enabled);
        }
    }

    // Reorder passes: move pass to a new index
    void MovePass(const std::string& name, size_t newIndex) {
        auto it = std::find_if(passes.begin(), passes.end(),
            [&name](const std::unique_ptr<RenderPass>& pass) {
                return pass && pass->GetName() == name;
            });

        if (it != passes.end() && newIndex < passes.size()) {
            auto pass = std::move(*it);
            passes.erase(it);
            passes.insert(passes.begin() + newIndex, std::move(pass));
        }
    }

    // Execute all enabled passes in order
    void Execute(OGLRenderer& renderer) {
        for (auto& pass : passes) {
            if (pass && pass->IsEnabled()) {
                pass->Execute(renderer);
            }
        }
    }

    // Initialize all passes (called once after all passes are added)
    void Initialize(OGLRenderer& renderer) {
        for (auto& pass : passes) {
            if (pass) {
                pass->Initialize(renderer);
            }
        }
    }

    // Get number of passes
    size_t GetPassCount() const { return passes.size(); }

    // Clear all passes
    void Clear() {
        passes.clear();
    }

    // Debug: print pipeline configuration
    void PrintPipeline() const {
        // Debug output disabled for release
    }

private:
    std::vector<std::unique_ptr<RenderPass>> passes;
};
