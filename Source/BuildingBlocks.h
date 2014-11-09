// Copyright 2012-2014 The Ephenation Authors
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

#include "OpenglBuffer.h"

class SimpleTextureShader;
class DrawFont;

namespace View {

 /// Present a list of available building blocks.
class BuildingBlocks {
public:
	static BuildingBlocks *Make(int numToDisplay); /// Make an instance, but not before OpenGL has been initialized
	void Draw(glm::mat4 &projection);
	void UpdateSelection(int pos); /// Update what building block is selected
	int CurrentBlockType(void) const;
private:
	void Init(int numToDisplay);
	static BuildingBlocks fgSingleton; // This is the pointer to the singleton.

	int fNumToDisplay = 0; 			// Number of building blocks to display
	SimpleTextureShader *fShader = 0;
	OpenglBuffer fOpenglBuffer;
	GLuint fVao = 0;
	GLuint fSelectedBlockText = 0;	// Current sentence to use to display the description of the selected block
	int fCurrentSelection = 0;
};

}
