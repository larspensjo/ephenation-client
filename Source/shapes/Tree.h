// Copyright 2012-2014 The Ephenation Authors
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

#pragma once

#include <vector>

#include "../primitives.h"
#include "../OpenglBuffer.h"

class Tree {
public:
	virtual ~Tree();
	// Create a tree with size of main trunk of 1.0.
	// numIter: number if times to split the trunk into branches
	// branching: How many branches to use for each iteration
	void Init(int numIter, int branching, float height);
	static void InitStatic();
	void Draw(void) const;

	static Tree sfBigTree, sfMediumTree, sfSmallTree;
private:
	void iter(const glm::mat4 &transf, int numIter, int branching, float height);
	// Add geometry that describes a cylinder, with smooth normals
	void AddCylinder(int numSegments, const glm::mat4 &transf);
	void AddOneLeaf(const glm::mat4 &transf, float height);
	static bool sfInitialized; // true if the static members are initialized.

	std::vector<TriangleSurfacef> fLeafTriangles;
	std::vector<TriangleSurfacef> fBranchTriangles;
	OpenglBuffer fLeafBuffer, fBranchBuffer;
	GLuint fVaoLeaf = 0; // The vertex array object for the leafs
	GLuint fVaoBranch = 0; // The vertex array object for the branches, including the trunk
};
