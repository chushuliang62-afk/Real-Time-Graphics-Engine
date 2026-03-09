#include "Mesh.h"
#include "Matrix2.h"

using std::string;

Mesh::Mesh(void)	{
	glGenVertexArrays(1, &arrayObject);
	
	for(int i = 0; i < MAX_BUFFER; ++i) {
		bufferObject[i] = 0;
	}

	numVertices  = 0;
	type		 = GL_TRIANGLES;

	numIndices		= 0;
	vertices		= nullptr;
	textureCoords	= nullptr;
	normals			= nullptr;
	tangents		= nullptr;
	indices			= nullptr;
	colours			= nullptr;
	weights			= nullptr;
	weightIndices	= nullptr;

	// bindPose and inverseBindPose are now std::vector, no need to initialize
}

Mesh* Mesh::MeshFromVectors(
	const std::vector < Vector3 >& positions,
	const std::vector < Vector4 >& colours,
	const std::vector < Vector2 >& texCoords,
	const std::vector < Vector3 >& normals,
	const std::vector < Vector4 >& tangents,
	const std::vector < Vector4 >& skinWeights,
	const std::vector < Vector4i >& skinIndices,

	const std::vector < unsigned int >& vIndices,
	const std::vector< SubMesh>& meshSetup
) {
	Mesh* m = new Mesh();

	m->numVertices	= positions.size();
	m->numIndices	= vIndices.size();

	if (!positions.empty()) {
		m->vertices = new Vector3[positions.size()];
		memcpy(m->vertices, positions.data(), sizeof(Vector3) * positions.size());
	}

	if (!colours.empty()) {
		m->colours = new Vector4[colours.size()];
		memcpy(m->colours, colours.data(), sizeof(Vector4) * colours.size());
	}
	if (!texCoords.empty()) {
		m->textureCoords = new Vector2[texCoords.size()];
		memcpy(m->textureCoords, texCoords.data(), sizeof(Vector2) * texCoords.size());
	}
	if (!normals.empty()) {
		m->normals = new Vector3[normals.size()];
		memcpy(m->normals, normals.data(), sizeof(Vector3) * normals.size());
	}
	if (!tangents.empty()) {
		m->tangents = new Vector4[tangents.size()];
		memcpy(m->tangents, tangents.data(), sizeof(Vector4) * tangents.size());
	}
	if (!skinWeights.empty()) {
		m->weights = new Vector4[skinWeights.size()];
		memcpy(m->weights, skinWeights.data(), sizeof(Vector4) * skinWeights.size());
	}
	if (!skinIndices.empty()) {
		m->weightIndices = new int[skinIndices.size() * 4];

		memcpy(m->weightIndices, skinIndices.data(), sizeof(Vector4i) * skinIndices.size());

	}
	if (!vIndices.empty()) {
		m->indices = new unsigned int[vIndices.size()];
		memcpy(m->indices, vIndices.data(), sizeof(unsigned int) * vIndices.size());
	}

	m->meshLayers = meshSetup;

	m->BufferData();

	return m;
}




Mesh::~Mesh(void)	{
	glDeleteVertexArrays(1, &arrayObject);			//Delete our VAO
	glDeleteBuffers(MAX_BUFFER, bufferObject);		//Delete our VBOs

	delete[]	vertices;
	delete[]	indices;
	delete[]	textureCoords;
	delete[]	tangents;
	delete[]	normals;
	delete[]	colours;
	delete[]	weights;
	delete[]	weightIndices;
}

Mesh* Mesh::CreateQuad() {
	std::vector<Vector3> positions = {
		Vector3(-1.0f, -1.0f, 0.0f),
		Vector3(-1.0f,  1.0f, 0.0f),
		Vector3( 1.0f,  1.0f, 0.0f),
		Vector3( 1.0f, -1.0f, 0.0f)
	};

	std::vector<Vector2> texCoords = {
		Vector2(0.0f, 1.0f),
		Vector2(0.0f, 0.0f),
		Vector2(1.0f, 0.0f),
		Vector2(1.0f, 1.0f)
	};

	std::vector<Vector4> colors = {
		Vector4(1.0f, 1.0f, 1.0f, 1.0f),
		Vector4(1.0f, 1.0f, 1.0f, 1.0f),
		Vector4(1.0f, 1.0f, 1.0f, 1.0f),
		Vector4(1.0f, 1.0f, 1.0f, 1.0f)
	};

	std::vector<Vector3> normals = {
		Vector3(0.0f, 0.0f, 1.0f),
		Vector3(0.0f, 0.0f, 1.0f),
		Vector3(0.0f, 0.0f, 1.0f),
		Vector3(0.0f, 0.0f, 1.0f)
	};

	std::vector<Vector4> tangents = {
		Vector4(1.0f, 0.0f, 0.0f, 1.0f),
		Vector4(1.0f, 0.0f, 0.0f, 1.0f),
		Vector4(1.0f, 0.0f, 0.0f, 1.0f),
		Vector4(1.0f, 0.0f, 0.0f, 1.0f)
	};

	std::vector<Vector4> skinWeights;  // Empty for non-skinned mesh
	std::vector<Vector4i> skinIndices; // Empty for non-skinned mesh

	std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};

	// Create one submesh for the entire quad
	std::vector<SubMesh> submeshes;
	SubMesh submesh;
	submesh.start = 0;
	submesh.count = 6;  // 6 indices (2 triangles)
	submesh.base = 0;
	submeshes.push_back(submesh);

	return MeshFromVectors(positions, colors, texCoords, normals, tangents,
						   skinWeights, skinIndices, indices, submeshes);
}

Mesh* Mesh::CreateWaterQuad() {
	// Create a horizontal quad in XZ plane (Y is up)
	std::vector<Vector3> positions = {
		Vector3(-1.0f, 0.0f, -1.0f),  // Bottom-left
		Vector3(-1.0f, 0.0f,  1.0f),  // Top-left
		Vector3( 1.0f, 0.0f,  1.0f),  // Top-right
		Vector3( 1.0f, 0.0f, -1.0f)   // Bottom-right
	};

	std::vector<Vector2> texCoords = {
		Vector2(0.0f, 0.0f),
		Vector2(0.0f, 1.0f),
		Vector2(1.0f, 1.0f),
		Vector2(1.0f, 0.0f)
	};

	std::vector<Vector4> colors = {
		Vector4(1.0f, 1.0f, 1.0f, 1.0f),
		Vector4(1.0f, 1.0f, 1.0f, 1.0f),
		Vector4(1.0f, 1.0f, 1.0f, 1.0f),
		Vector4(1.0f, 1.0f, 1.0f, 1.0f)
	};

	// Water surface normal must point UP (0, 1, 0)
	std::vector<Vector3> normals = {
		Vector3(0.0f, 1.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f),
		Vector3(0.0f, 1.0f, 0.0f)
	};

	std::vector<Vector4> tangents = {
		Vector4(1.0f, 0.0f, 0.0f, 1.0f),
		Vector4(1.0f, 0.0f, 0.0f, 1.0f),
		Vector4(1.0f, 0.0f, 0.0f, 1.0f),
		Vector4(1.0f, 0.0f, 0.0f, 1.0f)
	};

	std::vector<Vector4> skinWeights;
	std::vector<Vector4i> skinIndices;
	std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};

	std::vector<SubMesh> submeshes;
	SubMesh submesh;
	submesh.start = 0;
	submesh.count = 6;
	submesh.base = 0;
	submeshes.push_back(submesh);

	return MeshFromVectors(positions, colors, texCoords, normals, tangents,
						   skinWeights, skinIndices, indices, submeshes);
}

void Mesh::Draw()	{
	glBindVertexArray(arrayObject);
	if(bufferObject[INDEX_BUFFER]) {
		glDrawElements(type, numIndices, GL_UNSIGNED_INT, 0);
	}
	else{
		glDrawArrays(type, 0, numVertices);
	}
	glBindVertexArray(0);	
}

void Mesh::DrawSubMesh(int i) {
	if (i < 0 || i >= (int)meshLayers.size()) {
		return;
	}
	SubMesh m = meshLayers[i];

	glBindVertexArray(arrayObject);
	if (bufferObject[INDEX_BUFFER]) {
		const GLvoid* offset = (const GLvoid * )(m.start * sizeof(unsigned int)); 
		glDrawElements(type, m.count, GL_UNSIGNED_INT, offset);
	}
	else {
		glDrawArrays(type, m.start, m.count);	//Draw the triangle!
	}
	glBindVertexArray(0);
}

void UploadAttribute(GLuint* id, int numElements, int dataSize, int attribSize, int attribID, void* pointer, const string&debugName) {
	glGenBuffers(1, id);
	glBindBuffer(GL_ARRAY_BUFFER, *id);
	glBufferData(GL_ARRAY_BUFFER, numElements * dataSize, pointer, GL_STATIC_DRAW);

	glVertexAttribPointer(attribID, attribSize, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(attribID);

	glObjectLabel(GL_BUFFER, *id, -1, debugName.c_str());
}

void	Mesh::BufferData()	{
	glBindVertexArray(arrayObject);

	////Buffer vertex data
	UploadAttribute(&bufferObject[VERTEX_BUFFER], numVertices, sizeof(Vector3), 3, VERTEX_BUFFER, vertices, "Positions");

	if(textureCoords) {	//Buffer texture data
		UploadAttribute(&bufferObject[TEXTURE_BUFFER], numVertices, sizeof(Vector2), 2, TEXTURE_BUFFER, textureCoords, "TexCoords");
	}

	if (colours) {
		UploadAttribute(&bufferObject[COLOUR_BUFFER], numVertices, sizeof(Vector4), 4, COLOUR_BUFFER, colours, "Colours");
	}

	if (normals) {	//Buffer normal data
		UploadAttribute(&bufferObject[NORMAL_BUFFER], numVertices, sizeof(Vector3), 3, NORMAL_BUFFER, normals, "Normals");
	}

	if (tangents) {	//Buffer tangent data
		UploadAttribute(&bufferObject[TANGENT_BUFFER], numVertices, sizeof(Vector4), 4, TANGENT_BUFFER, tangents, "Tangents");
	}

	if (weights) {		//Buffer weights data
		UploadAttribute(&bufferObject[WEIGHTVALUE_BUFFER], numVertices, sizeof(Vector4), 4, WEIGHTVALUE_BUFFER, weights, "Weights");
	}

	//Buffer weight indices data...uses a different function since its integers...
	if (weightIndices) {
		glGenBuffers(1, &bufferObject[WEIGHTINDEX_BUFFER]);
		glBindBuffer(GL_ARRAY_BUFFER, bufferObject[WEIGHTINDEX_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(int) * 4, weightIndices, GL_STATIC_DRAW);
		glVertexAttribIPointer(WEIGHTINDEX_BUFFER, 4, GL_INT, 0, 0); //note the new function...
		glEnableVertexAttribArray(WEIGHTINDEX_BUFFER);

		glObjectLabel(GL_BUFFER, bufferObject[WEIGHTINDEX_BUFFER], -1, "Weight Indices");
	}

	//buffer index data
	if(indices) {
		glGenBuffers(1, &bufferObject[INDEX_BUFFER]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferObject[INDEX_BUFFER]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices*sizeof(GLuint), indices, GL_STATIC_DRAW);

		glObjectLabel(GL_BUFFER, bufferObject[INDEX_BUFFER], -1, "Indices");
	}
	glBindVertexArray(0);	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


/*
* 
* Extra file loading stuff!
* 
* */

enum class GeometryChunkTypes {
	VPositions		= 1,
	VNormals		= 2,
	VTangents		= 4,
	VColors			= 8,
	VTex0			= 16,
	VTex1			= 32,
	VWeightValues	= 64,
	VWeightIndices	= 128,
	Indices			= 256,
	JointNames		= 512,
	JointParents	= 1024,
	BindPose		= 2048,
	BindPoseInv		= 4096,
	Material		= 65536,
	SubMeshes		= 1 << 14,
	SubMeshNames	= 1 << 15
};

void ReadTextFloats(std::ifstream& file, vector<Vector2>& element, int numVertices) {
	for (int i = 0; i < numVertices; ++i) {
		Vector2 temp;
		file >> temp.x;
		file >> temp.y;
		element.emplace_back(temp);
	}
}

void ReadTextFloats(std::ifstream& file, vector<Vector3>& element, int numVertices) {
	for (int i = 0; i < numVertices; ++i) {
		Vector3 temp;
		file >> temp.x;
		file >> temp.y;
		file >> temp.z;
		element.emplace_back(temp);
	}
}

void ReadTextFloats(std::ifstream& file, vector<Vector4>& element, int numVertices) {
	for (int i = 0; i < numVertices; ++i) {
		Vector4 temp;
		file >> temp.x;
		file >> temp.y;
		file >> temp.z;
		file >> temp.w;
		element.emplace_back(temp);
	}
}

void ReadTextVertexIndices(std::ifstream& file, vector<int>& element, int numVertices) {
	for (int i = 0; i < numVertices; ++i) {
		int indices[4];
		file >> indices[0];
		file >> indices[1];
		file >> indices[2];
		file >> indices[3];
		element.emplace_back(indices[0]);
		element.emplace_back(indices[1]);
		element.emplace_back(indices[2]);
		element.emplace_back(indices[3]);
	}
}

void ReadIndices(std::ifstream& file, vector<unsigned int>& elements, int numIndices) {
	for (int i = 0; i < numIndices; ++i) {
		unsigned int temp;
		file >> temp;
		elements.emplace_back(temp);
	}
}

void ReadJointParents(std::ifstream& file, vector<int>& dest) {
	int jointCount = 0;
	file >> jointCount;

	for (int i = 0; i < jointCount; ++i) {
		int id = -1;
		file >> id;
		dest.emplace_back(id);
	}
}

void ReadJointNames(std::ifstream& file, vector<string>& dest) {
	int jointCount = 0;
	file >> jointCount;
	for (int i = 0; i < jointCount; ++i) {
		std::string jointName;
		file >> jointName;
		dest.emplace_back(jointName);
	}
}

void ReadRigPose(std::ifstream& file, std::vector<Matrix4>& into) {
	int matCount = 0;
	file >> matCount;

	into.resize(matCount);

	for (int i = 0; i < matCount; ++i) {
		Matrix4 mat;
		for (int j = 0; j < 16; ++j) {
			file >> mat.values[j];
		}
		into[i] = mat;
	}
}

void ReadSubMeshes(std::ifstream& file, int count, vector<Mesh::SubMesh> & subMeshes) {
	for (int i = 0; i < count; ++i) {
		Mesh::SubMesh m;
		file >> m.start;
		file >> m.count;
		subMeshes.emplace_back(m);
	}
}

void ReadSubMeshNames(std::ifstream& file, int count, vector<string>& names) {
	std::string scrap;
	std::getline(file, scrap);

	for (int i = 0; i < count; ++i) {
		std::string meshName;
		std::getline(file, meshName);
		names.emplace_back(meshName);
	}
}

Mesh* Mesh::LoadFromMeshFile(const string& name) {
	Mesh* mesh = new Mesh();

	std::ifstream file(MESHDIR + name);

	std::string filetype;
	int fileVersion;

	file >> filetype;

	if (filetype != "MeshGeometry") {
		std::cout << "File is not a MeshGeometry file!" << std::endl;
		return nullptr;
	}

	file >> fileVersion;

	if (fileVersion != 1) {
		std::cout << "MeshGeometry file has incompatible version!" << std::endl;
		return nullptr;
	}

	int numMeshes	= 0; //read
	int numVertices = 0; //read
	int numIndices	= 0; //read
	int numChunks	= 0; //read

	file >> numMeshes;
	file >> numVertices;
	file >> numIndices;
	file >> numChunks;

	vector<Vector3> readPositions;
	vector<Vector4> readColours;
	vector<Vector3> readNormals;
	vector<Vector4> readTangents;
	vector<Vector2> readUVs;
	vector<Vector4> readWeights;
	vector<int> readWeightIndices;

	vector<unsigned int>		readIndices;

	for (int i = 0; i < numChunks; ++i) {
		int chunkType = (int)GeometryChunkTypes::VPositions;

		file >> chunkType;

		switch ((GeometryChunkTypes)chunkType) {
		case GeometryChunkTypes::VPositions:ReadTextFloats(file, readPositions, numVertices);  break;
		case GeometryChunkTypes::VColors:	ReadTextFloats(file, readColours, numVertices);  break;
		case GeometryChunkTypes::VNormals:	ReadTextFloats(file, readNormals, numVertices);  break;
		case GeometryChunkTypes::VTangents:	ReadTextFloats(file, readTangents, numVertices);  break;
		case GeometryChunkTypes::VTex0:		ReadTextFloats(file, readUVs, numVertices);  break;
		case GeometryChunkTypes::Indices:	ReadIndices(file, readIndices, numIndices); break;

		case GeometryChunkTypes::VWeightValues:		ReadTextFloats(file, readWeights, numVertices);  break;
		case GeometryChunkTypes::VWeightIndices:	ReadTextVertexIndices(file, readWeightIndices, numVertices);  break;
		case GeometryChunkTypes::JointNames:		ReadJointNames(file, mesh->jointNames);  break;
		case GeometryChunkTypes::JointParents:		ReadJointParents(file, mesh->jointParents);  break;
		case GeometryChunkTypes::BindPose:			ReadRigPose(file, mesh->bindPose);  break;
		case GeometryChunkTypes::BindPoseInv:		ReadRigPose(file, mesh->inverseBindPose);  break;
		case GeometryChunkTypes::SubMeshes: 		ReadSubMeshes(file, numMeshes, mesh->meshLayers); break;
		case GeometryChunkTypes::SubMeshNames: 		ReadSubMeshNames(file, numMeshes, mesh->layerNames); break;
		}
	}
	//Now that the data has been read, we can shove it into the actual Mesh object

	mesh->numVertices	= numVertices;
	mesh->numIndices	= numIndices;

	if (!readPositions.empty()) {
		mesh->vertices = new Vector3[numVertices];
		memcpy(mesh->vertices, readPositions.data(), numVertices * sizeof(Vector3));
	}

	if (!readColours.empty()) {
		mesh->colours = new Vector4[numVertices];
		memcpy(mesh->colours, readColours.data(), numVertices * sizeof(Vector4));
	}

	if (!readNormals.empty()) {
		mesh->normals = new Vector3[numVertices];
		memcpy(mesh->normals, readNormals.data(), numVertices * sizeof(Vector3));
	}

	if (!readTangents.empty()) {
		mesh->tangents = new Vector4[numVertices];
		memcpy(mesh->tangents, readTangents.data(), numVertices * sizeof(Vector4));
	}

	if (!readUVs.empty()) {
		mesh->textureCoords = new Vector2[numVertices];
		memcpy(mesh->textureCoords, readUVs.data(), numVertices * sizeof(Vector2));
	}
	if (!readIndices.empty()) {
		mesh->indices = new unsigned int[numIndices];
		memcpy(mesh->indices, readIndices.data(), numIndices * sizeof(unsigned int));
	}

	if (!readWeights.empty()) {
		mesh->weights = new Vector4[numVertices];
		memcpy(mesh->weights, readWeights.data(), numVertices * sizeof(Vector4));
	}

	if (!readWeightIndices.empty()) {
		mesh->weightIndices = new int[numVertices * 4];
		memcpy(mesh->weightIndices, readWeightIndices.data(), numVertices * sizeof(int) * 4);
	}

	mesh->BufferData();

	return mesh;
}

int Mesh::GetIndexForJoint(const std::string& name) const {
	for (unsigned int i = 0; i < jointNames.size(); ++i) {
		if (jointNames[i] == name) {
			return i;
		}
	}
	return -1;
}

int Mesh::GetParentForJoint(const std::string& name) const {
	int i = GetIndexForJoint(name);
	if (i == -1) {
		return -1;
	}
	return jointParents[i];
}

int Mesh::GetParentForJoint(int i) const {
	if (i == -1 || i >= (int)jointParents.size()) {
		return -1;
	}
	return jointParents[i];
}

bool Mesh::GetSubMesh(int i, const SubMesh* s) const {
	if (i < 0 || i >= (int)meshLayers.size()) {
		return false;
	}
	s = &meshLayers[i];
	return true;
}

bool Mesh::GetSubMesh(const string& name, const SubMesh* s) const {
	for (unsigned int i = 0; i < layerNames.size(); ++i) {
		if (layerNames[i] == name) {
			return GetSubMesh(i, s);
		}
	}
	return false;
}

//EMERGENCY

void Mesh::SetJointNames(const std::vector < std::string >& newNames) {
	jointNames = newNames;
}

void Mesh::SetJointParents(const std::vector<int>& newParents) {
	jointParents = newParents;
}

void Mesh::SetBindPose(const std::vector<Matrix4>& newMats) {
	bindPose = newMats;
}

void Mesh::SetInverseBindPose(const std::vector<Matrix4>& newMats) {
	inverseBindPose = newMats;
}

Mesh* Mesh::CreateSphere(int latitudeSegments, int longitudeSegments) {
	// Generate UV sphere using spherical coordinates
	std::vector<Vector3> positions;
	std::vector<Vector2> texCoords;
	std::vector<Vector3> normals;
	std::vector<unsigned int> indices;

	const float PI = 3.14159265359f;

	// Generate vertices
	for (int lat = 0; lat <= latitudeSegments; ++lat) {
		float theta = lat * PI / latitudeSegments;  // 0 to PI
		float sinTheta = sin(theta);
		float cosTheta = cos(theta);

		for (int lon = 0; lon <= longitudeSegments; ++lon) {
			float phi = lon * 2.0f * PI / longitudeSegments;  // 0 to 2*PI
			float sinPhi = sin(phi);
			float cosPhi = cos(phi);

			// Spherical to Cartesian coordinates
			float x = cosPhi * sinTheta;
			float y = cosTheta;
			float z = sinPhi * sinTheta;

			positions.push_back(Vector3(x, y, z));
			normals.push_back(Vector3(x, y, z));  // Normal = normalized position for sphere
			texCoords.push_back(Vector2((float)lon / longitudeSegments, (float)lat / latitudeSegments));
		}
	}

	// Generate indices for triangles
	for (int lat = 0; lat < latitudeSegments; ++lat) {
		for (int lon = 0; lon < longitudeSegments; ++lon) {
			int current = lat * (longitudeSegments + 1) + lon;
			int next = current + longitudeSegments + 1;

			// First triangle
			indices.push_back(current);
			indices.push_back(next);
			indices.push_back(current + 1);

			// Second triangle
			indices.push_back(current + 1);
			indices.push_back(next);
			indices.push_back(next + 1);
		}
	}

	// Create mesh
	Mesh* sphere = new Mesh();
	sphere->numVertices = positions.size();
	sphere->numIndices = indices.size();
	sphere->type = GL_TRIANGLES;

	// Allocate and fill vertex data
	sphere->vertices = new Vector3[sphere->numVertices];
	sphere->textureCoords = new Vector2[sphere->numVertices];
	sphere->normals = new Vector3[sphere->numVertices];
	sphere->colours = new Vector4[sphere->numVertices];
	sphere->tangents = new Vector4[sphere->numVertices];
	sphere->indices = new GLuint[sphere->numIndices];

	for (size_t i = 0; i < positions.size(); ++i) {
		sphere->vertices[i] = positions[i];
		sphere->textureCoords[i] = texCoords[i];
		sphere->normals[i] = normals[i];
		sphere->colours[i] = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		sphere->tangents[i] = Vector4(1.0f, 0.0f, 0.0f, 1.0f);  // Simple tangent
	}

	for (size_t i = 0; i < indices.size(); ++i) {
		sphere->indices[i] = indices[i];
	}

	sphere->BufferData();
	return sphere;
}