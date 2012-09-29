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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <GL/glew.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "msgwindow.h"
#include "render.h"
#include "DrawText.h"
#include "modes.h"
#include "HealthBar.h"
#include "Options.h"
#include "primitives.h"
#include "ScrollingMessages.h"

#define ILL_SENT 0xFFFFFFFF

MsgWindow::MsgWindow(int lines, int width, int x, int y): fNumLines(lines), fX(x), fY(y), fWidth(width), fBackground(0) {
	fActivateDropMessage = false;
	fLines = new(unsigned int[lines]);
	for (int i=0; i<lines; i++) {
		fLines[i] = ILL_SENT;
	}
}
void MsgWindow::Init(void) {
	fMsgWinTransparency = ((float)Options::fgOptions.fMsgWinTransparency)/100.0f;;
	fBackground = HealthBar::Make();
}

void MsgWindow::Add(const char *fmt, ...) {
	va_list vl;
	va_start(vl, fmt);
	char buff[1000];
#ifdef WIN32
	_vsnprintf(buff, sizeof buff, fmt, vl);
#else
	vsnprintf(buff, sizeof buff, fmt, vl);
#endif

	if (gMode.Get() != GameMode::GAME && gMode.Get() != GameMode::CONSTRUCT && gMode.Get() != GameMode::TELEPORT) {
		// Fail safe test if the graphics hasn't been initialized yet.
		printf("Message: %s\n", buff);
		return;
	}
	if (fActivateDropMessage)
		gScrollingMessages.AddMessage(fDropX, fDropY, buff);
	for (char *p=buff;;) {
		int total = strlen(p);
		unsigned int sent = fLines[0]; // Re-use this sentence, if it was allocated
		if (sent == ILL_SENT)
			sent = gDrawFont.vsfl.genSentence();
		// Scroll all lines up one step
		for (int i=0; i<fNumLines-1; i++) {
			fLines[i] = fLines[i+1];
		}
		// Find the number of characters that will fit.
		int count = gDrawFont.vsfl.numberOfCharacters(p, fWidth);
		gDrawFont.vsfl.prepareSentence(sent, std::string(p, count));
		fLines[fNumLines-1] = sent;
		if (count >= total) {
			break;
		}
		p += count;
	}
	gShowMsgWindow = true; // Show the debug window again. TODO: This should be a per-window flag.
}

MsgWindow::~MsgWindow() {
	for (int i=0; i < fNumLines; i++) {
		if (fLines[i] != ILL_SENT)
			gDrawFont.vsfl.deleteSentence(fLines[i]);
	}
	delete []fLines; fLines = 0;
}

void MsgWindow::Render(void) const {
	// Draw a darker background for the text, to make it easier to read. Problem is, this draw function
	// do not use the same projection. Variables with the prefix 'r' are relative window size (0 to 2).
	float totalWinHeight = gViewport[3]; // Measured in pixels
	float totalWinWidth = gViewport[2]; // Measured in pixels
	float msgWinHeight = fNumLines*20.0f; // Assume 20 pixels per character. TODO: naked constant.
	float rBackgroundWidth = fWidth*2/totalWinWidth; // Screen in background coordinates is ranging from -1 to +1.
	float rBackgroundOffset = fY*2/totalWinHeight;
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f - rBackgroundWidth, rBackgroundOffset - 1.0f, 0.0f));
	float yScale = msgWinHeight*2/totalWinHeight;
	// Assume 40 characters, default font size 10, and 12 pixels per character on average.
	// float msgWinWidth = 40.0f * Options::fgOptions.fFontSize / 10.0f * 12.0f;
	float msgWinWidth = fWidth;
	float xScale = msgWinWidth*2/totalWinWidth;
	model = glm::scale(model, glm::vec3(xScale, yScale, 1.0f));
	fBackground->DrawSquare(model, 0.0f, 0.0f, 0.0f, fMsgWinTransparency); // Draw a black square, semi transparent, as a background
	gDrawFont.Enable();
	for (int i=0; i<fNumLines; i++) {
		if (fLines[i] != ILL_SENT) {
			gDrawFont.SetOffset(totalWinWidth-fWidth, float(totalWinHeight - fY - msgWinHeight + i * 20));
			gDrawFont.vsfl.renderSentence(fLines[i]);
		}
	}
	gDrawFont.Disable();
}

void MsgWindow::SetAlternatePosition(int x, int y, bool enable) {
	fActivateDropMessage = enable;
	fDropX = x;
	fDropY = y;
}

MsgWindow gMsgWindow(12, 550, 0, 70);
bool gShowMsgWindow = true;
