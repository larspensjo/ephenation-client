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
#include <GL/glfw.h>

#include <stdio.h>
#include <string.h>

#include "../glm/glm.hpp"
#include "../glm/gtc/matrix_transform.hpp"
#include "../primitives.h"
#include "dialog.h"
#include "maindialog.h"
#include "../textures.h"
#include "../DrawTexture.h"
#include "../DrawText.h"

dialog::dialog() {
	fBodyFont.reset(new DrawFont());
	fBodyFont->Init("textures/gabriola18");
	fMenuFont.reset(new DrawFont());
	fMenuFont->Init("textures/gabriola36");
	fTitleFont.reset(new DrawFont());
	fTitleFont->Init("textures/gabriola48");

	// printf("body %d, menu %d, title %d\n", fBodyFont->vsfl.height(), fMenuFont->vsfl.height(), fTitleFont->vsfl.height());
}

void dialog::Draw(DrawTexture *drawTexture, float alpha) {
	// The bitmap is 640x460
	float xscale = 2.0f*640.0f/gViewport[2];
	float yscale = 2.0f*480.0f/gViewport[3];
	static const glm::mat4 ident(1.0f); // No need to create a new one every time.

	// Scale the quad to 640x480
	glm::mat4 model = glm::scale(ident, glm::vec3(xscale, yscale, 1.0f));

	// The quad defined from 0 to 1, center it on screen.
	model = glm::translate(model, glm::vec3(-0.5f, -0.5f, 0.0f));
	glBindTexture(GL_TEXTURE_2D, GameTexture::DialogBackground);
	glEnable(GL_BLEND);
	drawTexture->Draw(ident, model, alpha);
}

void dialog::GetOffset(float &x, float &y) {
	// 50 pixel offset is a good offset into the dialog
	x = (gViewport[2] - 640.0f)/2.0f + 50.0f;
	y = (gViewport[3] - 480.0f)/2.0f;
}

void dialog::GetMousePosition(float &x, float &y) const {
	float ox, oy;
	GetOffset(ox, oy);
	int ix, iy;
	glfwGetMousePos(&ix, &iy);
	x = float(ix)-ox;
	y = float(iy)-oy;
}

DialogText::DialogText() : fIdx(0xFFFFFFFF) {
}

DialogText::~DialogText() {
	VSFLFont *vsfl = &fFont->vsfl;
	if (fIdx != 0xFFFFFFFF)
		vsfl->deleteSentence(fIdx);
}

void DialogText::Init(shared_ptr<DrawFont> &font, const std::string &text, int xoffs, int yoffs) {
	fFont = font;
	VSFLFont *vsfl = &fFont->vsfl;
	fIdx = vsfl->genSentence();
	vsfl->prepareSentence(fIdx, text);
	fWidth = vsfl->numberOfPixels(text);
	fXoffset = xoffs;
	fYoffset = yoffs;
}

void DialogText::Draw(glm::vec3 color, float alpha) const {
	float xoffs, yoffs;
	dialog::GetOffset(xoffs, yoffs);
	// There are three offsets:
	// 1. Dialog offset to window
	// 2. Offsets into the dialog to get a border (included in result from GetOffset)
	// 3. Specific offset for this text.
	fFont->SetOffset(xoffs+fXoffset, yoffs+fYoffset);
	VSFLFont *vsfl = &fFont->vsfl;
	if (alpha < 1.0f)
		color = glm::vec3(-0.8f,-0.8f,-0.8f); // Override
	vsfl->renderSentence(fIdx, color, alpha);
}

bool DialogText::Inside(int x, int y) const {
	VSFLFont *vsfl = &fFont->vsfl;
	int height = vsfl->height();
	return x > fXoffset && x < fWidth+fXoffset && y > fYoffset && y < height+fYoffset;
}

void dialog::Push(dialog *d) {
	fStack.push_back(unique_ptr<dialog>(d));
}

void dialog::Pop(void) {
	if (fStack.empty())
		return;
	fStack.pop_back();
}

bool dialog::DispatchClick(void) {
	if (fStack.empty()) // Fail safe test
		return false;
	unsigned size = fStack.size();
	fStack[size-1]->Click();
	return !fStack.empty();
}

void dialog::DispatchDraw(DrawTexture *drawTexture, float alpha, dialog *newDialog) {
	// If there is a specific new dialog to use, push it on the stack.
	if (newDialog != 0)
		dialog::Push(newDialog);
	if (fStack.empty())
		dialog::Push(new MainDialog);
	unsigned size = fStack.size();
	fStack[size-1]->Draw(drawTexture, alpha);
}

bool dialog::DispatchEscape(void) {
	if (fStack.empty()) // Fail safe test
		return false;
	unsigned size = fStack.size();
	fStack[size-1]->Escape();
	return !fStack.empty();
}

void dialog::DispatchKey(int ch) {
	if (fStack.empty()) // Fail safe test
		return;
	unsigned size = fStack.size();
	fStack[size-1]->KeyPress(ch);
}

void dialog::DispatchChar(int ch) {
	if (fStack.empty()) // Fail safe test
		return;
	unsigned size = fStack.size();
	fStack[size-1]->Character(ch);
}

std::vector<unique_ptr<dialog> > dialog::fStack;
