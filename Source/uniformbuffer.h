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

//
// Provide data and functionality common to shaders.
//

#pragma once

#include <GL/glew.h>

#include <glm/glm.hpp>

/// Manage a uniform buffer
/// See common.glsl, with the shader definition of the buffer
class UniformBuffer {
public:
	/// Clear all data
	UniformBuffer();

	/// Free buffers
	~UniformBuffer();

	/// Do the actual buffer allocation
	void Init();

	/// Configure the parameters needed for Oculus Rift
	/// @param dist The 4 constants for the barrel distortion
	/// @param lensCenter The horizontal distance from the view center to the lens center
	void SetOVRConstants(const float *dist, float lensCenter);

	/// Used for stereo view
	void SelectEye(bool left) { fLeftEeye = left; }

	/// Update all data in the uniform buffer
	/// It is a const function as no parameters in the class are changed.
	/// @param ovrMode true if screen is adapted for Oculus Rift display
	void Update(bool ovrMode) const;

	/// Call once for each program during initialization
	void UniformBlockBinding(GLuint program, GLuint idx);

	/// Set the camera data.
	/// It will not be used until Update() has been called.
	void Camera(const glm::vec4 &);

	/// Calibrate shaders during development. Not used in normal game mode.
	void SetcalibrationFactor(float fact) { fDebugfactor = fact; }

	/// Set the near and far plane cut off of the projection
	void SetFrustum(float nearCutoff, float farCutoff) { fNearCutoff = nearCutoff; fFarCutoff = farCutoff; }
private:
	void BindBufferBase(void) const;

	GLuint fUBOBuffer;

	glm::vec4 fCamera;
	glm::vec4 fDistortion = glm::vec4(1.0f, 0.22f, 0.24f, 0.0f);
	glm::vec2 fOvrLensCenter = glm::vec2(0.15f, 0.0f); // Best guess.
	bool fLeftEeye = true;
	float fDebugfactor = 1.0f;
	float fNearCutoff, fFarCutoff;
};

extern UniformBuffer gUniformBuffer;
