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

#include <memory>
// This is a kludge.
// Sometimes, primitives.h will be included after FL.h, and FL.h will include fl_utf8.h that will define
// hypot as something else. That is not compatible with <cmath>, which is using ::hypot
#undef hypot
#include <glm/glm.hpp>

#include "animationmodels.h"

class ChunkShader;
class AddDynamicShadow;
class ShadowRender;
class DeferredLighting;
class Object;
class AddPointLight;
class AnimationShader;
class AddPointShadow;
class SkyBox;
class AddLocalFog;
class AddSSAO;
class MainUserInterface;

/**
 * @brief Manage buffers and shaders for the various render passes
 *
 * All drawing stages are controlled from this class. The main activities are:
 * 1. A Frame Buffer Object to use as a target for the first rendering.
 * 2. Call the functions that update the images in the FBO
 * 3. Project the final result to the screen.
 */
class RenderControl {
public:
	RenderControl();
	virtual ~RenderControl();

	// Initialize the FBO to a specified size, and allocate buffers.
	void Resize(GLsizei width, GLsizei height);

	void Init(void);
	void ShadowMapMatrix(const glm::mat4 &) const;

	GLuint Debug(void);
	void Draw(bool underWater, bool thirdPersonView, std::shared_ptr<const Object> selectedObject, bool showMap, int mapWidth, MainUserInterface *ui);
private:

	// Free the buffers. Also needed to reinitialize for a new size.
	void FreeFBO();
	GLuint fboName;
	GLuint fDepthBuffer; // Render target buffers
	GLuint fDiffuseTexture, fPositionTexture, fNormalsTexture, fBlendTexture, fLightsTexture;
	GLsizei fWidth, fHeight;

	std::unique_ptr<AddDynamicShadow> fAddDynamicShadow;
	std::unique_ptr<ShadowRender> fShadowRender;
	std::unique_ptr<DeferredLighting> fDeferredLighting;
	std::unique_ptr<AddPointLight> fAddPointLight;
	std::unique_ptr<AddPointShadow> fAddPointShadow;
	std::unique_ptr<AddLocalFog> fAddLocalFog;
	std::unique_ptr<AddSSAO> fAddSSAO;
	std::unique_ptr<SkyBox> fSkyBox;
	ChunkShader *fShader;
	AnimationShader *fAnimation;
	AnimationModels fAnimationModels;

	void ComputeShadowMap(void);

	void drawClearFBO(void); // Initialize all images in the FBO
	void drawClear(void); // Clear the main window
	void drawNonTransparentLandscape(void);
	void drawDynamicShadows(void);
	void drawDeferredLighting(bool underWater, float whitepoint);
	void drawPointLights(void);
	void drawMonsters(void);
	void drawTransparentLandscape(void);
	void drawPointShadows(void);
	void drawPlayer(void);
	void drawOtherPlayers(void);
	void drawSkyBox(void);
	void drawLocalFog(void);
	void drawSelection(const glm::vec3 &);
	void drawUI(MainUserInterface *ui);
	void drawMap(int mapWidth);
	void drawSSAO(void);
	void drawColoredLights() const;
};
