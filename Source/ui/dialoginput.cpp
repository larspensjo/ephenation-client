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

#include "dialoginput.h"
#include "dialog.h"
#include "../DrawText.h"

DialogInput::DialogInput(int xoffs, int yoffs, int lines, int width) :
	fWidth(width), fXoffset(xoffs), fYoffset(yoffs), fMaxLines(lines), fCursorPos(0) {
}

void DialogInput::Init(shared_ptr<DrawFont> &font) {
	fFont = font;
}

void DialogInput::Set(const string &str) {
	fString = str;
	fCursorPos = 0;
}

void DialogInput::Render(float alpha, bool active) const {
	glm::vec3 black(-1,-1,-1), white(0,0,0);
	VSFLFont *vsfl = &fFont->vsfl;
	int lineheight = this->EffectiveHeight();
	int line = 0;
	glm::vec3 color = black;
	if (active)
		color = white;
	if (alpha < 1.0f)
		color = glm::vec3(-0.8f,-0.8f,-0.8f); // Override
	float xoffs, yoffs;
	dialog::GetOffset(xoffs, yoffs);
	for (unsigned begin = 0; begin != string::npos && line < fMaxLines; line++) {
		unsigned end = fString.find('\n', begin);
		unsigned space = vsfl->numberOfCharacters(fString.substr(begin), fWidth);
		if (end > begin+space)
			end = begin+space; // Have to break the line here, it doesn't fit
		unsigned len = unsigned(string::npos);
		if (end != string::npos)
			len = end - begin;
		// Don't draw empty lines.
		if (len > 0 && len != string::npos) {
			fFont->SetOffset(fXoffset+xoffs, fYoffset + yoffs + line * lineheight);
			vsfl->renderAndDiscard(fString.substr(begin, end-begin), color, alpha);
			if (fCursorPos >= begin && fCursorPos <= end) {
				// Add cursor if input position was on the current line
				int cursorOffset = vsfl->numberOfPixels(fString.substr(begin, fCursorPos-begin));
				this->DrawCursor(fXoffset+xoffs+cursorOffset, fYoffset+yoffs + line * lineheight, alpha, active);
			}
		}
		begin = end;
	}
}

void DialogInput::DrawCursor(int x, int y, float alpha, bool active) const {
	glm::vec3 black(-1,-1,-1), white(0,0,0);
	glm::vec3 color = black;
	fFont->SetOffset(x, y);
	VSFLFont *vsfl = &fFont->vsfl;
	double now = glfwGetTime();
	int blink = int(now*2) % 2;
	if (blink == 0 && alpha == 1.0f && active)
		color = white; // Change to white now and then if active
	vsfl->renderAndDiscard("|", color, alpha); // Use a bar as a cursor
}

void DialogInput::HandleCharacter(int ch) {
	this->Insert(ch);
}

void DialogInput::HandleKey(int ch) {
	switch(ch) {
	case GLFW_KEY_ENTER:
	case GLFW_KEY_KP_ENTER:
		this->Insert('\n'); // The default would have been '\r'.
		break;
	case GLFW_KEY_BACKSPACE:
		this->Delete();
		break;
	case GLFW_KEY_DEL:
		if (fCursorPos < fString.size()) {
			fCursorPos++;
			this->Delete();
		}
		break;
	case GLFW_KEY_END:
		fCursorPos = fString.size();
		break;
	case GLFW_KEY_HOME:
		fCursorPos = 0;
		break;
	case GLFW_KEY_RIGHT:
		if (fCursorPos < fString.size())
			fCursorPos++;
		break;
	case GLFW_KEY_LEFT:
		if (fCursorPos > 0)
			fCursorPos--;
		break;
	}
}

void DialogInput::Insert(char ch) {
	fString = fString.substr(0, fCursorPos) + ch + fString.substr(fCursorPos);
	fCursorPos++;
}

void DialogInput::Delete(void) {
	if (fCursorPos == 0)
		return;
	if (fCursorPos == fString.size())
		fString = fString.substr(0, fCursorPos-1);
	else
		fString = fString.substr(0, fCursorPos-1) + fString.substr(fCursorPos);
	fCursorPos--;
}

bool DialogInput::Inside(int x, int y) const {
	int height = this->EffectiveHeight();
	return x > fXoffset && x < fWidth+fXoffset && y > fYoffset && y < fMaxLines*height+fYoffset;
}

int DialogInput::EffectiveHeight(void) const {
	VSFLFont *vsfl = &fFont->vsfl;
	return vsfl->height() * 0.7; // The reported height seems to be excessive.
}
