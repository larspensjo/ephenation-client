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

#include <string>

#define CLIENT_MAJOR_VERSION 4
#define CLIENT_MINOR_VERSION 3

namespace Rocket {
	namespace Controls {
		class ElementFormControlSelect;
	}
};

using std::string;

class OptionsDialog;

class Options {
public:
	void Init(const std::string &fileName);

	// These are the options that will be saved at shutdown. They can be changed in any way
	// with no effect on the current game play, as they are not otherwise used.
	static Options sfSave;

	// These are parameters initialized from the ini file
	std::string fEmail;
	std::string fLicenseKey;
	float fViewingDistance;
	float fCameraDistance; // Used for third person view
	float fWhitePoint; // The limit that shall be considered as white in the HDR image
	float fExposure;
	int fWindowWidth;
	int fWindowHeight;
	int fFontSize;
	int fMsgWinTransparency;
	int fMusicVolume;
	int fMusicOn;
	int fSoundFxVolume;
	int fEnableTestbutton;
	int fPerformance; // A value from 1 to 4.
	int fNewPerformance; // Next time, use this performance
	int fMaxLamps, fMaxShadows, fMaxFog;
	int fFullScreen;
	int fAmbientLight; // Going from 0 to 100
	int fSmoothTerrain;
	int fMergeNormals; // Merge normals in the terrain to make a smoother view
	int fAddNoise; // Add random terrain noise
	int fDynamicShadows;   // Use a shadowmap for dynamic shadow generation.
	int fStaticShadows;    // A simpler version of dynamic shadows.
	int fAnisotropicFiltering;
	int fVSYNC;
	int fOculusRift;
	unsigned fNumThreads; // Not really configurable
	signed long long fPlayerX, fPlayerY, fPlayerZ; // Remember the last known player position

	// Update the select control with available graphical modes.
	void ListGraphicModes(Rocket::Controls::ElementFormControlSelect *) const;

	// Set the graphics mode.
	void SetGraphicsMode(int);

	// Parse one option, and return true if it was a valid option.
	bool ParseOneOption(const string &key, const string &arg);
	Options(void);
	~Options(void);
	void Save(void); // Save the parsed data back to the file again

	// When the general performance is changed, other options may change as a result. Don't call
	// this function unless the user actively changes the performance index.
	void UpdatePerformance(int perf);
private:
	std::string fFileName;
};

// These are the options used during play. They are initialized at startup, but will not
// be saved at shutdown. That way, changing options during play will have no effect until next session.
extern Options gOptions;
