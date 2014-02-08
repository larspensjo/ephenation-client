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

#pragma once
#include "shader.h"

class StageOneShader: public ShaderBase {
public:
	enum InputLocations {
		Normal, Vertex, SkinWeights, Joints, Material
	};

	virtual void Model(const glm::mat4 &) = 0;// Define the Model matrix

	virtual void DrawingWater(bool) {} // Override this if wanted

	// Used for atlas bitmap
	virtual void TextureOffsetMulti(float offsX, float offsY, float mult) {} // Override this if wanted

	// Define memory layout for data. A buffer must be bound to do this.
	static void VertexAttribPointer(int offset = 0);

	// If there are skin weights, then these are mapped separately.
	static void VertexAttribPointerSkinWeights(int skinOffset, int jointOffset);

	// If bones are used, VertexAttribPointerSkinWeights() also has to be called.
	static void EnableVertexAttribArray(bool useBones = false);

	virtual void EnableProgram(void);
	void DisableProgram(void);
};
