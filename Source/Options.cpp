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

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <GL/glfw.h>
#include <thread>
#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <glm/glm.hpp>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "render.h"
#include "Options.h"
#include "assert.h"

using std::stringstream;
using std::ifstream;
using std::ofstream;
using std::string;
using std::endl;

Options Options::sfSave;
Options gOptions;

bool Options::ParseOneOption(const string &key, const string &arg) {
	if (key == "Login.email")
		fEmail = arg;
	else if (key == "Login.licensekey")
		fLicenseKey = arg;
	else if (key == "Login.testbutton")
		fEnableTestbutton = atoi(arg.c_str());
	else if (key == "Display.viewingdistance")
		fViewingDistance = atof(arg.c_str());
	else if (key == "Display.windowwidth")
		fWindowWidth = atoi(arg.c_str());
	else if (key == "Display.windowheight")
		fWindowHeight = atoi(arg.c_str());
	else if (key == "Display.fontsize")
		fFontSize = atoi(arg.c_str());
	else if (key == "Display.msgwindowtransparency")
		fMsgWinTransparency = atoi(arg.c_str());
	else if (key == "Display.cameradistance")
		fCameraDistance = atof(arg.c_str());
	else if (key == "Display.whitepoint")
		fWhitePoint = atof(arg.c_str());
	else if (key == "Display.exposure")
		fExposure = atof(arg.c_str());
	else if (key == "Display.vsync")
		fVSYNC = atof(arg.c_str());
	else if (key == "Display.oculusrift")
		fOculusRift = atoi(arg.c_str());
	else if (key == "Display.comfortmode")
		fComforMode = atoi(arg.c_str());
	else if (key == "Audio.musicvolume")
		fMusicVolume = atoi(arg.c_str());
	else if (key == "Audio.musicon")
		fMusicOn = atoi(arg.c_str());
	else if (key == "Audio.effectvolume")
		fSoundFxVolume = atoi(arg.c_str());
	else if (key == "Graphics.performance")
		fNewPerformance = fPerformance = atoi(arg.c_str());
	else if (key == "Graphics.maxlamps")
		fMaxLamps = atoi(arg.c_str());
	else if (key == "Graphics.maxshadows")
		fMaxShadows = atoi(arg.c_str());
	else if (key == "Graphics.maxfogs")
		fMaxFog = atoi(arg.c_str());
	else if (key == "Graphics.fullscreen")
		fFullScreen = atoi(arg.c_str());
	else if (key == "Graphics.ambient")
		fAmbientLight = atoi(arg.c_str());
	else if (key == "Graphics.SmoothTerrain")
		fSmoothTerrain = atoi(arg.c_str());
	else if (key == "Graphics.MergeNormals")
		fMergeNormals = atoi(arg.c_str());
	else if (key == "Graphics.AddNoise")
		fAddNoise = atoi(arg.c_str());
	else if (key == "Graphics.DynamicShadows")
		fDynamicShadows = atoi(arg.c_str());
	else if (key == "Graphics.StaticShadows")
		fStaticShadows = atoi(arg.c_str());
	else if (key == "Graphics.AnisotropicFiltering")
		fAnisotropicFiltering = atoi(arg.c_str());
	else if (key == "Player.position") {
		char *end = 0;
		fPlayerX = strtoll(arg.c_str(), &end, 10);
		if (*end == ',')
			fPlayerY = strtoll(end+1, &end, 10);
		if (*end == ',')
			fPlayerZ = strtoll(end+1, &end, 10);
	} else
		return false; // Not a recognized option
	return true;
}

void Options::Init(const std::string &fileName) {
	fFileName = fileName;
	ifstream optionsFile;
	optionsFile.open(fFileName.c_str());
	if (optionsFile.is_open()) {
		string line;
		string prefix;
		while(std::getline(optionsFile, line)) {
			if (line.length() == 0 || line.at(0) == '#')
				continue;
			if (line.at(0) == '[') {
				prefix = line.substr(1, line.length()-2) + ".";
				continue;
			}
			line = prefix + line;
			size_t pos = line.find('=');
			if (pos == line.npos)
				continue; // Illegal format, ignore this line
			string key = line.substr(0, pos);
			string arg = line.substr(pos+1);
			auto comment = arg.find('#');
			if (comment != arg.npos)
				arg = arg.substr(0,comment-1); // Truncate comment
			this->ParseOneOption(key, arg);
		}
		optionsFile.close();
	}
	if (fDynamicShadows)
		fStaticShadows = 0; // Override
	gOptions = *this; // Initialize with a copy
}

void Options::Save(void) {
	ASSERT(fFileName != "");
	if (fPerformance != fNewPerformance)
		this->UpdatePerformance(fNewPerformance);

	ofstream optionsFile;
	optionsFile.open(fFileName.c_str());
	optionsFile << "# This file describes default options\n";
	optionsFile << "# The file is automatically updated whenever a value has changed\n";
	optionsFile << "#\n";
	optionsFile << "# Data used by the login process\n";
	optionsFile << "[Login]\n";
	optionsFile << "email=" << fEmail << endl;
	optionsFile << "licensekey=" << fLicenseKey << endl;
	if (fEnableTestbutton != 0)
		optionsFile << "testbutton=" << fEnableTestbutton << endl; // Do not show this flag unless it has been set
	optionsFile << endl;
	optionsFile << "# Display settings\n";
	optionsFile << "[Display]\n";
	optionsFile << "viewingdistance=" << fViewingDistance << endl;
	optionsFile << "windowwidth=" << fWindowWidth << endl;
	optionsFile << "windowheight=" << fWindowHeight << endl;
	optionsFile << "fontsize=" << fFontSize << endl;
	optionsFile << "msgwindowtransparency=" << fMsgWinTransparency << endl;
	optionsFile << "cameradistance=" << fCameraDistance << endl;
	optionsFile << "whitepoint=" << fWhitePoint << endl;
	optionsFile << "exposure=" << fExposure << endl;
	optionsFile << "vsync=" << fVSYNC << endl;
	optionsFile << "oculusrift=" << fOculusRift << endl;
	optionsFile << "comfortmode=" << fComforMode << endl;
	optionsFile << endl;
	optionsFile << "# Control audio settings\n";
	optionsFile << "[Audio]\n";
	optionsFile << "musicvolume=" << fMusicVolume << endl;
	optionsFile << "musicon=" << fMusicOn << endl;
	optionsFile << "effectvolume=" << fSoundFxVolume << endl;
	optionsFile << endl;
	optionsFile << "# Control graphics settings\n";
	optionsFile << "[Graphics]\n";
	optionsFile << "performance=" << fPerformance << endl;
	optionsFile << "maxlamps=" << fMaxLamps << endl;
	optionsFile << "maxshadows=" << fMaxShadows << endl;
	optionsFile << "maxfogs=" << fMaxFog << endl;
	optionsFile << "fullscreen=" << fFullScreen << endl;
	optionsFile << "ambient=" << fAmbientLight << endl;
	optionsFile << "SmoothTerrain=" << fSmoothTerrain << endl;
	optionsFile << "MergeNormals=" << fMergeNormals << endl;
	optionsFile << "AddNoise=" << fAddNoise << endl;
	optionsFile << "DynamicShadows=" << fDynamicShadows << endl;
	optionsFile << "StaticShadows=" << fStaticShadows << endl;
	optionsFile << "AnisotropicFiltering=" << fAnisotropicFiltering << endl;
	optionsFile << endl;
	optionsFile << "# Remember some last known player data\n";
	optionsFile << "[Player]\n";
	optionsFile << "position=" << fPlayerX << "," << fPlayerY << "," << fPlayerZ << endl;
	optionsFile.close();
}

Options::Options(void) : fViewingDistance(50), fWindowWidth(1024), fWindowHeight(768), fFontSize(12), fMsgWinTransparency(30), fMusicVolume(35),
	fMusicOn(1), fSoundFxVolume(100), fEnableTestbutton(0), fPerformance(2), fMaxLamps(25), fMaxShadows(15), fMaxFog(10), fFileName("") {
	fFullScreen = 0;
	fCameraDistance = 5.0; // Initialize to a little behind the player
	fAmbientLight = 20;
	fSmoothTerrain = 1;
	fMergeNormals = 1;
	fAddNoise = 1;
	fDynamicShadows = 0;
	fStaticShadows = 1;
	fAnisotropicFiltering = 0;
	fWhitePoint = 8.0f;
	fExposure = 1.0f;
	fVSYNC = 0;
	fPlayerX = 0;
	fPlayerY = 0;
	fPlayerZ = 0;

#ifdef WIN32
	fNumThreads = 0;
#else
	fNumThreads = std::thread::hardware_concurrency(); // This doesn't work for MinGW gcc 4.6.
#endif
	if (fNumThreads == 0) {
#ifdef WIN32
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		fNumThreads = info.dwNumberOfProcessors;
#endif
#ifdef unix
		fNumThreads = sysconf( _SC_NPROCESSORS_ONLN );
#endif
	}

	this->UpdatePerformance(fPerformance); // Initialize various parameters
}

Options::~Options(void) {
}

void Options::UpdatePerformance(int perf) {
	fNewPerformance = fPerformance = perf;
	switch(perf) {
	default:
	case 1:
		fViewingDistance = 75;
		fMaxLamps = 50; fMaxShadows = 8; fMaxFog = 4;
		fDynamicShadows = 0;
		fStaticShadows = 0;
		break;
	case 2:
		fViewingDistance = 110;
		fMaxLamps = 100; fMaxShadows = 20; fMaxFog = 10;
		fDynamicShadows = 0;
		fStaticShadows = 1;
		break;
	case 3:
		fViewingDistance = 135;
		fMaxLamps = 150; fMaxShadows = 40; fMaxFog = 30;
		fDynamicShadows = 0;
		fStaticShadows = 1;
		break;
	case 4:
		fViewingDistance = 160;
		fMaxLamps = 300; fMaxShadows = MAX_RENDER_SHADOWS; fMaxFog = MAX_RENDER_FOGS;
		fDynamicShadows = 1;
		fStaticShadows = 0;
		break;
	}
	maxRenderDistance = fViewingDistance;

	// The following depends a lot on the number of cores, not graphics performance
	fSmoothTerrain = 0; fMergeNormals = 0; fAddNoise = 0;
	if (fNumThreads > 1) {
		fSmoothTerrain = 1;
		fMergeNormals = 1;
		fAddNoise = 1;
	}
}

// Maximum number of modes that we want to list
#define MAX_NUM_MODES 40
static GLFWvidmode sgViewModes[MAX_NUM_MODES];

void Options::ListGraphicModes(Rocket::Controls::ElementFormControlSelect *element) const {
	element->RemoveAll();
	int modecount; // Total number of modes found

	// List available video modes. Only allow those that are good for Ephenation.
	modecount = glfwGetVideoModes( sgViewModes, MAX_NUM_MODES );
	for( int i = 0; i < modecount; i ++ ) {
		// Only allow 24 bit modes and a window no smaller than 800x600.
		int bits = sgViewModes[i].RedBits + sgViewModes[i].GreenBits + sgViewModes[i].BlueBits;
		if (bits == 24 && sgViewModes[i].Width >= 800 && sgViewModes[i].Height >= 600) {
			stringstream ss;
			ss << i << ": " << sgViewModes[i].Width << "x" << sgViewModes[i].Height << " " << bits << " bits";
			string rml = ss.str().c_str();
			ss.str(""); // Erase the old string
			ss << i;
			int ind = element->Add(rml.c_str(), ss.str().c_str()); // The mode is stored as the value
			if (sgViewModes[i].Width == this->fWindowWidth && sgViewModes[i].Height == this->fWindowHeight && bits == 24) {
				element->SetSelection(ind);
			}
		}
	}
}

void Options::SetGraphicsMode(int graphicMode) {
	this->fWindowWidth = sgViewModes[graphicMode].Width;
	this->fWindowHeight = sgViewModes[graphicMode].Height;
}
