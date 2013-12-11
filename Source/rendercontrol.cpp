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

#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "primitives.h"
#include "rendercontrol.h"
#include "ui/Error.h"
#include "Options.h"
#include "uniformbuffer.h"
#include "shadowconfig.h"
#include "shaders/ChunkShader.h"
#include "shaders/adddynamicshadow.h"
#include "shaders/DeferredLighting.h"
#include "shaders/addpointlight.h"
#include "shaders/AnimationShader.h"
#include "shaders/TranspShader.h"
#include "shaders/addpointshadow.h"
#include "shaders/addlocalfog.h"
#include "shaders/addssao.h"
#include "shaders/downsmpllum.h"
#include "shaders/gaussblur.h"
#include "render.h"
#include "Options.h"
#include "shadowrender.h"
#include "chunk.h"
#include "player.h"
#include "monsters.h"
#include "timemeasure.h"
#include "shaders/skybox.h"
#include "shapes/quadstage1.h"
#include "otherplayers.h"
#include "textures.h"
#include "Map.h"
#include "DrawTexture.h"
#include "worsttime.h"
#include "msgwindow.h"
#include "ui/mainuserinterface.h"
#include "animationmodels.h"
#include "modes.h"
#include "fboflat.h"
#include "HudTransformation.h"

#define NELEM(x) (sizeof x / sizeof x[0])

using namespace View;

RenderControl::RenderControl() {
	fboName = 0;
	fDepthBuffer = 0;
	fDiffuseTexture = 0; fPositionTexture = 0; fNormalsTexture = 0; fBlendTexture = 0; fLightsTexture = 0;
	fAnimation = 0; fShader = 0;
	fSkyBox = 0;
	fCameraDistance = 0.0f;
	fRequestedCameraDistance = 0.0f;

	fDownSampleLumTexture1 = 0; fDownSampleLumTexture2 = 0;
}

RenderControl::~RenderControl() {
	if (fDepthBuffer != 0) {
		glDeleteFramebuffers(1, &fboName);
		glDeleteTextures(1, &fDepthBuffer);
		glDeleteTextures(1, &fDiffuseTexture);
		glDeleteTextures(1, &fPositionTexture);
		glDeleteTextures(1, &fNormalsTexture);
		glDeleteTextures(1, &fBlendTexture);
		glDeleteTextures(1, &fLightsTexture);
		glDeleteTextures(1, &fDownSampleLumTexture1);
		glDeleteTextures(1, &fDownSampleLumTexture2);
	}
}

void RenderControl::Init(int lightSamplingFactor, bool stereoView) {
	// This only has to be done first time
	glGenRenderbuffers(1, &fDepthBuffer);
	// glGenTextures(1, &fDownSampleLumTextureBlurred); gDebugTextures.push_back(fDownSampleLumTextureBlurred); // Add this texture to the debugging list of textures
	glGenTextures(1, &fDownSampleLumTexture1); gDebugTextures.push_back(DebugTexture(fDownSampleLumTexture1, "DownSampleLum1")); // Add this texture to the debugging list of textures
	glGenTextures(1, &fDownSampleLumTexture2); gDebugTextures.push_back(DebugTexture(fDownSampleLumTexture2, "DownSampleLum2")); // Add this texture to the debugging list of textures
	glGenTextures(1, &fDiffuseTexture); gDebugTextures.push_back(DebugTexture(fDiffuseTexture, "Deferred Diffuse")); // Add this texture to the debugging list of textures
	glGenTextures(1, &fPositionTexture); gDebugTextures.push_back(DebugTexture(fPositionTexture, "Deferred position"));
	glGenTextures(1, &fNormalsTexture); gDebugTextures.push_back(DebugTexture(fNormalsTexture, "Deferred normals"));
	glGenTextures(1, &fBlendTexture); gDebugTextures.push_back(DebugTexture(fBlendTexture, "Deferred blend"));
	glGenTextures(1, &fLightsTexture); gDebugTextures.push_back(DebugTexture(fLightsTexture, "Deferred lighting"));

	glGenFramebuffers(1, &fboName);

	fLightSamplingFactor = lightSamplingFactor;
	fAddDynamicShadow.reset(new AddDynamicShadow);
	fAddDynamicShadow->Init();
	fShader = ChunkShader::Make(); // Singleton
	fAnimation = AnimationShader::Make(); // Singleton
	fSkyBox.reset(new SkyBox);
	fSkyBox->Init();
	fDeferredLighting.reset(new DeferredLighting);
	fDeferredLighting->Init();
	fAddPointLight.reset(new AddPointLight);
	fAddPointLight->Init();
	fAddPointShadow.reset(new AddPointShadow);
	fAddPointShadow->Init();
	fAddLocalFog.reset(new AddLocalFog);
	fAddLocalFog->Init();
	fAddSSAO.reset(new AddSSAO);
	fAddSSAO->Init();
	fDownSamplingLuminance.reset(new DownSamplingLuminance);
	fDownSamplingLuminance->Init();
	fAnimationModels.Init();
	fRequestedCameraDistance = gOptions.fCameraDistance;
	if (stereoView)
		fRequestedCameraDistance = 0.0f;
	fGaussianBlur.reset(new GaussianBlur);
	fGaussianBlur->Init();

	if (gOptions.fDynamicShadows) {
		fShadowRender.reset(new ShadowRender(DYNAMIC_SHADOW_MAP_SIZE,DYNAMIC_SHADOW_MAP_SIZE));
		fShadowRender->Init();
	} else if (gOptions.fStaticShadows) {
		fShadowRender.reset(new ShadowRender(STATIC_SHADOW_MAP_SIZE,STATIC_SHADOW_MAP_SIZE));
		fShadowRender->Init();
	}
}

void RenderControl::Resize(GLsizei width, GLsizei height) {
	fWidth = width; fHeight = height;

	//==============================================================================
	// Create the G-buffers used by the deferred shader. Linear sampling is enabled
	// as there are some filters that down sample the textures.
	//==============================================================================

	// Generate and bind the texture depth information
	glBindRenderbuffer(GL_RENDERBUFFER, fDepthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	// Generate and bind the texture for diffuse
	glBindTexture(GL_TEXTURE_2D, fDiffuseTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); // Get a black border when outside
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// Generate and bind the texture for positions
	glBindTexture(GL_TEXTURE_2D, fPositionTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Generate and bind the texture for normals
	glBindTexture(GL_TEXTURE_2D, fNormalsTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Generate and bind the texture for blending data
	glBindTexture(GL_TEXTURE_2D, fBlendTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); // Get a black border when outside
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// Generate and bind the texture for lights
	glBindTexture(GL_TEXTURE_2D, fLightsTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Attach all textures and the depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, fboName);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fDepthBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fDiffuseTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, fPositionTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, fNormalsTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, fBlendTexture, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, fLightsTexture, 0);
	glReadBuffer(GL_NONE);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
		ErrorDialog("RenderControl::Resize: Main frameBuffer incomplete: %s (0x%x)\n", FrameBufferError(fboStatus), fboStatus);
		exit(1);
	}

	// Generate and bind the texture for lights
	glBindTexture(GL_TEXTURE_2D, fDownSampleLumTexture1);
	int w = gViewport[2] / fLightSamplingFactor;
	int h = gViewport[3] / fLightSamplingFactor;
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Generate and bind the texture for lights
	glBindTexture(GL_TEXTURE_2D, fDownSampleLumTexture2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Create the FBO used for drawing the luminance map.
	fboDownSampleLum1.reset(new FBOFlat);
	fboDownSampleLum1->AttachTexture(fDownSampleLumTexture1);
	fboDownSampleLum2.reset(new FBOFlat);
	fboDownSampleLum2->AttachTexture(fDownSampleLumTexture2);

	checkError("RenderControl::Resize");
}

void RenderControl::Draw(bool underWater, shared_ptr<const Model::Object> selectedObject, bool showMap, int mapWidth, MainUserInterface *ui, bool stereoView) {
	if (gShowFramework)
		glPolygonMode(GL_FRONT, GL_LINE);
	// Create all bitmaps setup in the frame buffer. This is all stage one shaders.
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboName);
	drawClearFBO();
	glViewport(0, 0, fWidth, fHeight);
	// If the player is dead, he will get a gray sky.
	int diffuseSky = GL_NONE;
	if (!Model::gPlayer.IsDead() && !Model::gPlayer.BelowGround() && !underWater)
		diffuseSky = GL_COLOR_ATTACHMENT0;
	drawSkyBox(diffuseSky, GL_COLOR_ATTACHMENT1, stereoView);

	// glEnable(GL_STENCIL_TEST);
	// glStencilFunc(GL_ALWAYS, STENCIL_NOSKY, STENCIL_NOSKY); // Set bit for no sky
	// glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // Replace bits when something is drawn
	gDrawObjectList.clear();
	drawNonTransparentLandscape(stereoView);
	if (this->ThirdPersonView() && gMode.Get() != GameMode::LOGIN)
		drawPlayer(); // Draw self, but not if camera is too close
	drawOtherPlayers();
	drawMonsters();
	drawTransparentLandscape(stereoView);
	if (ui) drawUI(ui); // Will draw into transparent layer
	if (fShowMouse)
		drawMousePointer();
	// glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // Make stencil read-only

	// Apply deferred shader filters.
	// glStencilFunc(GL_EQUAL, STENCIL_NOSKY, STENCIL_NOSKY); // Only execute when no sky and no UI
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	if ((gOptions.fDynamicShadows || gOptions.fStaticShadows) && !Model::gPlayer.BelowGround())
		drawDynamicShadows();
	drawPointLights();
	// drawSSAO(); // TODO: Not good enough yet.
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Restore default
	glDisable(GL_BLEND);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // Frame buffer has done its job now, results are in the diffuse image.
	// glDisable(GL_STENCIL_TEST);
	glViewport(gViewport[0], gViewport[1], gViewport[2], gViewport[3]); // Restore default viewport.

	// At this point, there is no depth buffer representing the geometry and no stencil. They are only valid inside the FBO.

	ComputeAverageLighting(underWater);
	if (gShowFramework)
		glPolygonMode(GL_FRONT, GL_FILL); // Restore normal drawing.
	// Draw the main result to the screen.
	drawDeferredLighting(underWater, gOptions.fWhitePoint);

	// Do some post processing
	if (!underWater)
		drawPointShadows();
	if (selectedObject)
		drawSelection(selectedObject->GetPosition());
	drawColoredLights();
	if (!underWater)
		drawLocalFog(); // This should come late in the drawing process, as we don't want light effects added to fog
	if (showMap)
		drawMap(mapWidth);
}

void RenderControl::drawClear(bool underWater) {
	float ambient = gOptions.fAmbientLight / 200.0f;
	if (underWater)
		glClearColor(0.0f, 0.1f, 0.5f, 1.0f);
	else
		glClearColor(ambient, ambient, ambient, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void RenderControl::drawClearFBO(void) {
	GLenum windowBuffClear[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(1, windowBuffClear); // diffuse buffer
	glClearColor(0.584f, 0.620f, 0.698f, 1.0f); // Gray background, will only be used where there is no sky box.
	glClear(GL_COLOR_BUFFER_BIT);

	glDrawBuffers(4, windowBuffClear+1); // Select all buffers but the diffuse buffer
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Set everything to zero.
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
}

void RenderControl::drawNonTransparentLandscape(bool stereoView) {
	static TimeMeasure tm("Landsc");
	tm.Start();
	GLenum windowBuffOpaque[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_NONE };
	glDrawBuffers(4, windowBuffOpaque); // Nothing is transparent here, do not produce any blending data on the 4:th render target.
	fShader->EnableProgram(); // Get program back again after the skybox.
	DrawLandscape(fShader, DL_NoTransparent, stereoView);
	tm.Stop();
}

void RenderControl::drawTransparentLandscape(bool stereoView) {
	static TimeMeasure tm("Transp");
	tm.Start();
	GLenum windowBuffOpaque[] = { GL_COLOR_ATTACHMENT3 }; // Only draw to the transparent buffer
	glDrawBuffers(1, windowBuffOpaque);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Use alpha 1 for source channel, as the colors are premultiplied by the alpha.
	glDepthMask(GL_FALSE);                // The depth buffer shall not be updated, or some transparent blocks behind each other will not be shown.
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fPositionTexture); // Position data is used by the transparent shader for distance computation.
	glActiveTexture(GL_TEXTURE0);
	gTranspShader.EnableProgram();
	gTranspShader.View(float(gCurrentFrameTime));
	DrawLandscape(&gTranspShader, DL_OnlyTransparent, stereoView);
	gTranspShader.DisableProgram();
	glDepthMask(GL_TRUE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Restore to default
	glDisable(GL_BLEND);
	tm.Stop();
}

void RenderControl::drawMonsters(void) {
	static TimeMeasure tm("Monst");
	tm.Start();
	GLenum windowBuffOpaque[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, windowBuffOpaque); // Nothing is transparent here, do not produce any blending data on the 4:th render target.
	Model::gMonsters.RenderMonsters(false, false, &fAnimationModels);
	tm.Stop();
}

void RenderControl::drawPlayer(void) {
	GLenum windowBuffOpaque[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, windowBuffOpaque); // Nothing is transparent here, do not produce any blending data on the 4:th render target.
	fAnimation->EnableProgram();
	Model::gPlayer.Draw(fAnimation, fShader, false, &fAnimationModels);
	fAnimation->DisableProgram();
}

void RenderControl::drawDynamicShadows() {
	static TimeMeasure tm("Dynshdws");
	tm.Start();
	GLenum windowBuffers[] = { GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(1, windowBuffers); // Nothing is transparent here, do not produce any blending data on the 4:th render target.
	if ((gOptions.fDynamicShadows || gOptions.fStaticShadows) && fShadowRender) {
		glActiveTexture(GL_TEXTURE4);
		fShadowRender->BindTexture();
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, fNormalsTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, fPositionTexture);
		glActiveTexture(GL_TEXTURE0); // Need to restore it or everything will break.
		glBindTexture(GL_TEXTURE_1D, GameTexture::PoissonDisk);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		fAddDynamicShadow->Draw(fShadowRender->GetProViewMatrix());
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
	}
	tm.Stop();
}

void RenderControl::drawDeferredLighting(bool underWater, float whitepoint) {
	static TimeMeasure tm("Deferred");
	tm.Start();
	// Prepare the input images needed
	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_1D, GameTexture::PoissonDisk);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, fLightsTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, fBlendTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fNormalsTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fPositionTexture);
	glActiveTexture(GL_TEXTURE0); // Need to restore it or everything will break.
	glBindTexture(GL_TEXTURE_2D, fDiffuseTexture);

	fDeferredLighting->EnableProgram();
	fDeferredLighting->InsideTeleport(gMode.Get() == GameMode::TELEPORT);
	fDeferredLighting->PlayerDead(Model::gPlayer.IsDead());
	fDeferredLighting->InWater(underWater);
	fDeferredLighting->SetWhitePoint(whitepoint);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	// Blend scene into background, depending on distance.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	fDeferredLighting->Draw();
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	fDeferredLighting->DisableProgram();
	tm.Stop();
}

void RenderControl::drawPointLights(void) {
	static TimeMeasure tm("Pntlght");
	tm.Start();
	GLenum windowBuffers[] = { GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(1, windowBuffers); // Draw only to the light buffer
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fNormalsTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fPositionTexture);
	glActiveTexture(GL_TEXTURE0); // Need to restore it or everything will break.
	glBindTexture(GL_TEXTURE_2D, fDiffuseTexture);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	// Iterate through all the objects, and add lights for some of them
	for (auto it = gDrawObjectList.begin(); it != gDrawObjectList.end(); it++) {
		switch(it->type) {
		case BT_Lamp1:
			// The coordinate is the base of the lamp. Move light souce a little upwards, to middle of lamp
			fAddPointLight->Draw(it->pos+glm::vec3(0, 0.2f, 0), LAMP1_DIST);
			break;
		case BT_Lamp2:
			// The coordinate is the base of the lamp. Move light souce a little upwards, to middle of lamp
			fAddPointLight->Draw(it->pos+glm::vec3(0, 0.3f, 0), LAMP2_DIST);
			break;
		case BT_Treasure:
			if (gOptions.fPerformance > 1) // Inhibit this for the low performance
				fAddPointLight->Draw(it->pos, 1.5f);
			break;
		case BT_Quest:
			if (gOptions.fPerformance > 1) // Inhibit this for the low performance
				fAddPointLight->Draw(it->pos, 1.5f);
			break;
		case BT_Teleport:
			if (gOptions.fPerformance > 1) // Inhibit this for the low performance
				fAddPointLight->Draw(it->pos, 3.0f);
			break;
		}
	}
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	tm.Stop();
}

void RenderControl::drawSSAO(void) {
	static TimeMeasure tm("SSAO ");
	tm.Start();
	GLenum windowBuffers[] = { GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(1, windowBuffers); // Nothing is transparent here, do not produce any blending data on the 4:th render target.
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fNormalsTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fPositionTexture);
	glActiveTexture(GL_TEXTURE0); // Need to restore it or everything will break.
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	fAddSSAO->Draw();
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	tm.Stop();
}

void RenderControl::drawPointShadows(void) {
	static TimeMeasure tm("Pntshdw");
	tm.Start();
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fPositionTexture);
	glActiveTexture(GL_TEXTURE0); // Need to restore it or everything will break.
	glm::vec4 *list = gShadows.GetList();
	int count = gShadows.GetCount();
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	for (int i=0; i < count; i++) {
		fAddPointShadow->DrawPointShadow(list[i]);
	}
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	tm.Stop();
}

void RenderControl::drawLocalFog(void) {
	static TimeMeasure tm("LclFog");
	tm.Start();
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fPositionTexture);
	glActiveTexture(GL_TEXTURE0); // Need to restore it or everything will break.
	glBindTexture(GL_TEXTURE_2D, fDownSampleLumTexture1);
	glm::vec4 *list = gFogs.GetList();
	int count = gFogs.GetCount();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	for (int i=0; i < count; i++) {
		fAddLocalFog->Draw(list[i]);
	}
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	tm.Stop();
}

void RenderControl::ComputeShadowMap() {
	static TimeMeasure tm("Shadowmap");
	tm.Start();
	if (gOptions.fDynamicShadows && fShadowRender) {
		fShadowRender->Render(352, 160, &fAnimationModels, fGaussianBlur.get());
	}
	if (gOptions.fStaticShadows && fShadowRender) {
		static ChunkCoord prev = {0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
		static double prevTime;
		ChunkCoord curr;
		Model::gPlayer.GetChunkCoord(&curr);
		if (curr.x != prev.x || curr.y != prev.y || curr.z != prev.z || gCurrentFrameTime-1.0 > prevTime) {
			// Only update the shadowmap when the player move to another chunk, or after a time out
			fShadowRender->Render(224, 160, &fAnimationModels, fGaussianBlur.get());
			prev = curr;
			prevTime = gCurrentFrameTime;
		}
	}
	tm.Stop();
}

void RenderControl::drawOtherPlayers(void) {
	Model::gOtherPlayers.RenderPlayers(fAnimation, false);
}

void RenderControl::drawSelection(const glm::vec3 &coord) {
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fNormalsTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fPositionTexture);
	glActiveTexture(GL_TEXTURE0); // Need to restore it or everything will break.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glm::vec4 point = glm::vec4(coord, 2.0f); // 2 blocks wide selection circle
	fAddPointShadow->DrawMonsterSelection(point);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

void RenderControl::drawColoredLights() const {
	static TimeMeasure tm("Clrlght");
	tm.Start();
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fNormalsTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fPositionTexture);
	glActiveTexture(GL_TEXTURE0); // Need to restore it or everything will break.
	glEnable(GL_BLEND);
	glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	glDisable(GL_CULL_FACE);
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glm::vec4 *list = gLightSources.GetList();
	int count = gLightSources.GetCount();
	for (int i=0; i < count; i++) {
		// The 'w' component contains both the radius and the type of color.
		int composite = int(list[i].w);
		int radius = composite % 100;
		int type = composite - radius;
		list[i].w = radius; // Only radius shall be sent to the renderer.
		switch (type) {
		case LightSources::RedOffset:
			fAddPointShadow->DrawRedLight(list[i]);
			break;
		case LightSources::GreenOffset:
			fAddPointShadow->DrawGreenLight(list[i]);
			break;
		case LightSources::BlueOffset:
			fAddPointShadow->DrawBlueLight(list[i]);
			break;
		}
	}
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	tm.Stop();
}

// This function can be used for two things:
// 1. Drawing the skybox color
// 2. Drawing the skybox position data
// Specify color target with 'diffuse' and position data target with 'position'.
void RenderControl::drawSkyBox(GLenum diffuse, GLenum position, bool disableDistortion) {
    static TimeMeasure tm("SkyBox");
    tm.Start();
    GLenum windowBuffOpaque[] = { diffuse, position };
    glDrawBuffers(2, windowBuffOpaque);
    glDepthRange(1, 1); // This will move the sky box to the far plane, exactly
    glDepthFunc(GL_LEQUAL); // Seems to be needed, or depth value 1.0 will not be shown.
    glDisable(GL_CULL_FACE); // Skybox is drawn with the wrong culling order.
	glEnable(GL_DEPTH_TEST);
    fSkyBox->Draw();
    glEnable(GL_CULL_FACE);
    glDepthFunc(GL_LESS);
    glDepthRange(0, 1);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(gViewport[0], gViewport[1], gViewport[2], gViewport[3]); // Restore default viewport.
	// Print the skybox texture immediately
	DrawTexture *texture = DrawTexture::Make();
	glBindTexture(GL_TEXTURE_2D, fDiffuseTexture);
	texture->DrawScreen(disableDistortion);
	glViewport(0, 0, fWidth, fHeight);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboName);
    tm.Stop();
}

void RenderControl::drawUI(MainUserInterface *ui) {
	static WorstTime tm("MainUI");
	tm.Start();
	GLenum windowBuffOpaque[] = { GL_COLOR_ATTACHMENT3 }; // Only draw to the transparent buffer
	glDrawBuffers(1, windowBuffOpaque);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);                // The depth buffer shall not be updated, or some transparent blocks behind each other will not be shown.
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	ui->Draw(gMode.Get() != GameMode::LOGIN);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	tm.Stop();
}

void RenderControl::drawMap(int mapWidth) {
	// Very inefficient algorithm, computing the map every frame.
	// The FBO is overwritten for this.
	std::unique_ptr<Map> map(new Map);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboName);
	GLenum windows[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, windows); // Only diffuse color needed
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	fShader->EnableProgram();
	map->Create(fAnimation, fShader, _angleHor, mapWidth, &fAnimationModels);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, fDiffuseTexture); // Override
	map->Draw(0.6f);
}

void RenderControl::drawMousePointer() const {
	glBindTexture(GL_TEXTURE_2D, GameTexture::MousePointerId); // Override
	// 0,0 is bottom left of the bitmap, but we want it to be top left.
	glm::mat4 moveToTip = glm::translate(glm::mat4(1), glm::vec3(0, -1, 0));
	glm::mat4 scaleToPixel = glm::scale(glm::mat4(1), glm::vec3(16, -32, 1)); // This is the size of the bitmap
	// 0,0 is middle of screen, whereas mouse coordinates starts at upper left of screen.
	glm::mat4 translToMouse = glm::translate(glm::mat4(1), glm::vec3(float(fMouseX)-gViewport[2]/2, float(fMouseY)-gViewport[3]/2, 0.0f));
	glm::mat4 model = View::gHudTransformation.GetGUITransform() * translToMouse * scaleToPixel * moveToTip;

	GLenum windowBuffOpaque[] = { GL_COLOR_ATTACHMENT3 }; // Only draw to the transparent buffer
	glDrawBuffers(1, windowBuffOpaque);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);                // The depth buffer shall not be updated, or some transparent blocks behind each other will not be shown.
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	DrawTexture::Make()->Draw(gProjectionMatrix, model);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void RenderControl::UpdateCameraPosition(int wheelDelta, bool stereoView, float yaw, float pitch, float roll) {
	fRequestedCameraDistance += wheelDelta;
	if (fRequestedCameraDistance < 0.0f)
		fRequestedCameraDistance = 0.0f;
	if (fRequestedCameraDistance > 20.0f)
		fRequestedCameraDistance = 20.0f; // Don't allow unlimited third person view distance
	static double last = 0.0;
	double delta = (gCurrentFrameTime - last)*20; // Number of blocks/s
	last = gCurrentFrameTime;
	if (delta > fRequestedCameraDistance - fCameraDistance)
		delta = fRequestedCameraDistance - fCameraDistance;
	// If the requested camera distance is bigger than the current, then slowly move out.
	if (fRequestedCameraDistance > fCameraDistance)
		fCameraDistance += delta;
	else
		fCameraDistance = fRequestedCameraDistance;
	gOptions.fCameraDistance = fRequestedCameraDistance;
	Options::sfSave.fCameraDistance = fRequestedCameraDistance; // Make sure it is saved

	ChunkCoord cc;
	Model::gPlayer.GetChunkCoord(&cc);

	glm::vec3 playerOffset = Model::gPlayer.GetOffsetToChunk();
	glm::vec3 pd(playerOffset.x, -playerOffset.z, playerOffset.y); // Same offset, but in Ephenation server coordinates

	glm::mat4 T1 = glm::translate(glm::mat4(1), playerOffset);
	glm::mat4 R1 = glm::rotate(glm::mat4(1), _angleVert, glm::vec3(-1.0f, 0.0f, 0.0f));
	glm::mat4 R2 = glm::rotate(glm::mat4(1), _angleHor, glm::vec3(0.0f, -1.0f, 0.0f));
	glm::mat4 T2 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, fCameraDistance));
	glm::vec4 camera = T1 * R2 * R1 * T2 * glm::vec4(0,0,0,1);

	gUniformBuffer.Camera(camera);

	glm::vec3 cd(camera.x, -camera.z, camera.y); // Camera delta in Ephenation coordinates.

	glm::vec3 norm = glm::normalize(cd-pd); // Vector from player to camera

	const float step = 0.1f;
	for (float d = step; d <= this->fCameraDistance; d = d + step) {
		signed long long x, y, z;
		x = Model::gPlayer.x + (signed long long)(d * norm.x*BLOCK_COORD_RES);
		y = Model::gPlayer.y + (signed long long)(d * norm.y*BLOCK_COORD_RES);
		z = Model::gPlayer.z + (signed long long)(d * norm.z*BLOCK_COORD_RES);
		if (Chunk::GetChunkAndBlock(x, y, z) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (Chunk::GetChunkAndBlock(x+1, y, z) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (Chunk::GetChunkAndBlock(x, y+1, z) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (Chunk::GetChunkAndBlock(x, y, z+1) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (Chunk::GetChunkAndBlock(x-1, y, z) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (Chunk::GetChunkAndBlock(x, y-1, z) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (Chunk::GetChunkAndBlock(x, y, z-1) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
	}

	gViewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -fCameraDistance));
	if (stereoView)
		gViewMatrix = glm::rotate(gViewMatrix, roll, glm::vec3(0.0f, 0.0f, -1.0f));
	if (stereoView)
		gViewMatrix = glm::rotate(gViewMatrix, pitch, glm::vec3(-1.0f, 0.0f, 0.0f)); // Don't let mouse affect pitch
	else
		gViewMatrix = glm::rotate(gViewMatrix, _angleVert, glm::vec3(1.0f, 0.0f, 0.0f));
	gViewMatrix = glm::rotate(gViewMatrix, _angleHor-yaw, glm::vec3(0.0f, 1.0f, 0.0f));
	gViewMatrix = glm::translate(gViewMatrix, -playerOffset);
}

void RenderControl::ComputeAverageLighting(bool underWater) {
	static TimeMeasure tm("AvgLght");
	tm.Start();
	fboDownSampleLum1->EnableWriting(GL_COLOR_ATTACHMENT0);
	if (Model::gPlayer.BelowGround())
		glClearColor(gOptions.fAmbientLight / 200.0f, 0.0f, 0.0f, 1.0f );
	else
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT);
	int w = fWidth / fLightSamplingFactor;
	int h = fHeight / fLightSamplingFactor;
	glViewport(0, 0, w, h); // set viewport to texture dimensions
	GLenum windowBuffer[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, windowBuffer);
	// Prepare the input images needed
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, fLightsTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, fBlendTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fNormalsTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fPositionTexture);
	glActiveTexture(GL_TEXTURE0); // Need to restore it or everything will break.
	glBindTexture(GL_TEXTURE_2D, fDiffuseTexture);

	fDownSamplingLuminance->EnableProgram();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	fDownSamplingLuminance->Draw();

	int tap = h/4;
	float sigma = float(tap)/3.0f;
	fboDownSampleLum2->EnableWriting(GL_COLOR_ATTACHMENT0);
	glBindTexture(GL_TEXTURE_2D, fDownSampleLumTexture1);
	fGaussianBlur->BlurHorizontal(tap, sigma);
	fboDownSampleLum1->EnableWriting(GL_COLOR_ATTACHMENT0);
	glBindTexture(GL_TEXTURE_2D, fDownSampleLumTexture2);
	fGaussianBlur->BlurVertical();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glViewport(gViewport[0], gViewport[1], gViewport[2], gViewport[3]); // Restore default viewport.
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	tm.Stop();
}
