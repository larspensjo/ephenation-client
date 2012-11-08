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

#include "shadowrender.h"
#include "ui/Error.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>
#include "shaders/shadowmapshader.h"
#include "shaders/AnimationShader.h"
#include "primitives.h"
#include "render.h"
#include "player.h"
#include "chunk.h"
#include "monsters.h"
#include "Options.h"

ShadowRender::ShadowRender(int width, int height) :
	fboName(0), fTexture(0), fMapWidth(width), fMapHeight(height) {
}

ShadowRender::~ShadowRender() {
	if (fboName != 0) {
		glDeleteFramebuffers(1, &fboName);
		glDeleteTextures(1, &fTexture);
	}
}

void ShadowRender::Init() {
	// Create the depth texture
	//-------------------------
	glGenTextures(1, &fTexture); gDebugTextures.push_back(fTexture); // Add this texture to the debugging list of textures
	glBindTexture(GL_TEXTURE_2D, fTexture);
	// Using a depth of less than 24 bits adds more random shadow flickering.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, fMapWidth, fMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	// There is no need for interpolation in the shadow map. It would give an interpolated value between a depth value and
	// something else, possible a long way off. An interpolated depth value is not meaningfull, as it will still give
	// a true/false result when compared to somwhere to cast the shadow.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	// No comparison function shall be used, or the depth value would be reported as 0 or 1 only. It is probably not
	// enabled by default anyway.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	// unbind, bind the default texture (0) instead
	glBindTexture(GL_TEXTURE_2D, 0);

	// Create the frame buffer object
	//-------------------------------
	glGenFramebuffers(1, &fboName);
	// enable this frame buffer as the current frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, fboName);
	// attach the texture to the frame buffer so that rendering saves into the texture
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	// check for sucecssful FBO creation
	auto result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (result != GL_FRAMEBUFFER_COMPLETE) {
		ErrorDialog("ShadowRender::Init: Framebuffer is not complete: %s\n", FrameBufferError(result));
		exit(1);
	}
	// bind the default frame buffer again
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	fShader = ShadowMapShader::Make();
}

void ShadowRender::Render(int width, int height, const AnimationModels *animationModels) {
	glm::mat4 R1 = glm::rotate(glm::mat4(1), -115.0f, glm::vec3(1.0f, 0.0f, 0.0f));       // Rotate world upwards, in direction of sun
	glm::mat4 R2 = glm::rotate(glm::mat4(1), -45.0f, glm::vec3(0.0f, 0.0f, 1.0f));        // Rotate world upwards, in direction of sun
	glm::mat4 T;
	if (gOptions.fStaticShadows)
		T = glm::translate(glm::mat4(1), glm::vec3(-CHUNK_SIZE/2.0f, -CHUNK_SIZE/2.0f, CHUNK_SIZE/2.0f));                  // Move world to put the player in the center
	else
		T = glm::translate(glm::mat4(1), -gPlayer.GetOffsetToChunk());                                        // Move world to put the player in the center
	glm::mat4 viewMat = R1 * R2 * T;
	// TODO: Why does the 'y' need to be mirrored? If not, triangles will be drawn in the wrong order, and culling of faces will get wrong.
	glm::mat4 projMat = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f/width, -2.0f/width, 2.0f/height));
	glm::mat4 shear = glm::shearZ3D(projMat, -1.0f, 0.577350269f);
	fProjViewMatrix = shear * viewMat;

	fShader->EnableProgram();
	fShader->ProjectionView(fProjViewMatrix);
	glBindFramebuffer (GL_FRAMEBUFFER, fboName);
	glDrawBuffer (GL_NONE);
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
		gMonsters.RenderMonsters(true, false, animationModels);
		gPlayer.Draw(anim, fShader.get(), false, animationModels);
		anim->EnableProgram(); // This is lost in the player drawing for weapons.
		anim->Shadowmap(false, fProjViewMatrix);
	}

	glViewport(0, 0, gViewport[2], gViewport[3]); // Restore default viewport.
	glBindFramebuffer (GL_FRAMEBUFFER, 0);
}

void ShadowRender::BindTexture(void) const {
	glBindTexture(GL_TEXTURE_2D, fTexture);
}

const glm::mat4 &ShadowRender::GetProViewMatrix(void) const {
	return fProjViewMatrix;
}
