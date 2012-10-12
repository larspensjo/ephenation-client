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

#include <glm/gtc/matrix_transform.hpp>
#include "../primitives.h"
#include "../textures.h"
#include "../imageloader.h"
#include "RocketRenderInterface.h"

RocketRenderInterface::RocketRenderInterface() : fColorShader(0)
{
}

void RocketRenderInterface::Init() {
	fColorShader = ColorShader::Make();
	fModulatedTextureShader.Init();
}

void RocketRenderInterface::RenderGeometry(Rocket::Core::Vertex* vertices,  int num_vertices, int* indices, int num_indices, const Rocket::Core::TextureHandle texture, const Rocket::Core::Vector2f& translation)
{
	// Immediate rendering of geometry isn't used.
}

Rocket::Core::CompiledGeometryHandle RocketRenderInterface::CompileGeometry(Rocket::Core::Vertex* vertices, int num_vertices, int* indices, int num_indices, const Rocket::Core::TextureHandle texturehandle)
{
	// Invert texture y, to match OpenGL.
	for (int i=0; i<num_vertices; i++)
		vertices[i].tex_coord.y = 1-vertices[i].tex_coord.y;

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
		glVertexAttribPointer(fColorShader->VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof (Rocket::Core::Vertex), &vp->position.x);
		glEnableVertexAttribArray(fColorShader->COLOR_INDEX);
		glVertexAttribPointer(fColorShader->COLOR_INDEX, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof (Rocket::Core::Vertex), &vp->colour.red);
	} else {
		// Use the ModulatedTextureShader
		glEnableVertexAttribArray(ModulatedTextureShader::VERTEX_INDEX);
		glVertexAttribPointer(ModulatedTextureShader::VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof (Rocket::Core::Vertex), &vp->position.x);
		glEnableVertexAttribArray(ModulatedTextureShader::TEXTURE_INDEX);
		glVertexAttribPointer(ModulatedTextureShader::TEXTURE_INDEX, 2, GL_FLOAT, GL_FALSE, sizeof (Rocket::Core::Vertex), &vp->tex_coord.x);
		glEnableVertexAttribArray(ModulatedTextureShader::COLOR_INDEX);
		glVertexAttribPointer(ModulatedTextureShader::COLOR_INDEX, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof (Rocket::Core::Vertex), &vp->colour.red);
	}
	glBindVertexArray(0);
	checkError("RocketRenderInterface::CompileGeometry");

	return reinterpret_cast<Rocket::Core::CompiledGeometryHandle>(geometry);
}

void RocketRenderInterface::RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle geometry_ptr, const Rocket::Core::Vector2f& translation)
{
	glm::mat4 proj = glm::ortho(0.0f, gViewport[2], gViewport[3], 0.0f, -1.0f, 1.0f);
	glm::vec3 offset(translation.x, translation.y, 0.0f);
	glm::mat4 model = glm::translate(glm::mat4(1.0), offset);
	Geometry* geometry = reinterpret_cast<Geometry*>(geometry_ptr);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glBindVertexArray(geometry->vao);
	if (geometry->texture == 0) {
		fColorShader->EnableProgram();
		fColorShader->ModelView(model);
		fColorShader->Color(glm::vec4(0,0,0,0)); // Will make the color vertex attribute be used instead
		fColorShader->Projection(proj);
		glDrawElements(GL_TRIANGLES, geometry->numIndices, GL_UNSIGNED_INT, 0);
		fColorShader->DisableProgram();
	} else {
		fModulatedTextureShader.EnableProgram();
		fModulatedTextureShader.ModelView(model);
		fModulatedTextureShader.Projection(proj);
		glBindTexture(GL_TEXTURE_2D, geometry->texture);
		glDrawElements(GL_TRIANGLES, geometry->numIndices, GL_UNSIGNED_INT, 0);
		fModulatedTextureShader.DisableProgram();
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
	// Bitmaps generated by libRocket are upside down compared to OpenGL.
	unsigned char bitmap[source_dimensions.x * source_dimensions.y * 4];
	for (int x=0; x<source_dimensions.x; x++) {
		for (int y=0; y<source_dimensions.y; y++) {
			for (int c=0; c<4; c++) {
				bitmap[y*source_dimensions.x*4 + x*4 + c] = source[(source_dimensions.y-y-1)*source_dimensions.x*4 + x*4 + c];
			}
		}
	}
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, source_dimensions.x, source_dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);

	texture_handle = texture;
	return true;
}

void RocketRenderInterface::ReleaseTexture(Rocket::Core::TextureHandle texture_handle)
{
	GLuint texture = texture_handle;
	glDeleteTextures(1, &texture);
}
