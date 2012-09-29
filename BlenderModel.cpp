// Copyright 2012 The Ephenation Authors
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

#include <assimp.hpp>      // C++ importer interface
#include <aiScene.h>       // Output data structure
#include <aiPostProcess.h> // Post processing flags
#include <stdio.h>
#include <GL/glew.h>
#include <map>
#include <math.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
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
	glm::vec4 colour;                  // The colour of this mesh
	unsigned int numFaces;             // Num ber of faces used by this mesh
	std::unique_ptr<MeshBone[]> bones; // All bones used by this mesh
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

BlenderModel gSwordModel1, gTuftOfGrass, gFrog, gMorran;

BlenderModel::BlenderModel() {
	fBufferId = 0;
	fVao = 0;
	fIndexBufferId = 0;
	fUsingBones = false;
}

BlenderModel::~BlenderModel() {
	// In case of glew not having run yet.
	if (glDeleteBuffers != 0) {
		glDeleteBuffers(1, &fBufferId);
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

	int vertexSize = 0, indexSize = 0;
	if (gVerbose) {
		printf("Scene: %s\n", scene->mRootNode->mName.data);
		printf("%d materials, %d meshes, %d animations, %d textures\n", scene->mNumMaterials, scene->mNumMeshes, scene->mNumAnimations, scene->mNumTextures);
	}

	//**********************************************************************
	// Count how much data is needed.
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
		vertexSize += m->mNumVertices;        // 3 floats for each vertex, times two because we count vertices + normals

		// Iterate through all bones used in this mesh. They may have been seen in another mesh already, which isn't handled yet.
		fMeshData[i].bones.reset(new MeshBone[m->mNumBones]);
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

	// Copy all data into one big buffer
	VertexDataf *vertexData = new VertexDataf[vertexSize];
	unsigned short *indexData = new unsigned short[indexSize];
	int dataOffset = 0, indexOffset = 0;
	int previousOffset = 0;

	// Skinning data. Allocate even if not using bones.
	char *numWeights = new char[vertexSize];
	memset(numWeights, 0, vertexSize);
	glm::vec3 *weights = new glm::vec3[vertexSize];
	float *joints = new float[vertexSize*3]; // Up to 4 joints per vertex, but only three are used.
	memset(joints, 0, vertexSize*3*sizeof (float));

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
		fMeshData[i].numFaces = m->mNumFaces;
		for (unsigned int face=0; face < m->mNumFaces; face++) {
			ASSERT(m->mFaces[face].mNumIndices == 3); // Only allow triangles
			indexData[indexOffset++] = m->mFaces[face].mIndices[0] + previousOffset;
			indexData[indexOffset++] = m->mFaces[face].mIndices[1] + previousOffset;
			indexData[indexOffset++] = m->mFaces[face].mIndices[2] + previousOffset;
		}
		for (unsigned int v = 0; v < m->mNumVertices; v++) {
			glm::vec4 v1(m->mVertices[v].x, m->mVertices[v].y, m->mVertices[v].z, 1);
			glm::vec4 v2 = meshmatrix[i] * v1;
#if 0
			if (gVerbose) {
				printf("from %6.3f %6.3f %6.3f %6.3f\n", v1.x, v1.y, v1.z, v1.w);
				printf("to   %6.3f %6.3f %6.3f %6.3f\n", v2.x, v2.y, v2.z, v2.w);
			}
#endif
			vertexData[dataOffset].SetVertex(glm::vec3(v2));
			glm::vec4 n1(m->mNormals[v].x, m->mNormals[v].y, m->mNormals[v].z, 1);
			glm::vec4 n2 = meshmatrix[i] * glm::normalize(n1);
			vertexData[dataOffset].SetNormal(glm::vec3(n2));
			if (m->mTextureCoords[0] != 0) {
				vertexData[dataOffset].SetTexture(m->mTextureCoords[0][v].x, m->mTextureCoords[0][v].y);
			} else {
				vertexData[dataOffset].SetTexture(0,0);
			}
			vertexData[dataOffset].SetIntensity(255);
			vertexData[dataOffset].SetAmbient(100);
			dataOffset++;
		}

		// Iterate through all bones used in this mesh, and copy weights to respective vertex.
		for (unsigned j=0; j < m->mNumBones; j++) {
			fMeshData[i].bones[j].offset *= glm::inverse(meshmatrix[i]);
			aiBone *aib = m->mBones[j];
			if (gVerbose) {
				printf("    Offset for mesh %d %s:\n", i, aib->mName.data);
				PrintMatrix(4, fMeshData[i].bones[j].offset);
			}
			for (unsigned k=0; k < aib->mNumWeights; k++) {
				int v = aib->mWeights[k].mVertexId;
				int w = numWeights[v+previousOffset]++;
				if (w >= 3)
					ErrorDialog("Too many bone weights on vertice %d, bone %s, model %s", v, aib->mName.data, filename);
				weights[v+previousOffset][w] = aib->mWeights[k].mWeight;
				boneindexIT_t it = fBoneIndex.find(aib->mName.data);
				if (it == fBoneIndex.end())
					ErrorDialog("Mesh %d bone %s not found", i, aib->mName.data);
				joints[(v+previousOffset)*3 + w] = it->second;
			}
		}
		previousOffset = dataOffset;
	}
	for (int j=0; j < vertexSize; j++)
		NormalizeWeights(weights[j], numWeights[j]);
	ASSERT(dataOffset == vertexSize);
	ASSERT(indexOffset == indexSize);

	// Allocated the vertex data in OpenGL. One buffer object is used, with the following layout:
	// 1. The usual vertex data, and array of type VertexDataf
	// 2. Skin weights, array of glm::vec3. 3 floats for each vertex.
	// 3. Bones index, 4 bytes for each vertex (only 3 used)
	const int AREA1 = vertexSize*sizeof vertexData[0];
	const int AREA2 = vertexSize*sizeof weights[0];
	const int AREA3 = vertexSize*3*sizeof (float);
	glGenBuffers(1, &fBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, fBufferId);
	int bufferSize = AREA1;
	if (this->fUsingBones) {
		// Also need space for weights and bones indices.
		bufferSize += AREA2 + AREA3;
	}
	glBufferData(GL_ARRAY_BUFFER, bufferSize, 0, GL_STATIC_DRAW);
	// check data size in VBO is same as input array, if not return 0 and delete VBO
	int tst = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &tst);
	if ((unsigned)tst != bufferSize) {
		glDeleteBuffers(1, &fBufferId);
		fBufferId = 0;
		ErrorDialog("BlenderModel::Init: Data size is mismatch with input array\n");
	}
	glBufferSubData(GL_ARRAY_BUFFER, 0, AREA1, vertexData);
	if (this->fUsingBones) {
		glBufferSubData(GL_ARRAY_BUFFER, AREA1, AREA2, weights);
		glBufferSubData(GL_ARRAY_BUFFER, AREA1+AREA2, AREA3, joints);
	}
	delete []numWeights;
	delete []weights;
	delete []joints;

	// Allocated the index data in OpenGL
	glGenBuffers(1, &fIndexBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fIndexBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize*sizeof indexData[0], indexData, GL_STATIC_DRAW);
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

	delete []vertexData;
	delete []indexData;
	if (gVerbose) {
		printf("Mesh bones for '%s':\n", filename);
		for (boneindexIT_t it = fBoneIndex.begin(); it != fBoneIndex.end(); it++) {
			unsigned int ind = it->second;
			printf(" %s: joint %d\n", it->first.c_str(), ind);
		}
	}

	// Decode animation information
	unsigned int numMeshBones = fBoneIndex.size();
	if (numMeshBones > 0 && scene->mNumAnimations == 0)
		ErrorDialog("BlenderModel::Init %s: Bones but no animations", filename);
	if (scene->mNumAnimations > 0) {
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
			unsigned int joint = it->second;
			fAnimations[i].bones[joint].frameMatrix.reset(new glm::mat4[numKeys]);
			channels[j].joint = joint;
		}

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
		// For some reason, the first key frame doesn't always start at time 0
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
				if (gVerbose) {
					printf("     Key %d animation bone %s absolute\n", k, channels[j].node->mNodeName.data);
					PrintMatrix(10, mat);
				}
				if (joint != UNUSEDCHANNEL) {
					fAnimations[i].bones[joint].frameMatrix[k] = mat * fMeshData[i].bones[joint].offset;
				}
			}
		}
	}
}

void BlenderModel::Draw(AnimationShader *shader, const glm::mat4 &modelMatrix, double animationStart, bool dead) {
	glm::mat4 rot = glm::rotate(modelMatrix, fRotateXCorrection, glm::vec3(1, 0, 0));
	if (!fUsingBones)
		ErrorDialog("BlenderModel::Draw called for model without bones");
	if (fUsingBones) {
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
		unsigned int numBonesActual = fBoneIndex.size();
		glm::mat4 m[numBonesActual];
		for (unsigned int i=0; i<numBonesActual; i++) {
			m[i] = fAnimations[a].bones[i].frameMatrix[key] * (1-w) + fAnimations[a].bones[i].frameMatrix[keyNext] * w;
			if (dead) {
				// Move all bones toward 0 translation.
				m[i][3][0] *= deathTransform;
				m[i][3][1] *= deathTransform;
				m[i][3][2] *= deathTransform;
			}

		}
		shader->Bones(m, numBonesActual);
		shader->Model(rot);
#if 0
		static double lastTime = 0.0;
		if (gCurrentFrameTime != lastTime) {
			printf("numKeyFrames %d totalAnimationTime %.2f delta %.2f animationOffset %.2f key %d keyFrameOffset %.2f keyFrameLength %.2f w %.2f\n",
			       numKeyFrames, totalAnimationTime, delta, animationOffset, key, keyFrameOffset, keyFrameLength, w);
#if 0
			for (unsigned i=0; i<numBonesActual; i++) {
				printf("Joint %d key %d offset time %5.3f final matrix:\n", i, key, offset);
				PrintMatrix(1, m[i]);
			}
#endif
			lastTime = gCurrentFrameTime;
		}
#endif
	}
	this->DrawStatic();
}

void BlenderModel::DrawStatic(void) {
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
}
