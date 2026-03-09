#pragma once
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <iostream>

#include "../Matrix4.h"
#include "../Vector3.h"
#include "../Vector4.h"

#include "OGLTexture.h"
#include "../Mesh.h"
#include "../MeshAnimation.h"

using OGLMesh = ::Mesh;

using UniqueMesh = std::unique_ptr<OGLMesh>;
using SharedMesh = std::shared_ptr<OGLMesh>;

using UniqueMeshAnim = std::unique_ptr<class MeshAnimation>;
using SharedMeshAnim = std::shared_ptr<class MeshAnimation>;

using UniqueTexture = std::unique_ptr<class OGLTexture>;
using SharedTexture = std::shared_ptr<class OGLTexture>;

namespace tinygltf {
	class Model;
}

class MeshAnimation;
class Texture;

enum GLTFAlphaMode {
	Opaque,
	Mask,
	Cutoff
};

// ========================================
// Material Type Enum for Performance Optimization
// ========================================
// Pre-computed material type based on node name
// Avoids expensive string operations every frame (批评.md Section 4.1)
enum class GLTFMaterialType {
	UNKNOWN,     // Default/unknown material
	ROOF,        // Roof tiles
	WALL,        // Stone walls
	WOOD,        // Wooden columns/pillars
	FLOOR        // Stone floor
};

struct GLTFMaterialLayer {
	SharedTexture albedo;
	SharedTexture bump;
	SharedTexture occlusion;
	SharedTexture emission;
	SharedTexture metallic;

	Vector4 albedoColour	= Vector4(1, 1, 1, 1);
	Vector3 emissionColour	= Vector3(0, 0, 0);
	float	metallicFactor	= 0.0f;
	float	roughnessFactor = 1.0f;
	float	alphaCutoff		= 0.5f;
	bool	doubleSided		= false;

	GLTFAlphaMode alphaMode = GLTFAlphaMode::Opaque;
	std::string name = "Unnamed Material Layer";
};

struct GLTFMaterial {
	std::vector< GLTFMaterialLayer > allLayers;
};		

struct GLTFNode {
	std::string name;
	uint32_t nodeID = 0;

	::Mesh* mesh = nullptr;
	//GLTFMaterial* material = nullptr;

	Matrix4 localMatrix;
	Matrix4 worldMatrix;

	int32_t parent = -1;
	std::vector<int32_t> children;

	// Pre-computed material type (initialized during load, not every frame)
	// This eliminates the need for string operations in the render loop
	GLTFMaterialType materialType = GLTFMaterialType::UNKNOWN;
};	

struct GLTFScene {
	std::vector<SharedMesh>			meshes;		
	std::vector<SharedMeshAnim>		animations;
	std::vector<SharedTexture>		textures;

	std::vector<GLTFMaterial>		materials;
	std::vector<GLTFMaterialLayer>	materialLayers;

	std::vector<GLTFNode>			sceneNodes;
};

class GLTFLoader	{
public:
	typedef std::function<SharedMesh (void)>				MeshConstructionFunction;
	typedef std::function<SharedTexture (std::string&)>		TextureConstructionFunction;

	static bool Load(const std::string& filename, GLTFScene& intoScene);

protected:		
	GLTFLoader()  = delete;
	~GLTFLoader() = delete;

	struct GLTFSkin {
		std::vector<std::string>	localJointNames;
		std::map<int, int>			sceneToLocalLookup;
		std::map<int, int>			localToSceneLookup;
		std::vector<Matrix4>		worldBindPose;
		std::vector<Matrix4>		worldInverseBindPose;
		Matrix4						globalTransformInverse;
	};

	struct BaseState {
		uint32_t firstNode;
		uint32_t firstMesh;
		uint32_t firstTex;
		uint32_t firstMat;
		uint32_t firstAnim;
		uint32_t firstMatLayer;
	};

	static void LoadImages(tinygltf::Model& m, GLTFScene& scene, BaseState state, const std::string& rootFile );
	static void LoadMaterials(tinygltf::Model& m, GLTFScene& scene, BaseState state);
	static void LoadSceneNodeData(tinygltf::Model& m, GLTFScene& scene, BaseState state);
	static void LoadVertexData(tinygltf::Model& m, GLTFScene& scene, BaseState state );
	static void LoadSkinningData(tinygltf::Model& model, GLTFScene& scene, int32_t nodeID, int32_t skinID, BaseState state);
	static void LoadAnimationData(tinygltf::Model& m, GLTFScene& scene, BaseState state, OGLMesh& mesh, GLTFSkin& skin);

	static void AssignNodeMeshes(tinygltf::Model& m, GLTFScene& scene, BaseState state);
};