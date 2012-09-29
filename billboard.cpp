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

#include <GL/glew.h>

#include "primitives.h"
#include "billboard.h"
#include "ui/Error.h"
#include "glm/gtc/matrix_transform.hpp"
#include "shaders/StageOneShader.h"
#include "uniformbuffer.h"
#include "textures.h"
#include "shapes/Tree.h"
#include "render.h"
#include "shapes/quadstage1.h"

#define ROW(p) (p / fNumPictures)
#define COL(p) (p % fNumPictures)

Billboard gBillboard(128, 3);

Billboard::Billboard(int pixelWidth, int numPictures) : fboAtlas(0), fboTemp(0), fAtlasId(0), fTempTextureId(0), fDepthBuffer(0),
	fPixelWidth(pixelWidth), fNumPictures(numPictures) {
}

Billboard::~Billboard() {
	if (fboAtlas != 0) {
		glDeleteTextures(1, &fAtlasId);
		glDeleteTextures(1, &fTempTextureId);
		glDeleteRenderbuffers(1, &fDepthBuffer);
		glDeleteFramebuffers(1, &fboAtlas);
		glDeleteFramebuffers(1, &fboTemp);
	}
}

void Billboard::Init(void) {
	// Create the atlas of all pictures.
	GLenum internalFormat = GL_RGBA2;
	int totalWidth = fPixelWidth * fNumPictures;
	glGenTextures(1, &fAtlasId); gDebugTextures.push_back(fAtlasId);
again:
	glBindTexture(GL_TEXTURE_2D, fAtlasId);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, totalWidth, totalWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Attach the texture to the FBO.
	glGenFramebuffers(1, &fboAtlas);
	glBindFramebuffer(GL_FRAMEBUFFER, fboAtlas);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fAtlasId, 0);
	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus == GL_FRAMEBUFFER_UNSUPPORTED && internalFormat == GL_RGBA2) {
		// Some cars doesn't support GL_RGBA2.
		internalFormat = GL_RGBA;
		glDeleteFramebuffers(1, &fboAtlas); // It will be created again
		goto again;
	}
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		ErrorDialog("Billboard::Init: FrameBuffer atlas incomplete: %s (0x%x)\n", FrameBufferError(fboStatus), fboStatus);
	}

	// Clear the atlas. Probably not needed as every picture will be cleared eventually.
	glViewport(0, 0, totalWidth, totalWidth); // set viewport to texture dimensions
	GLenum windowBuffer[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, windowBuffer);
	glClearColor(1,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, gViewport[2], gViewport[3]); // Restore default viewport.

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Create a FBO for temporary drawing

	glGenTextures(1, &fTempTextureId);
	glBindTexture(GL_TEXTURE_2D, fTempTextureId); gDebugTextures.push_back(fTempTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, fPixelWidth, fPixelWidth, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// The size of the depth buffer shall match the size of the temporary texture buffer
	glGenRenderbuffers(1, &fDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, fDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, fPixelWidth, fPixelWidth);

	glGenFramebuffers(1, &fboTemp);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboTemp);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fDepthBuffer);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fTempTextureId, 0);
	fboStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		ErrorDialog("Billboard::Init: FrameBuffer temp incomplete: %s (0x%x)\n", FrameBufferError(fboStatus), fboStatus);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	// Initialize list of free pictures.
	for (int i=0; i<fNumPictures*fNumPictures; i++)
		fFreePictures.push_back(i);
}

int Billboard::AllocatePermanentPicture(void) {
	if (fFreePictures.empty())
		return -1;
	int p = fFreePictures.front();
	fFreePictures.pop_front();
	return p;
}

void Billboard::FreePermanentPicture(int p) {
	fFreePictures.push_back(p);
}

void Billboard::EnableForDrawing(void) {
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboTemp); // Enable for both reading and writing
	glViewport(0, 0, fPixelWidth, fPixelWidth); // set viewport to texture dimensions
	GLenum windowBuffer[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, windowBuffer);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void Billboard::UppdateBillboard(int picture) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboTemp);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboAtlas);
	GLint dstY0 = ROW(picture)*fPixelWidth;
	GLint dstX0 = COL(picture)*fPixelWidth;
	glBlitFramebuffer(0,0,fPixelWidth,fPixelWidth, dstX0, dstY0, dstX0+fPixelWidth, dstY0+fPixelWidth, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, gViewport[2], gViewport[3]); // Restore default viewport.
}

void Billboard::BindTexture(void) {
	glBindTexture(GL_TEXTURE_2D, fAtlasId);
}

void Billboard::InitializeTextures(StageOneShader *shader) {
	int picture;
	glm::mat4 fProjViewMatrix = gProjectionMatrix;
	shader->EnableProgram();
	shader->Model(glm::mat4(1));
	int width = 20, depth = 40;
	fPredefined.resize(numElements);
	glm::mat4 projMat(1);
	projMat = glm::translate(projMat, glm::vec3(0,-1.0f, 0));
	projMat = glm::scale(projMat, glm::vec3(2.0f/width, 2.0f/width, 2.0f/depth));
	gViewMatrix = glm::mat4(1);
	glm::mat4 ShadowTilt = glm::rotate(glm::mat4(1), 45.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 ShadowRot = glm::rotate(glm::mat4(1), 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));

	gProjectionMatrix = projMat;

	// Big tree
	picture = gBillboard.AllocatePermanentPicture();
	gBillboard.EnableForDrawing();
	gUniformBuffer.Update();
	glBindTexture(GL_TEXTURE_2D, GameTexture::TreeBarkId); // This will be used for trunk and all branches.
	Tree::sfBigTree.Draw();
	gBillboard.UppdateBillboard(picture);
	fPredefined.at(tree3) = picture;

	// Medium tree
	picture = gBillboard.AllocatePermanentPicture();
	gBillboard.EnableForDrawing();
	gUniformBuffer.Update();
	glBindTexture(GL_TEXTURE_2D, GameTexture::TreeBarkId); // This will be used for trunk and all branches.
	Tree::sfMediumTree.Draw();
	gBillboard.UppdateBillboard(picture);
	fPredefined.at(tree2) = picture;

	// Small tree
	picture = gBillboard.AllocatePermanentPicture();
	gBillboard.EnableForDrawing();
	gUniformBuffer.Update();
	glBindTexture(GL_TEXTURE_2D, GameTexture::TreeBarkId); // This will be used for trunk and all branches.
	Tree::sfSmallTree.Draw();
	gBillboard.UppdateBillboard(picture);
	fPredefined.at(tree1) = picture;

	gProjectionMatrix = projMat * ShadowTilt * ShadowRot;

	// Big tree shadow
	picture = gBillboard.AllocatePermanentPicture();
	gBillboard.EnableForDrawing();
	gUniformBuffer.Update();
	glBindTexture(GL_TEXTURE_2D, GameTexture::TreeBarkId); // This will be used for trunk and all branches.
	Tree::sfBigTree.Draw();
	gBillboard.UppdateBillboard(picture);
	fPredefined.at(tree3Shadow) = picture;

	// Medium tree shadow
	picture = gBillboard.AllocatePermanentPicture();
	gBillboard.EnableForDrawing();
	gUniformBuffer.Update();
	glBindTexture(GL_TEXTURE_2D, GameTexture::TreeBarkId); // This will be used for trunk and all branches.
	Tree::sfMediumTree.Draw();
	gBillboard.UppdateBillboard(picture);
	fPredefined.at(tree2Shadow) = picture;

	// Small tree shadow
	picture = gBillboard.AllocatePermanentPicture();
	gBillboard.EnableForDrawing();
	gUniformBuffer.Update();
	glBindTexture(GL_TEXTURE_2D, GameTexture::TreeBarkId); // This will be used for trunk and all branches.
	Tree::sfSmallTree.Draw();
	gBillboard.UppdateBillboard(picture);
	fPredefined.at(tree1Shadow) = picture;

	gProjectionMatrix = fProjViewMatrix; // Restore
	gUniformBuffer.Update();
}

void Billboard::Draw(StageOneShader *shader, const glm::mat4 &modelMatrix, enum Predefined picture) {
	int ind = fPredefined[picture];
	float srcY0 = float(ROW(ind)) / fNumPictures;
	float srcX0 = float(COL(ind)) / fNumPictures;
	glm::mat4 model = modelMatrix;
	model = glm::translate(model, glm::vec3(0, 10, 0));
	model = glm::scale(model, glm::vec3(20, 20, 20));
	shader->Model(model);
	shader->TextureOffsetMulti(srcX0, srcY0, 1.0f/fNumPictures);
	gBillboard.BindTexture();
	gQuadStage1.DrawSingleSide();
	shader->TextureOffsetMulti(0, 0, 1.0f); // Restore default
}
