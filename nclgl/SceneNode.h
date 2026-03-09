#pragma once

#include "Matrix4.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Mesh.h"
#include "Shader.h"
#include <vector>
#include <memory>

class SceneNode {
public:
    SceneNode(Mesh* m = nullptr, Vector4 colour = Vector4(1, 1, 1, 1));
    ~SceneNode(void);

    // Set transform properties
    void SetTransform(const Matrix4& matrix) { transform = matrix; }
    const Matrix4& GetTransform() const { return transform; }
    Matrix4 GetWorldTransform() const { return worldTransform; }

    // Set colour
    Vector4 GetColour() const { return colour; }
    void SetColour(const Vector4& c) { colour = c; }

    // Set mesh
    Mesh* GetMesh() const { return mesh; }
    void SetMesh(Mesh* m) { mesh = m; }

    // Set shader
    Shader* GetShader() const { return shader; }
    void SetShader(Shader* s) { shader = s; }

    // Set texture
    GLuint GetTexture() const { return texture; }
    void SetTexture(GLuint tex) { texture = tex; }

    // ========================================
    // DUAL-RESOURCE SYSTEM (Addressing 批评.md Section 2.2)
    // ========================================
    // Instead of maintaining two separate scene graphs (memory wasteful + hard to sync),
    // we store two sets of resources in EACH node and switch between them.
    // This ensures "same place, different time" rather than "two different worlds".
    //
    // CRITICAL FIX (批评.md): Now supports nodes that don't exist in certain eras.
    // Example: Ancient temple's incense burner shouldn't exist in modern ruins.
    // Example: Modern graffiti wall shouldn't exist in ancient times.

    // Era existence flags (bitmask: bit 0 = ancient, bit 1 = modern)
    enum EraFlags {
        EXISTS_IN_ANCIENT = 1 << 0,
        EXISTS_IN_MODERN = 1 << 1,
        EXISTS_IN_BOTH = EXISTS_IN_ANCIENT | EXISTS_IN_MODERN
    };

    // Set which eras this node exists in
    void SetEraExistence(int flags) { eraFlags = flags; }
    bool ExistsInEra(bool isModern) const {
        return (eraFlags & (isModern ? EXISTS_IN_MODERN : EXISTS_IN_ANCIENT)) != 0;
    }

    // Set dual meshes for ancient/modern worlds
    void SetDualMeshes(Mesh* ancient, Mesh* modern) {
        mesh_Ancient = ancient;
        mesh_Modern = modern;
        mesh = ancient;  // Default to ancient
    }

    // Set dual textures for ancient/modern worlds
    void SetDualTextures(GLuint ancient, GLuint modern) {
        tex_Ancient = ancient;
        tex_Modern = modern;
        texture = ancient;  // Default to ancient
    }

    // Recursively update all nodes to show a specific world state
    void UpdateWorldState(bool isModern) {
        // Switch resources if dual resources are available
        if (mesh_Ancient && mesh_Modern) {
            mesh = isModern ? mesh_Modern : mesh_Ancient;
        }
        if (tex_Ancient != 0 && tex_Modern != 0) {
            texture = isModern ? tex_Modern : tex_Ancient;
        }

        // Recursively update all children
        for (auto& child : children) {
            child->UpdateWorldState(isModern);
        }
    }

    // Set model scale
    Vector3 GetModelScale() const { return modelScale; }
    void SetModelScale(const Vector3& s) { modelScale = s; }

    // Bounding sphere for distance culling
    float GetBoundingRadius() const { return boundingRadius; }
    void SetBoundingRadius(float r) { boundingRadius = r; }

    // Camera distance (for transparency sorting)
    float GetCameraDistance() const { return distanceFromCamera; }
    void SetCameraDistance(float d) { distanceFromCamera = d; }

    // Transparency
    bool IsTransparent() const { return colour.w < 1.0f; }

    // Child management
    // Note: AddChild takes ownership of the pointer
    void AddChild(SceneNode* s);
    // RemoveChild returns ownership to caller
    SceneNode* RemoveChild(SceneNode* s);

    // Draw this node and all children
    virtual void Update(float dt);
    virtual void Draw(const OGLRenderer& r);

    // Get all children (raw pointers for iteration)
    std::vector<SceneNode*> GetChildren() const {
        std::vector<SceneNode*> result;
        result.reserve(children.size());
        for (const auto& child : children) {
            result.push_back(child.get());
        }
        return result;
    }

    size_t GetChildCount() const {
        return children.size();
    }

    // Static comparison for sorting transparent objects
    static bool CompareByCameraDistance(SceneNode* a, SceneNode* b) {
        return a->distanceFromCamera < b->distanceFromCamera;
    }

protected:
    SceneNode* parent;
    Mesh* mesh;
    Shader* shader;
    GLuint texture;

    // Dual-resource support for Two-Worlds system
    // Allows switching between ancient/modern resources without duplicating scene graph
    Mesh* mesh_Ancient;
    Mesh* mesh_Modern;
    GLuint tex_Ancient;
    GLuint tex_Modern;

    // Era existence flags (NEW: addressing 批评.md Section 2.2)
    int eraFlags;

    Matrix4 worldTransform;
    Matrix4 transform;
    Vector3 modelScale;
    Vector4 colour;

    float boundingRadius;
    float distanceFromCamera;

    std::vector<std::unique_ptr<SceneNode>> children;
};
