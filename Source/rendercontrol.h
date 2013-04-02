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

#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>
#include <entityx/Entity.h>

#include "animationmodels.h"

class ChunkShader;
class AddDynamicShadow;
class DeferredLighting;
class AddPointLight;
class AnimationShader;
class AddPointShadow;
class SkyBox;
class AddLocalFog;
class AddSSAO;
class MainUserInterface;
class DownSamplingLuminance;
class FBOFlat;
class GaussianBlur;

namespace Model {
	class Object;
}

namespace View {
	class ShadowRender;

/// @brief This is the top level View of the Model/View/Controller
///
/// Manage buffers and shaders for the various render passes
/// All drawing stages are controlled from this class. The main activities are:
/// -# A Frame Buffer Object to use as a target for the first rendering.
/// -# Call the functions that update the images in the FBO
/// -# Project the final result to the screen.
class RenderControl {
public:
	RenderControl();
	virtual ~RenderControl();

	/// Initialize the FBO to a specified size, and allocate buffers.
	void Resize(GLsizei width, GLsizei height);

	/// Initialize
	/// @param lumSamplingFactor The windows size is divided by this to get the size of the luminance map
	void Init(int lightSamplingFactor);

	void ShadowMapMatrix(const glm::mat4 &) const;

	/// Do the actual drawing
	/// @param underWater True if underwater effects shall be applied
	/// @param selectedObject This object shall be visibly marked
	/// @param showMap True if a map shall be shown
	/// @param mapWidth Pixels in width used when drawing a map
	/// @param ui Pointer to the user interface
	void Draw(bool underWater, entityx::Entity selectedObject, bool showMap, int mapWidth, MainUserInterface *ui);

	/// Update the camera position.
	/// Check if camera position is inside a wall.
	/// @param wheelDelta How much the player used the wheel
	void UpdateCameraPosition(int wheelDelta);

	/// Return true if the player is using third person view
	bool ThirdPersonView() const { return fCameraDistance > 2.0f; }
private:

	GLuint fboName;
	GLuint fDepthBuffer; // Render target buffers
	GLuint fDiffuseTexture, fPositionTexture, fNormalsTexture, fBlendTexture, fLightsTexture;
	GLsizei fWidth, fHeight;

	GLuint fDownSampleLumTexture1, fDownSampleLumTexture2;
	int fLightSamplingFactor;

	std::unique_ptr<AddDynamicShadow> fAddDynamicShadow;
	std::unique_ptr<ShadowRender> fShadowRender;
	std::unique_ptr<DeferredLighting> fDeferredLighting;
	std::unique_ptr<AddPointLight> fAddPointLight;
	std::unique_ptr<AddPointShadow> fAddPointShadow;
	std::unique_ptr<AddLocalFog> fAddLocalFog;
	std::unique_ptr<AddSSAO> fAddSSAO;
	std::unique_ptr<SkyBox> fSkyBox;
	std::unique_ptr<DownSamplingLuminance> fDownSamplingLuminance;
	std::unique_ptr<GaussianBlur> fGaussianBlur;
	// Use two FBOs to switch between for blurring
	std::unique_ptr<FBOFlat> fboDownSampleLum1, fboDownSampleLum2;
	ChunkShader *fShader;
	AnimationShader *fAnimation;
	AnimationModels fAnimationModels;

	float fCameraDistance; /// Actual camera distance behind the player
	float fRequestedCameraDistance; /// Requested camera distance behind the player

	void ComputeShadowMap(void);
	void ComputeAverageLighting(bool underWater);

	void drawClearFBO(void); // Initialize all images in the FBO
	void drawClear(bool underWater); // Clear the main window
	void drawNonTransparentLandscape(void);
	void drawDynamicShadows(void);
	void drawDeferredLighting(bool underWater, float whitepoint);
	void drawPointLights(void);
	void drawMonsters(void);
	void drawTransparentLandscape(void);
	void drawPointShadows(void);
	void drawPlayer(void);
	void drawOtherPlayers(void);
	void drawSkyBox(GLenum diffuse, GLenum position);
	void drawLocalFog(void);
	void drawSelection(const glm::vec3 &);
	void drawUI(MainUserInterface *ui);
	void drawMap(int mapWidth);
	void drawSSAO(void);
	void drawColoredLights() const;
};

}
