#include "SceneNode.h"

SceneNode::SceneNode(Mesh* mesh, Vector4 colour) {
    this->mesh = mesh;
    this->colour = colour;
    this->shader = nullptr;
    this->texture = 0;

    // Initialize dual-resource pointers
    this->mesh_Ancient = nullptr;
    this->mesh_Modern = nullptr;
    this->tex_Ancient = 0;
    this->tex_Modern = 0;

    // Default: node exists in both eras (addressing 批评.md Section 2.2)
    this->eraFlags = EXISTS_IN_BOTH;

    parent = nullptr;
    modelScale = Vector3(1, 1, 1);
    boundingRadius = 1.0f;
    distanceFromCamera = 0.0f;
}

SceneNode::~SceneNode(void) {
    // Children are now automatically deleted by unique_ptr
}

void SceneNode::AddChild(SceneNode* s) {
    children.push_back(std::unique_ptr<SceneNode>(s));
    s->parent = this;
}

SceneNode* SceneNode::RemoveChild(SceneNode* s) {
    for (auto it = children.begin(); it != children.end(); ++it) {
        if (it->get() == s) {
            s->parent = nullptr;
            SceneNode* result = it->release();
            children.erase(it);
            return result;
        }
    }
    return nullptr;
}

void SceneNode::Update(float dt) {
    if (parent) {
        worldTransform = parent->worldTransform * transform;
    }
    else {
        worldTransform = transform;
    }

    // Update all children
    for (auto& child : children) {
        child->Update(dt);
    }
}

void SceneNode::Draw(const OGLRenderer& r) {
    // Drawing is handled by the renderer
    // This is a placeholder for future extensions
}
