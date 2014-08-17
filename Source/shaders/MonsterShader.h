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
// This shader is specialized for drawing monsters.
// Every limb comes with a reference to the joint coordinate, to make it possible to compute
// the angle of the limb in the shader. This angle is controlled by a uniform "cycle" parameter.
// The angle is defined as an unsigned char, where the limb is back to the beginning in the range 0-255.
// The model and view transformation matrix are
// combined into one single ModelView matrix.
// This is a singleton class.
//

#ifndef MONSTERSHADER_H_
#define MONSTERSHADER_H_

#include <glbinding/gl/types.h>

#include "shader.h"

using gl::GLvoid;

class MonsterShader : ShaderBase {
public:
	static MonsterShader *Make(void);
	void EnableProgram(void);
	void DisableProgram(void);
	void ModelView(const glm::mat4 &model, const glm::mat4 &view, float sun=0.0f, float ambient=0.2f);	// Define the ModelView matrix
	void Cycle(unsigned char);		// The cycle, 0-255, of the limbs movement.
	void ColorAddon(const float *);
	// Define memory layout for the vertices. A buffer must be bound to do this.
	void VertexAttribPointer(GLsizei stride, const GLvoid * pointer);
	// Define memory layout for the textures. A buffer must be bound to do this.
	void TextureAttribPointer(GLsizei stride, const GLvoid * pointer);
	void NormalAttribPointer(GLsizei stride, const GLvoid *offset);

	void EnableVertexAttribArray(void);
protected:
	virtual void PreLinkCallback(GLuint prg);
private:
	// Define all uniform and attribute indices.
	virtual void GetLocations(void);
	MonsterShader(); // Only allow access through the maker.
	virtual ~MonsterShader();
	static MonsterShader fgSingleton; // This is the singleton instance
	static const GLchar *fVertexShaderSource[];
	static const GLchar *fFragmentShaderSource[];
	GLint fgFirstTextureIndex, fCycleIndex, fModelViewMatrixIndex, fNormalMatrixIdx, fModelMatrixIdx;
	GLint fgVertexIndex, fgTexCoordIndex, fNormalIndex, fColorAddonIdx, fSunIdx, fAmbientIdx;
};

#endif /* MONSTERSHADER_H_ */
