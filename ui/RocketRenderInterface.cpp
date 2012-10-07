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

	// Set up the vertex array object
	glGenVertexArrays(1, &geometry->vao);
	glBindVertexArray(geometry->vao);

	// Allocate the vertex data object
	glGenBuffers(1, &geometry->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vbo);
	glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof (Rocket::Core::Vertex), vertices, GL_STATIC_DRAW);

	// Allocate the index data in OpenGL
	glGenBuffers(1, &geometry->vbi);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry->vbi);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * sizeof indices[0], indices, GL_STATIC_DRAW);
	geometry->numIndices = num_indices;

	Rocket::Core::Vertex *vp = 0;
	if (texturehandle == 0) {
		// Use the ColorShader
		glEnableVertexAttribArray(fColorShader->VERTEX_INDEX);
		glEnableVertexAttribArray(fColorShader->COLOR_INDEX);
		glVertexAttribPointer(fColorShader->VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof (Rocket::Core::Vertex), &vp->position.x);
		glVertexAttribPointer(fColorShader->COLOR_INDEX, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof (Rocket::Core::Vertex), &vp->colour.red);
	} else {
		fSimpleTextureShader->EnableVertexAttribArray();
		fSimpleTextureShader->VertexAttribPointer(GL_FLOAT, 2, sizeof (Rocket::Core::Vertex), &vp->position.x);
		fSimpleTextureShader->TextureAttribPointer(GL_FLOAT, sizeof (Rocket::Core::Vertex), &vp->tex_coord.x);
		// Color override. They are all the same, so first one is saved.
		geometry->color.r = vertices->colour.red/255.0f;
		geometry->color.g = vertices->colour.green/255.0f;
		geometry->color.b = vertices->colour.blue/255.0f;
		// Use the SimpleTextureShader
	}
	glBindVertexArray(0);
	checkError("RocketRenderInterface::CompileGeometry");

	return reinterpret_cast<Rocket::Core::CompiledGeometryHandle>(geometry);
}

void RocketRenderInterface::RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry_ptr, const Rocket::Core::Vector2f& translation)
{
	// DrawPlayerStats(); return;
	glm::mat4 proj = glm::ortho(0.0f, gViewport[2], gViewport[3], 0.0f, -1.0f, 1.0f);
	glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(translation.x, translation.y, 0.0f));
	Geometry* geometry = reinterpret_cast<Geometry*>(geometry_ptr);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBindVertexArray(geometry->vao);
	if (geometry->texture == 0) {
		fColorShader->EnableProgram();
		fColorShader->ModelView(model);
		fColorShader->Color(glm::vec4(0,0,0,0)); // Will make the color vertex attribute be used instead
		fColorShader->Projection(proj);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Not using premultiplied alpha
		glDrawElements(GL_TRIANGLES, geometry->numIndices, GL_UNSIGNED_INT, 0);
		glDisable(GL_BLEND);
		fColorShader->DisableProgram();
	} else {
		glm::vec3 offset = geometry->color - glm::vec3(1,1,1);
		fSimpleTextureShader->EnableProgram();
		fSimpleTextureShader->SetColorOffset(offset);
		fSimpleTextureShader->ModelView(model);
		fSimpleTextureShader->Projection(proj);
		glBindTexture(GL_TEXTURE_2D, geometry->texture);
		glDrawElements(GL_TRIANGLES, geometry->numIndices, GL_UNSIGNED_INT, 0);
		fSimpleTextureShader->SetColorOffset(glm::vec3(0,0,0)); // Restore default
		fSimpleTextureShader->DisableProgram();
	}
	glBindVertexArray(0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
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
	glScissor(x, gViewport[3]-(y+height), width, height);
}

bool RocketRenderInterface::LoadTexture(Rocket::Core::TextureHandle& texture_handle, Rocket::Core::Vector2i& texture_dimensions, const Rocket::Core::String& source)
{
	// std::string path = "dialogs/" + std::string(source.CString());
	auto bmp = loadBMP(source.CString());
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
	glBindTexture(GL_TEXTURE_2D, texture);
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
