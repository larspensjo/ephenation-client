// Copyright 2012-2013 The Ephenation Authors
//
// This file is part of Ephenation.
//
// Ephenation is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 3.
//
// Ephenation is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Ephenation.  If not, see <http://www.gnu.org/licenses/>.
//

//
// This is probably the most complicated part of the Ephenation client. One reason
// is that model and animation data in Assimp is complex. Another is that the data
// has to be converted to a local representation optimized for drawing using a shader.
// At first glance, it may look as if Assimp is unecessaery complex. But the data is
// really needed, and it has to be organized they way it is. There is a reason for it.
//
// See http://ephenationopengl.blogspot.de/2012/06/doing-animations-in-opengl.html
//

#ifdef ASSIMP3
    #include <assimp/Importer.hpp>
    #include <assimp/scene.h>       // Output data structure
    #include <assimp/postprocess.h>
#else
    #include <assimp/assimp.hpp>      // C++ importer interface
    #include <assimp/aiScene.h>       // Output data structure
    #include <assimp/aiPostProcess.h> // Post processing flags
#endif

#include <stdio.h>
#include <GL/glew.h>
#include <map>
#include <vector>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "BlenderModel.h"
#include "shaders/ChunkShader.h"
#include "shaders/AnimationShader.h"
#include "assert.h"
#include "ui/Error.h"
#include "primitives.h"
#include "textures.h"

// There is a list of bones associated to a mesh.
struct MeshBone {
	glm::mat4 offset; // Offset to mesh
	unsigned int jointIndex;
};

struct BlenderModel::Mesh {
	glm::vec4 colour;            // The colour of this mesh
	unsigned int numFaces;       // Number of faces used by this mesh
	std::vector<MeshBone> bones; // All bones used by this mesh
};

// One bone in an animation contains the absolute location of all frames
struct AnimationBone {
	std::unique_ptr<glm::mat4[]> frameMatrix;
};

// There can be several animations, like "running", "walking", etc.
struct BlenderModel::Animation {
	std::string name;
	std::unique_ptr<AnimationBone[]> bones;
	std::unique_ptr<double[]> times;        // The time stamps of the key frames.
	int numKeys;
	double keysPerSecond;
	double duration;                  // Number of ticks
};

BlenderModel gSwordModel1, gTuftOfGrass, gFrog, gMorran, gAlien;

BlenderModel::BlenderModel() : fRotateXCorrection(0.0f), fNumMeshes(0) {
	fBufferId = 0;
	fVao = 0;
	fIndexBufferId = 0;
	fUsingBones = false;
}

BlenderModel::~BlenderModel() {
	// In case of glew not having run yet.
	if (glDeleteBuffers != 0) {
		glDeleteBuffers(1, &fBufferId);
		glDeleteBuffers(1, &fIndexBufferId);
		glDeleteVertexArrays(1, &fVao);
	}
}

static void CopyaiMat(const aiMatrix4x4 *from, glm::mat4 &to) {
	// OpenGL (and glm) uses column major order, but Assimp uses row major order.
	to[0][0] = from->a1; to[1][0] = from->a2; to[2][0] = from->a3; to[3][0] = from->a4;
	to[0][1] = from->b1; to[1][1] = from->b2; to[2][1] = from->b3; to[3][1] = from->b4;
	to[0][2] = from->c1; to[1][2] = from->c2; to[2][2] = from->c3; to[3][2] = from->c4;
	to[0][3] = from->d1; to[1][3] = from->d2; to[2][3] = from->d3; to[3][3] = from->d4;
}

void NormalizeWeights(glm::vec3 &w, int n) {
	if (n == 0)
		return;
	float sum = 0;
	for (int i=0; i<n; i++)
		sum += w[i];
	for (int i=0; i<n; i++)
		w[i] /= sum;
}

static void CopyaiQuat(glm::mat4 &dest, const aiQuaternion &quat) {
	glm::quat q(quat.w, quat.x, quat.y, quat.z);
	dest = glm::mat4_cast(q);
}

static void PrintMatrix(int indent, const glm::mat4 &mat) {
	printf("%*s%5.2f %5.2f %5.2f %5.2f\n", indent, "", mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
	printf("%*s%5.2f %5.2f %5.2f %5.2f\n", indent, "", mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
	printf("%*s%5.2f %5.2f %5.2f %5.2f\n", indent, "", mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
	printf("%*s%5.2f %5.2f %5.2f %5.2f\n", indent, "", mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
}

#if 0
static void PrintQuaternion(int indent, const aiQuaternion &quat) {
	printf("%*sQuaternion [%.2f,%.2f,%.2f,%.2f]\n", indent, "", quat.x, quat.y, quat.z, quat.w);
}
#endif

// Traverse the node tree, and find the node with the given name.
static aiNode *FindNode(aiNode *node, const char *name) {
	if (strcmp(name, node->mName.data) == 0)
		return node;

	for (unsigned int i=0; i < node->mNumChildren; i++) {
		aiNode *res = FindNode(node->mChildren[i], name);
		if (res != 0)
			return res;
	}

	return 0; // Not found
}

static glm::mat4 armature(1);

// Print out the complete hierarchical node tree
static void DumpNodeTree(int level, const aiNode *node) {
	printf("%*sNode '%s', %d meshes ", level*4, "", node->mName.data, node->mNumMeshes);
	if (node->mNumMeshes > 0) {
		printf("(");
		for (unsigned m = 0; m < node->mNumMeshes; m++)
			printf("%d ", node->mMeshes[m]);
		printf(")");
	}
	printf("\n");

	// Recursively print children
	for (unsigned int i=0; i < node->mNumChildren; i++) {
		DumpNodeTree(level+1, node->mChildren[i]);
	}
}

// Iterate through the node tree and compute all mesh relative matrices.
void BlenderModel::FindMeshTransformations(int level, glm::mat4 *meshmatrix, const glm::mat4 &base, const aiNode *node) {
	glm::mat4 delta;
	CopyaiMat(&node->mTransformation, delta);
	glm::mat4 result = base * delta;
	if (strcmp(node->mName.data, "Armature") == 0)
		armature = result;
	if (gVerbose) {
		printf("%*sNode '%s' relative\n", level, "", node->mName.data);
		PrintMatrix(level, delta);
		printf("%*sGives\n", level, "");
		PrintMatrix(level, result);
	}

	// Transform all meshes belonging to this node
	for (unsigned int i=0; i<node->mNumMeshes; i++) {
		unsigned int m = node->mMeshes[i];
		meshmatrix[m] = result;
	}

	// last step, update all children.
	for (unsigned int i=0; i < node->mNumChildren; i++) {
		FindMeshTransformations(level+1, meshmatrix, result, node->mChildren[i]);
	}
}

void BlenderModel::Init(const char *filename, float xRotateCorrection, bool normalize) {
	this->fRotateXCorrection = xRotateCorrection;
	// Create an instance of the Importer class
	Assimp::Importer importer;

	unsigned int flags = aiProcess_JoinIdenticalVertices|aiProcess_Triangulate|aiProcess_FixInfacingNormals|aiProcess_ValidateDataStructure|
	                     aiProcess_GenNormals|aiProcess_LimitBoneWeights;
	// |aiProcess_OptimizeMeshes
	if (normalize)
		flags |= aiProcess_PreTransformVertices; // This flag will remove all bones.
	importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true); // TODO: This is only used for aiProcess_PreTransformVertices
	importer.SetPropertyInteger(AI_CONFIG_PP_LBW_MAX_WEIGHTS, 3); // The shader can only handle three weights for each vertice.
	const aiScene* scene = importer.ReadFile( filename, flags);
	if (scene == 0) {
		ErrorDialog("assimp loading %s: %s\n", filename, importer.GetErrorString());
		return;
	}

	// Number of vertices and number of indices
	int vertexSize = 0, indexSize = 0;
	if (gVerbose) {
		printf("\nScene: %s (%s)*******************\n", scene->mRootNode->mName.data, filename);
		printf("%d materials, %d meshes, %d animations, %d textures\n", scene->mNumMaterials, scene->mNumMeshes, scene->mNumAnimations, scene->mNumTextures);
	}

	//**********************************************************************
	// Count how much data is needed. All indices and vetices are saved in
	// the same buffer.
	//**********************************************************************

	fMeshData.reset(new Mesh[scene->mNumMeshes]);
	fNumMeshes = scene->mNumMeshes;
	for (unsigned int i=0; i<scene->mNumMeshes; i++) {
		aiMesh *m = scene->mMeshes[i];
		if (m->mNumBones > 0)
			this->fUsingBones = true; // True if any mesh uses bones
		aiMaterial *mat = scene->mMaterials[m->mMaterialIndex];
		aiColor3D c (0.f,0.f,0.f);
		mat->Get(AI_MATKEY_COLOR_DIFFUSE, c);
		fMeshData[i].colour = glm::vec4(c.r, c.g, c.b, 1.0f);
		indexSize += m->mNumFaces * 3; // Always 3 indices for each face (triangle).
		vertexSize += m->mNumVertices;
		if (gVerbose)
			printf("\tMesh %d ('%s'): %d faces, %d vertices\n", i, m->mName.data, m->mNumFaces, m->mNumVertices);

		// Find all animation bones in all meshes. They may have been seen in another mesh already.
		fMeshData[i].bones.resize(m->mNumBones);
		for (unsigned int j=0; j < m->mNumBones; j++) {
			aiBone *aib = m->mBones[j];
			// Add the bone to the global list of bones if it isn't already there.
			boneindexIT_t it = fBoneIndex.find(aib->mName.data);
			unsigned int jointIndex = fBoneIndex.size();
			if (it == fBoneIndex.end())
				fBoneIndex[aib->mName.data] = jointIndex;
			else
				jointIndex = it->second;

			fMeshData[i].bones[j].jointIndex = jointIndex;
			CopyaiMat(&aib->mOffsetMatrix, fMeshData[i].bones[j].offset);
		}
	}
	if (gVerbose)
		printf("Total vertices needed: %d, index count %d\n", vertexSize, indexSize);

	// Find all mesh transformations and update the bones matrix dependency on parents
	glm::mat4 meshmatrix[scene->mNumMeshes];
	FindMeshTransformations(0, meshmatrix, glm::mat4(1), scene->mRootNode);
	if (gVerbose)
		DumpNodeTree(0, scene->mRootNode);

	// Copy all vertex data into one big buffer and all index data into another buffer
	VertexDataf vertexData[vertexSize];
	unsigned short indexData[indexSize];
	int vertexOffset = 0, indexOffset = 0;
	int previousOffset = 0; // The index offset for the current mesh

	// Skinning data. Allocate even if not using bones.
	char numWeights[vertexSize];
	memset(numWeights, 0, sizeof numWeights);
	glm::vec3 weights[vertexSize];
	float joints[vertexSize*3]; // Up to 4 joints per vertex, but only three are used.
	memset(joints, 0, sizeof joints);

	//**********************************************************************
	// Traverse the meshes again, generating the vertex data
	//**********************************************************************

	for (unsigned int i=0; i<scene->mNumMeshes; i++) {
		aiMesh *m = scene->mMeshes[i];
		if (gVerbose)
			printf("Mesh %d: %d faces, %d vertices, mtl index %d\n", i, m->mNumFaces, m->mNumVertices, m->mMaterialIndex);
#if 1
		if (gVerbose) {
#if 0
			printf("Indices:\n");
			for (unsigned int face=0; face < m->mNumFaces; face++) {
				printf("%d:(", m->mFaces[face].mNumIndices);
				for (unsigned int ind=0; ind < m->mFaces[face].mNumIndices; ind++)
					printf("%d,", m->mFaces[face].mIndices[ind]);
				printf("),");
			}
			printf("\nVertices:\n");
			for (unsigned int vert=0; vert < m->mNumVertices; vert++)
				printf("%6.3f %6.3f %6.3f\n", m->mVertices[vert].x, m->mVertices[vert].y, m->mVertices[vert].z);
#endif
			printf("    Transformation matrix:\n");
			PrintMatrix(4, meshmatrix[i]);
			printf("\n");
		}
#endif

		// Copy faces, but only those that are proper triangles.
		unsigned int numTriangles = 0; // The number of found triangles.
		for (unsigned int face=0; face < m->mNumFaces; face++) {
			if (m->mFaces[face].mNumIndices != 3)
				continue; // Only allow triangles
			// m->mFaces[face].mIndices is a local index into the current mesh. Value 0 will thus
			// adress the first vertex in the current mesh. As all vertices are stored in the same buffer,
			// an offset need to be added to get the correct index of the vertex.
			indexData[indexOffset++] = m->mFaces[face].mIndices[0] + previousOffset;
			indexData[indexOffset++] = m->mFaces[face].mIndices[1] + previousOffset;
			indexData[indexOffset++] = m->mFaces[face].mIndices[2] + previousOffset;
			numTriangles++;
		}
		fMeshData[i].numFaces = numTriangles;

		// Copy all vertices
		for (unsigned int v = 0; v < m->mNumVertices; v++) {
			glm::vec4 v1(m->mVertices[v].x, m->mVertices[v].y, m->mVertices[v].z, 1);
			glm::vec4 v2 = meshmatrix[i] * v1;
			vertexData[vertexOffset].SetVertex(glm::vec3(v2));
			glm::vec4 n1(m->mNormals[v].x, m->mNormals[v].y, m->mNormals[v].z, 1);
			glm::vec4 n2 = meshmatrix[i] * glm::normalize(n1);
			vertexData[vertexOffset].SetNormal(glm::vec3(n2));
			if (m->mTextureCoords[0] != 0) {
				vertexData[vertexOffset].SetTexture(m->mTextureCoords[0][v].x, m->mTextureCoords[0][v].y);
			} else {
				vertexData[vertexOffset].SetTexture(0,0);
			}
			vertexData[vertexOffset].SetIntensity(255);
			vertexData[vertexOffset].SetAmbient(100);
			vertexOffset++;
		}

		// Every animation bone in Assimp data structures have a list of weights and vertex index.
		// We want the weights, 0-3 of them, sorted on vertices instead.
		// Iterate through all bones used in this mesh, and copy weights to respective vertex data.
		for (unsigned j=0; j < m->mNumBones; j++) {
			fMeshData[i].bones[j].offset *= glm::inverse(meshmatrix[i]);
			aiBone *aib = m->mBones[j];
			boneindexIT_t it = fBoneIndex.find(aib->mName.data);
			if (it == fBoneIndex.end())
				ErrorDialog("Mesh %d bone %s not found", i, aib->mName.data);
			if (gVerbose) {
				printf("    Offset for mesh %d %s (%d) joint %d:\n", i, aib->mName.data, j, it->second);
				PrintMatrix(4, fMeshData[i].bones[j].offset);
			}
			for (unsigned k=0; k < aib->mNumWeights; k++) {
				int v = aib->mWeights[k].mVertexId + previousOffset; // Add the local vertex number in the current mesh to the offset to get the global number
				int w = numWeights[v]++;
				// Because AI_CONFIG_PP_LBW_MAX_WEIGHTS is maximized to 3, There can't be more than 3 weights for a vertex
				// unless there is a bug in Assimp.
				if (w >= 3)
					ErrorDialog("Too many bone weights on vertice %d, bone %s, model %s", v, aib->mName.data, filename);
				weights[v][w] = aib->mWeights[k].mWeight;
				joints[v*3 + w] = it->second;
			}
		}
		previousOffset = vertexOffset;
	}

	// Now that the weights are sorted on vertices, it is possible to normalize them. The sum shall be 1.
	for (int j=0; j < vertexSize; j++)
		NormalizeWeights(weights[j], numWeights[j]);
	ASSERT(vertexOffset == vertexSize);
	ASSERT(indexOffset <= indexSize);

	// Allocated the vertex data in OpenGL. The buffer object is used with the following layout:
	// 1. The usual vertex data, and array of type VertexDataf
	// 2. Skin weights, array of glm::vec3. 3 floats for each vertex.
	// 3. Bones index, 4 bytes for each vertex (only 3 used)
	const int AREA1 = vertexSize*sizeof vertexData[0];
	const int AREA2 = vertexSize*sizeof weights[0];
	const int AREA3 = vertexSize*3*sizeof (float);
	glGenBuffers(1, &fBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, fBufferId);
	int bufferSize = AREA1;
	if (fUsingBones) {
		// Also need space for weights and bones indices.
		bufferSize += AREA2 + AREA3;
	}
	glBufferData(GL_ARRAY_BUFFER, bufferSize, 0, GL_STATIC_DRAW); // Allocate the buffer, random content so far
	// check data size in VBO is same as input array, if not return 0 and delete VBO
	int tst = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &tst);
	if (tst != bufferSize) {
		glDeleteBuffers(1, &fBufferId);
		fBufferId = 0;
		ErrorDialog("BlenderModel::Init: Data size is mismatch with input array\n");
	}
	glBufferSubData(GL_ARRAY_BUFFER, 0, AREA1, vertexData);
	if (this->fUsingBones) {
		glBufferSubData(GL_ARRAY_BUFFER, AREA1, AREA2, weights);
		glBufferSubData(GL_ARRAY_BUFFER, AREA1+AREA2, AREA3, joints);
	}

	// Allocate the index data in OpenGL
	glGenBuffers(1, &fIndexBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fIndexBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize*sizeof indexData[0], indexData, GL_STATIC_DRAW); // Allocate and copy
	// check data size in VBO is same as input array, if not return 0 and delete VBO
	bufferSize = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if ((unsigned)bufferSize != indexSize*sizeof indexData[0]) {
		glDeleteBuffers(1, &fBufferId);
		fBufferId = 0;
		ErrorDialog("BlenderModel::Init: Data size is mismatch with input array\n");
	}

	glGenVertexArrays(1, &fVao);
	glBindVertexArray(fVao);
	StageOneShader::EnableVertexAttribArray(this->fUsingBones); // Will be remembered in the VAO state
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fIndexBufferId); // Will be remembered in the VAO state
	StageOneShader::VertexAttribPointer();
	if (this->fUsingBones)
		StageOneShader::VertexAttribPointerSkinWeights(AREA1, AREA1+AREA2);
	checkError("BlenderModel::Init");
	glBindVertexArray(0);

	if (gVerbose) {
		printf("Mesh bones for '%s':\n", filename);
		for (boneindexIT_t it = fBoneIndex.begin(); it != fBoneIndex.end(); ++it) {
			unsigned int ind = it->second;
			printf(" %s: joint %d\n", it->first.c_str(), ind);
		}
	}

	// Decode animation information
	unsigned int numMeshBones = fBoneIndex.size();
	if (numMeshBones > 0 && scene->mNumAnimations == 0)
		ErrorDialog("BlenderModel::Init %s: Bones but no animations", filename);
	if (scene->mNumAnimations > 0) {
		if (numMeshBones == 0)
			ErrorDialog("BlenderModel::Init %s: No mesh bones", filename);
		if (gVerbose)
			printf("Parsing %d animations\n", scene->mNumAnimations);
	}

	//**********************************************************************
	// Decode the animations
	// There may be more animated bones than used by meshes.
	//**********************************************************************
	fAnimations.reset(new Animation[scene->mNumAnimations]);
	for (unsigned int i=0; i < scene->mNumAnimations; i++) {
		aiAnimation *aia = scene->mAnimations[i];
		fAnimations[i].name = aia->mName.data;
		fAnimations[i].keysPerSecond = aia->mTicksPerSecond;
		fAnimations[i].duration = aia->mDuration;

		// Set the size of bones to the actual nutmber of bones used by the meshes, not the total number of bones
		// in the model.
		fAnimations[i].bones.reset(new AnimationBone[numMeshBones]);

		// The number of keys has to be the same for all channels. This is a limitation if it is going to be possible to
		// pre compute all matrices.
		unsigned numChannels = aia->mNumChannels;
		if (numChannels == 0)
			ErrorDialog("BlenderModel::Init no animation channels for %s, %s", aia->mName.data, filename);
		if (numChannels != numMeshBones)
			ErrorDialog("BlenderModel::Init %s animation %d: Can only handle when all bones are used (%d out of %d)", filename, i, numChannels, numMeshBones);
		unsigned int numKeys = aia->mChannels[0]->mNumPositionKeys;
		struct channel {
			glm::mat4 mat; // Relative transformation matrix to parent
			channel *parent;
			aiNodeAnim *node;
			unsigned joint;
#define UNUSEDCHANNEL 0xFFFF
		};
		channel channels[numChannels]; // This is the list of all bones in this animation
		// Check that there are the same number of keys for all channels, and allocate transformation matrices
		bool foundOneBone = false;
		for (unsigned int j=0; j < numChannels; j++) {
			aiNodeAnim *ain = aia->mChannels[j];
			if (ain->mNumPositionKeys != numKeys || ain->mNumRotationKeys != numKeys || ain->mNumScalingKeys != numKeys)
				ErrorDialog("BlenderModel::Init %s Bad animation setup: Pos keys %d, rot keys %d, scaling keys %d", filename, ain->mNumPositionKeys, ain->mNumRotationKeys, ain->mNumScalingKeys);
			channels[j].node = ain;
			channels[j].parent = 0;
			boneindexIT_t it = fBoneIndex.find(ain->mNodeName.data);
			if (it == fBoneIndex.end()) {
				// Skip this channel (bone animation)
				channels[j].joint = UNUSEDCHANNEL; // Mark it as not used
				continue;
				// ErrorDialog("BlenderModel::Init: Unknown bone %s in animation %s for %s", ain->mNodeName.data, fAnimations[i].name.c_str(), filename);
			}
			foundOneBone = true;
			unsigned int joint = it->second;
			fAnimations[i].bones[joint].frameMatrix.reset(new glm::mat4[numKeys]);
			channels[j].joint = joint;
		}
		if (!foundOneBone)
			ErrorDialog("BlenderModel::Init %s animation %d no bones used", filename, i);

		// Find the parent for each animation node
		for (unsigned j=0; j<numChannels; j++) {
			aiNode *n = FindNode(scene->mRootNode, channels[j].node->mNodeName.data);
			if (n == 0 || n->mParent == 0)
				continue; // No parent
			n = n->mParent;
			for (unsigned p=0; p<numChannels; p++) {
				if (p == j || strcmp(n->mName.data, channels[p].node->mNodeName.data) != 0)
					continue;
				// Found it!
				channels[j].parent = &channels[p];
				break;
			}
		}

		fAnimations[i].times.reset(new double[numKeys]);
		fAnimations[i].numKeys = numKeys;
		// The first key frame doesn't always start at time 0
		double firstKeyTime = channels[0].node->mPositionKeys[0].mTime;
		for (unsigned k=0; k < numKeys; k++) {
			fAnimations[i].times[k] = channels[0].node->mPositionKeys[k].mTime - firstKeyTime;
			// printf("Animation key %d at time %.2f\n", k, fAnimations[i].times[k]);
			for (unsigned int j=0; j < numChannels; j++) {
				aiNodeAnim *ain = channels[j].node;
				glm::mat4 R;
				CopyaiQuat(R, ain->mRotationKeys[k].mValue);
				glm::mat4 T = glm::translate(glm::mat4(1), glm::vec3(ain->mPositionKeys[k].mValue.x, ain->mPositionKeys[k].mValue.y, ain->mPositionKeys[k].mValue.z));
				glm::mat4 S = glm::scale(glm::mat4(1), glm::vec3(ain->mScalingKeys[k].mValue.x, ain->mScalingKeys[k].mValue.y, ain->mScalingKeys[k].mValue.z));
				channels[j].mat = T * R * S;
				if (gVerbose) {
					printf("     Key %d animation bone %s relative\n", k, channels[j].node->mNodeName.data);
					PrintMatrix(10, channels[j].mat);
				}
			}
			// Iterate through each animation bone and compute the transformation matrix using parent matrix.
			for (unsigned int j=0; j < numChannels; j++) {
				unsigned joint = channels[j].joint;
				glm::mat4 mat(1);
				for (channel *node = &channels[j]; node; node = node->parent) {
					mat = node->mat * mat;
				}
				mat = armature * mat;
				if (joint != UNUSEDCHANNEL) {
					fAnimations[i].bones[joint].frameMatrix[k] = mat;
					if (gVerbose) {
						printf("     Key %d animation bone %s channel %d absolute\n", k, channels[j].node->mNodeName.data, j);
						PrintMatrix(10, mat);
					}
				}
			}
		}
	}
}

void BlenderModel::DrawAnimation(AnimationShader *shader, const glm::mat4 &modelMatrix, double animationStart, bool dead, const GLuint *textures) {
	glm::mat4 rot = glm::rotate(modelMatrix, fRotateXCorrection, glm::vec3(1, 0, 0));
	if (!fUsingBones)
		ErrorDialog("BlenderModel::Draw called for model without bones");
	double deathTransform = 0.0;
	if (dead) {
		deathTransform = (gCurrentFrameTime - animationStart)/3.0;
		if (deathTransform > 1.0)
			deathTransform = 1.0;
		deathTransform = 1 - deathTransform;
		deathTransform *= deathTransform; // Higher slope in beginning.
		animationStart = gCurrentFrameTime; // disable the other animations
	}
	int a = 0; // Can only handle first animation
	int numKeyFrames = fAnimations[a].numKeys;
	double totalAnimationTime = fAnimations[a].times[numKeyFrames-1];
	double delta = gCurrentFrameTime - animationStart; // Time since this animation started
	double animationOffset = fmod(delta, totalAnimationTime); // Compute time offset into current animation.
	// Find the key on the left side
	int key = 0;
	while (animationOffset > fAnimations[a].times[key+1])
		key++;
	double keyFrameOffset = animationOffset - fAnimations[a].times[key]; // Time offset after key
	int keyNext = key+1;
	if (keyNext >= numKeyFrames)
		keyNext = numKeyFrames-1; // Safe guard, should not happen
	// Find the time interval from this key frame to next
	double keyFrameLength = fAnimations[a].times[keyNext] - fAnimations[a].times[key];
	// 'w' is a value between 0 and 1, Depending on the distance between previous key frame and next.
	double w = 1.0;
	if (keyFrameLength > 0)
		w = keyFrameOffset / keyFrameLength;
	shader->Model(rot);

	unsigned int numBonesActual = fBoneIndex.size();
	glm::mat4 m[numBonesActual];
	glBindVertexArray(fVao);
	int indexOffset = 0;
	for (unsigned int i=0; i<fNumMeshes; i++) {
		Mesh *mesh = &fMeshData[i];
		if (mesh->numFaces == 0)
			continue; // Not normally the case, but can happen for funny models.
		for (const auto &bone : mesh->bones) {
			unsigned joint = bone.jointIndex;
			m[joint] = fAnimations[a].bones[joint].frameMatrix[key] * (1-w) + fAnimations[a].bones[joint].frameMatrix[keyNext] * w;
			m[joint] *= bone.offset;
			if (dead) {
				// Work-around for dying animation.
				m[joint][3][0] *= deathTransform;
				m[joint][3][1] *= deathTransform;
				m[joint][3][2] *= deathTransform;
			}
		}
		shader->Bones(m, numBonesActual);

		if (textures != 0)
			glBindTexture(GL_TEXTURE_2D, textures[i]);
		// The AnimationShader is now prepared with input data, do the actual drawing.
		glDrawElements(GL_TRIANGLES, mesh->numFaces*3, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(indexOffset));
		gDrawnQuads += mesh->numFaces*3;
		gNumDraw++;
		int n = mesh->numFaces * 3 * sizeof (unsigned short); // Always 3 indices for each face (triangle).
		indexOffset += n;
	}

#if 0
	static double lastTime = 0.0;
	if (gCurrentFrameTime != lastTime) {
		printf("numKeyFrames %d totalAnimationTime %.2f delta %.2f animationOffset %.2f key %d keyFrameOffset %.2f keyFrameLength %.2f w %.2f\n",
			   numKeyFrames, totalAnimationTime, delta, animationOffset, key, keyFrameOffset, keyFrameLength, w);
#if 0
		for (unsigned i=0; i<numBonesActual; i++) {
			printf("Joint %d key %d final matrix:\n", i, key);
			PrintMatrix(1, m[i]);
		}
#endif
		lastTime = gCurrentFrameTime;
	}
#endif
}

void BlenderModel::DrawStatic(void) {
	// Draw all meshes in one go.
	glBindVertexArray(fVao);
	int faces = 0;
	for (unsigned int i=0; i<fNumMeshes; i++) {
		// fShader->Color(fColor[i]);
		faces += fMeshData[i].numFaces;
	}
	glDrawElements(GL_TRIANGLES, faces*3, GL_UNSIGNED_SHORT, 0);
	gNumDraw++;
	gDrawnQuads += faces*3;
}

void BlenderModel::Align(glm::mat4 &mat) const {
	mat = glm::rotate(mat, fRotateXCorrection, glm::vec3(1, 0, 0));
}

void BlenderModel::InitModels(void) {
	gFrog.Init("models/frog.dae", 0.0f, false);
	gMorran.Init("models/morran.dae", 0.0f, false);
	gTuftOfGrass.Init("models/tuft.obj", 0.0f, true);
	gSwordModel1.Init("models/BasicSword.obj", -90.0f, true);
	gAlien.Init("models/alien.dae", 0.0f, false);
}
