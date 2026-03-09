/******************************************************************************
Class:Mesh
Implements:
Author:Rich Davison	 <richard-gordon.davison@newcastle.ac.uk>
Description:Wrapper around OpenGL primitives, geometry and related 
OGL functions.

There's a couple of extra functions in here that you didn't get in the tutorial
series, to draw debug normals and tangents. 


-_-_-_-_-_-_-_,------,   
_-_-_-_-_-_-_-|   /\_/\   NYANYANYAN
-_-_-_-_-_-_-~|__( ^ .^) /
_-_-_-_-_-_-_-""  ""   

*//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "OGLRenderer.h"
#include <vector>
#include <string>

//A handy enumerator, to determine which member of the bufferObject array
//holds which data
enum MeshBuffer {
	VERTEX_BUFFER	,
	COLOUR_BUFFER	, 
	TEXTURE_BUFFER	,
	NORMAL_BUFFER	, 
	TANGENT_BUFFER	,

	WEIGHTVALUE_BUFFER,		//new this year, weight values of vertices
	WEIGHTINDEX_BUFFER,	//new this year, indices of weights

	INDEX_BUFFER	,

	MAX_BUFFER
};

struct Vector4i {
	int x;
	int y;
	int z;
	int w;
};

class Mesh	{
public:	
	struct SubMesh {
		int start	= 0;
		int count	= 0;
		int base	= 0;
	};

	Mesh(void);
	~Mesh(void);

	void Draw();
	void DrawSubMesh(int i);

	// Access internal VAO for instanced rendering
	GLuint GetVAO() const { return arrayObject; }
	int GetIndexCount() const { return numIndices; }

	static Mesh* LoadFromMeshFile(const std::string& name);

	// Create a simple quad mesh (useful for full-screen effects, skybox, etc.)
	// This creates a vertical quad in XY plane facing forward (Z+)
	static Mesh* CreateQuad();

	// Create a horizontal water plane mesh (useful for water surfaces)
	// This creates a horizontal quad in XZ plane facing up (Y+)
	static Mesh* CreateWaterQuad();

	// Create a UV sphere mesh for sun/moon rendering
	static Mesh* CreateSphere(int latitudeSegments = 20, int longitudeSegments = 20);

	static Mesh* MeshFromVectors(
		const std::vector < Vector3 >& positions,
		const std::vector < Vector4 >& colours,
		const std::vector < Vector2 >& texCoords,
		const std::vector < Vector3 >& normals,
		const std::vector < Vector4 >& tangents,
		const std::vector < Vector4 >& skinWeights,
		const std::vector < Vector4i >& skinIndices,

		const std::vector < unsigned int >& vIndices,
		const std::vector< SubMesh>& meshSetup
	);

	unsigned int GetTriCount() const {
		int primCount = indices ? numIndices : numVertices;
		return primCount / 3;
	}

	unsigned int GetJointCount() const {
		return (unsigned int)jointNames.size();
	}

	int GetIndexForJoint(const std::string& name) const;
	int GetParentForJoint(const std::string& name) const;
	int GetParentForJoint(int i) const;

	const Matrix4* GetBindPose() const {
		return bindPose.empty() ? nullptr : bindPose.data();
	}

	const Matrix4* GetInverseBindPose() const {
		return inverseBindPose.empty() ? nullptr : inverseBindPose.data();
	}

	int		GetSubMeshCount() const {
		return (int)meshLayers.size(); 
	}

	bool GetSubMesh(int i, const SubMesh* s) const;
	bool GetSubMesh(const std::string& name, const SubMesh* s) const;



	//EMERGENCY

	void SetJointNames(const std::vector < std::string >& newnames);
	void SetJointParents(const std::vector<int>& newParents);
	void SetBindPose(const std::vector<Matrix4>& newMats);
	void SetInverseBindPose(const std::vector<Matrix4>& newMats);

protected:
	void	BufferData();

	GLuint	arrayObject;

	GLuint	bufferObject[MAX_BUFFER];

	GLuint	numVertices;
	GLuint	numIndices;
	
	GLuint	type;

	Vector3*		vertices;
	Vector4*		colours;
	Vector2*		textureCoords;
	Vector3*		normals;
	Vector4*		tangents;

	Vector4*		weights;
	int*			weightIndices;

	unsigned int*	indices;

	std::vector<Matrix4> bindPose;
	std::vector<Matrix4> inverseBindPose;

	std::vector<std::string>	jointNames;
	std::vector<int>			jointParents;
	std::vector< SubMesh>		meshLayers;
	std::vector<std::string>	layerNames;
};

