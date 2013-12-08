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

#include "shadowrender.h"
#include "ui/Error.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include "shaders/shadowmapshader.h"
#include "shaders/AnimationShader.h"
#include "shaders/gaussblur.h"
#include "primitives.h"
#include "render.h"
#include "player.h"
#include "chunk.h"
#include "monsters.h"
#include "Options.h"
#include "fboflat.h"

using namespace View;

ShadowRender::ShadowRender(int width, int height) :
	fTexture1(0), fTexture2(0), fMapWidth(width), fMapHeight(height) {
}

ShadowRender::~ShadowRender() {
	if (fTexture1 != 0) {
		glDeleteTextures(1, &fTexture1);
		glDeleteTextures(1, &fTexture2);
	}
}

void ShadowRender::Init() {
	// Create the depth textures
	//--------------------------
	glGenTextures(1, &fTexture1); gDebugTextures.push_back(DebugTexture(fTexture1, "Shadow render texture 1")); // Add this texture to the debugging list of textures
	glBindTexture(GL_TEXTURE_2D, fTexture1);
	// Using a depth of less than 24 bits adds more random shadow flickering.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, fMapWidth, fMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// No comparison function shall be used, or the depth value would be reported as 0 or 1 only. It is probably not
	// enabled by default anyway.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

	glGenTextures(1, &fTexture2);
	glBindTexture(GL_TEXTURE_2D, fTexture2); gDebugTextures.push_back(DebugTexture(fTexture2, "Shadow render texture 2")); // Add this texture to the debugging list of textures
	// Using a depth of less than 24 bits adds more random shadow flickering.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, fMapWidth, fMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// No comparison function shall be used, or the depth value would be reported as 0 or 1 only. It is probably not
	// enabled by default anyway.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	// unbind, bind the default texture (0) instead
	glBindTexture(GL_TEXTURE_2D, 0);

	// Create the frame buffer objects
	//--------------------------------
	fbo1.reset(new FBOFlat);
	fbo1->AttachDepthBuffer(fTexture1);
	fbo2.reset(new FBOFlat);
	fbo2->AttachDepthBuffer(fTexture2);

	fShader = ShadowMapShader::Make();
}

void ShadowRender::Render(int width, int height, const AnimationModels *animationModels, GaussianBlur *blurShader) {
	glm::mat4 R1 = glm::rotate(glm::mat4(1), -115.0f, glm::vec3(1.0f, 0.0f, 0.0f));       // Rotate world upwards, in direction of sun
	glm::mat4 R2 = glm::rotate(glm::mat4(1), -45.0f, glm::vec3(0.0f, 0.0f, 1.0f));        // Rotate world upwards, in direction of sun
	glm::mat4 T;
	if (gOptions.fStaticShadows)
		T = glm::translate(glm::mat4(1), glm::vec3(-CHUNK_SIZE/2.0f, -CHUNK_SIZE/2.0f, CHUNK_SIZE/2.0f));                  // Move world to put the player in the center
	else
		T = glm::translate(glm::mat4(1), -Model::gPlayer.GetOffsetToChunk());                                        // Move world to put the player in the center
	glm::mat4 viewMat = R1 * R2 * T;
	// TODO: Why does the 'y' need to be mirrored? If not, triangles will be drawn in the wrong order, and culling of faces will get wrong.
	glm::mat4 projMat = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f/width, -2.0f/width, 2.0f/height));
	glm::mat4 shear = glm::shearZ3D(projMat, -1.0f, 0.577350269f);
	fProjViewMatrix = shear * viewMat;

	fShader->EnableProgram();
	fShader->ProjectionView(fProjViewMatrix);
	fbo1->EnableWriting(GL_NONE);
	glClear(GL_DEPTH_BUFFER_BIT); // Clear the depth buffer
	glViewport(0, 0, fMapWidth, fMapHeight); // set viewport to texture dimensions
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	DrawLandscapeForShadows(fShader.get());

	if (gOptions.fDynamicShadows) {
		// Don't render dynamic objects in static shadow mode
		AnimationShader *anim = AnimationShader::Make();
		anim->EnableProgram();
		anim->Shadowmap(true, fProjViewMatrix);
		Model::gMonsters.RenderMonsters(true, false, animationModels);
		Model::gPlayer.Draw(anim, fShader.get(), false, animationModels);
		anim->EnableProgram(); // This is lost in the player drawing for weapons.
		anim->Shadowmap(false, fProjViewMatrix);
	}

	// Blur the depth buffer, before the default viewport is restored.
	this->Blur(blurShader);

	glViewport(gViewport[0], gViewport[1], gViewport[2], gViewport[3]); // Restore default viewport.
	glBindFramebuffer (GL_FRAMEBUFFER, 0);
}

void ShadowRender::Blur(GaussianBlur *blurShader) {
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_ALWAYS); // Overwrite the old values
	fbo2->EnableWriting(GL_NONE); // Only update depth buffer
	glBindTexture(GL_TEXTURE_2D, fTexture1);
	blurShader->BlurHorizontal(11, 4.0f);
	fbo1->EnableWriting(GL_NONE); // Only update depth buffer
	glDepthFunc(GL_LESS); // Restore default
	glBindTexture(GL_TEXTURE_2D, fTexture2);
	blurShader->BlurVertical();
	glEnable(GL_CULL_FACE);
}

void ShadowRender::BindTexture(void) const {
	glBindTexture(GL_TEXTURE_2D, fTexture1);
}

const glm::mat4 &ShadowRender::GetProViewMatrix(void) const {
	return fProjViewMatrix;
}
