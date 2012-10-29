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

//
// This class implements a loader for animated models defined in Blender, as well as a Draw() function.
// The loading is based on Assimp, which actually allows for many different
// file formats. As of now, the Collada format has been used (.dae).
//

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <glm/glm.hpp>

class AnimationShader;
class aiNode;

class BlenderModel {
public:
	struct Animation;
	struct Mesh;

	BlenderModel();
	virtual ~BlenderModel();

	// Initialize a model from a file.
	// xRotateCorrection: Compensate for model being rotated wrong around x axis.
	// normalize: Normalize the size to 1.0. Doesn't work for animations.
	void Init(const char *filename, float xRotateCorrection, bool normalize);

	// 'textures' has to be an array of textures, one for each mesh.
	void DrawAnimation(AnimationShader *shader, const glm::mat4 &modelMatrix, double animationStart, bool dead, const GLuint *textures);

	// Use the model, with any shader.
	void DrawStatic(void);
	static void InitModels(void);

	// Modify a transformation matrix that will align the model, as needed.
	void Align(glm::mat4 &mat) const;
private:
	GLuint fBufferId;
	GLuint fIndexBufferId;
	GLuint fVao; // A list of Vertex Attribute Object
	float fRotateXCorrection; // How much to rotate around the X axis to normalize direction

	// Traverse the node tree. Compute the absolute matrices (the matrices are relative to the parent). Also update meshes.
	void FindMeshTransformations(int level, glm::mat4 *meshmatrix, const glm::mat4 &base, const aiNode *node);

	bool fUsingBones; // True if bones are used to define mesh positions.
	std::unique_ptr<Animation[]> fAnimations; // for each animation.
	unsigned fNumMeshes;
	std::unique_ptr<Mesh[]> fMeshData; // Data for each mesh.

	// All joints are ordered. Map from bone name to joint number. Only bones actually used in a mesh are saved here.
	typedef std::map<std::string, unsigned int>::iterator boneindexIT_t;
	std::map<std::string, unsigned int> fBoneIndex;
};

extern BlenderModel gSwordModel1, gTuftOfGrass, gFrog, gMorran, gAlien;
