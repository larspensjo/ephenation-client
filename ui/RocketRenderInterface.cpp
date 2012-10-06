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

#include <Rocket/Core.h>
#include <Rocket/Core/Geometry.h>
#include <GL/glew.h>

#include "../glm/gtc/matrix_transform.hpp"
#include "../primitives.h"
#include "../textures.h"
#include "../imageloader.h"
#include "RocketRenderInterface.h"

RocketRenderInterface::RocketRenderInterface() : fColorShader(0), fSimpleTextureShader(0)
{
}

void RocketRenderInterface::Init() {
	fColorShader = ColorShader::Make();
	fSimpleTextureShader = SimpleTextureShader::Make();
}

void RocketRenderInterface::RenderGeometry(Rocket::Core::Vertex* vertices,  int num_vertices, int* indices, int num_indices, const Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation)
{
	// Immediate rendering of geometry isn't used.
}

Rocket::Core::CompiledGeometryHandle RocketRenderInterface::CompileGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rocket::Core::TextureHandle texturehandle)
{
	Geometry* geometry = new Geometry;

	geometry->texture = (GLuint)texturehandle;

	// Allocate the vertex data object
	glGenBuffers(1, &geometry->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vbo);
	glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof (Rocket::Core::Vertex), vertices, GL_STATIC_DRAW);

	// Allocate the index data in OpenGL
	glGenBuffers(1, &geometry->vbi);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->vbi);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof indices[0], indices, GL_STATIC_DRAW);
	geometry->numIndices = num_indices;

	// Set up the vertex array object
	glGenVertexArrays(1, &geometry->vao);
	glBindVertexArray(geometry->vao);
	Rocket::Core::Vertex *vp = 0;
	if (texturehandle == 0) {
		// Use the ColorShader
		glEnableVertexAttribArray(fColorShader->VERTEX_INDEX);
		glEnableVertexAttribArray(fColorShader->COLOR_INDEX);
		glVertexAttribPointer(fColorShader->VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof (Rocket::Core::Vertex), &vp->position.x);
		glVertexAttribPointer(fColorShader->COLOR_INDEX, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof (Rocket::Core::Vertex), &vp->colour.red);
	} else {
		fSimpleTextureShader->EnableVertexAttribArray();
		fSimpleTextureShader->VertexAttribPointer(GL_FLOAT, 2, sizeof (Rocket::Core::Vertex), &vp->position.x);
		fSimpleTextureShader->TextureAttribPointer(GL_FLOAT, sizeof (Rocket::Core::Vertex), &vp->tex_coord.x);
		// Use the SimpleTextureShader
	}
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->vbi); // Will be remembered in the VAO state
	glBindVertexArray(0);
	checkError("RocketRenderInterface::CompileGeometry");

	return reinterpret_cast<Rocket::Core::CompiledGeometryHandle>(geometry);
}

void RocketRenderInterface::RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry_ptr, const Rocket::Core::Vector2f& translation)
{
	glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(translation.x, translation.y, 1.0f));
	Geometry* geometry = reinterpret_cast<Geometry*>(geometry_ptr);
	if (geometry->texture == 0) {
		fColorShader->EnableProgram();
		fColorShader->ModelView(model);
		fColorShader->Color(glm::vec4(0)); // Will make the color vertex attribute be used instead
		fColorShader->Projection(glm::mat4(1));
		glDrawElements(GL_TRIANGLES, geometry->numIndices, GL_UNSIGNED_INT, 0);
		fColorShader->DisableProgram();
	} else {
		fSimpleTextureShader->EnableProgram();
		fSimpleTextureShader->ModelView(model);
		fSimpleTextureShader->Projection(glm::mat4(1));
		glBindTexture(GL_TEXTURE_2D, geometry->texture);
		glDrawElements(GL_TRIANGLES, geometry->numIndices, GL_UNSIGNED_INT, 0);
		fSimpleTextureShader->DisableProgram();
	}
}

void RocketRenderInterface::ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry_ptr)
{
	Geometry* geometry = reinterpret_cast<Geometry*>(geometry_ptr);
	glDeleteBuffers(1, &geometry->vbo);
	glDeleteVertexArrays(1, &geometry->vao);
	glDeleteBuffers(1, &geometry->vbi);
	delete geometry;
}

void RocketRenderInterface::EnableScissorRegion(bool enable)
{
	if (enable)
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);
}

void RocketRenderInterface::SetScissorRegion(int x, int y, int width, int height)
{
	glScissor(x, y, width, height);
}

bool RocketRenderInterface::LoadTexture(Rocket::Core::TextureHandle& texture_handle, Rocket::Core::Vector2i& texture_dimensions, const Rocket::Core::String& source)
{
	std::string path = "textures/" + std::string(source.CString()) + ".bmp";
	auto bmp = loadBMP(path.c_str());
	GLuint texture = LoadBitmapForGui(bmp);

	if (texture != 0) {
		texture_handle = texture;
		texture_dimensions = Rocket::Core::Vector2i(bmp->width, bmp->height);
		return true;
	}
	return false;
}

bool RocketRenderInterface::GenerateTexture(Rocket::Core::TextureHandle& texture_handle,  const Rocket::Core::byte* source, const Rocket::Core::Vector2i& source_dimensions)
{
	GLuint texture;
	glGenTextures(1, &texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, source_dimensions.x, source_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, source);

	texture_handle = texture;
	return true;
}

void RocketRenderInterface::ReleaseTexture(Rocket::Core::TextureHandle texture_handle)
{
	GLuint texture = texture_handle;
	glDeleteTextures(1, &texture);
}
