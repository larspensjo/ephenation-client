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

#include <GL/glew.h>
#include <Rocket/Core.h>

#include "../shaders/ColorShader.h"
#include "../shaders/SimpleTextureShader.h"

class RocketRenderInterface: public Rocket::Core::RenderInterface
{
public:
	RocketRenderInterface();

	void Init();

	virtual void RenderGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation);

	virtual Rocket::Core::CompiledGeometryHandle CompileGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, Rocket::Core::TextureHandle texture);

	virtual void RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry, const Rocket::Core::Vector2f& translation);

	virtual void ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry);

	virtual void EnableScissorRegion(bool enable);

	virtual void SetScissorRegion(int x, int y, int width, int height);

	virtual bool LoadTexture(Rocket::Core::TextureHandle& texture_handle, Rocket::Core::Vector2i& texture_dimensions, const Rocket::Core::String& source);

	virtual bool GenerateTexture(Rocket::Core::TextureHandle& texture_handle, const Rocket::Core::byte* source, const Rocket::Core::Vector2i& source_dimensions);

	virtual void ReleaseTexture(Rocket::Core::TextureHandle texture_handle);

private:

	struct Geometry
	{
		GLuint texture;
		int numIndices;
		GLuint vao;
		GLuint vbo;
		GLuint vbi;
		glm::vec3 color;
	};

	ColorShader *fColorShader; // Pointer to singleton, do not delete
	SimpleTextureShader *fSimpleTextureShader; // Pointer to singleton, do not delete
};
