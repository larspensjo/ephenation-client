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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <memory>
#include <GL/glew.h>
#include <GL/glfw.h>
#include <math.h>
#include <Rocket/Debugger.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#include "primitives.h"
#include "player.h"
#include "gamedialog.h"
#include "connection.h"
#include "client_prot.h"
#include "msgwindow.h"
#include "parse.h"
#include "textures.h"
#include "modes.h"
#include "monsters.h"
#include "otherplayers.h"
#include "shaders/ChunkShader.h"
#include "shaders/ChunkShaderPicking.h"
#include "shaders/DeferredLighting.h"
#include "shaders/TranspShader.h"
#include "HealthBar.h"
#include "BuildingBlocks.h"
#include "DrawTexture.h"
#include "SoundControl.h"
#include "Options.h"
#include "Inventory.h"
#include "Map.h"
#include "ScrollingMessages.h"
#include "Teleport.h"
#include "ui/base.h"
#include "ui/RocketGui.h"
#include "ui/Error.h"
#include "ui/factory.h"
#include "uniformbuffer.h"
#include "billboard.h"
#include "rendercontrol.h"
#include "timemeasure.h"
#include "worsttime.h"
#include "ChunkProcess.h"
#include "entitycomponentsystem.h"

using namespace Controller;
using View::SoundControl;
using View::gSoundControl;
using View::gMsgWindow;

using std::string;

const float defaultRenderViewAngle = 60.0f;
float renderViewAngle = defaultRenderViewAngle;

// The time stamp when the player clicked on a magical portal.
static double sStartZoom;

static std::vector<unsigned>::iterator sTextureIterator;
static bool sShowAlternateBitmap = false;

static float mouseSens = -5.0f;
gameDialog Controller::gGameDialog; // For now, there will only be one

// If there are any strings in these parameters, show them once in a popup dialog.
string sgPopup, sgPopupTitle = "Ephenation";

#define NELEM(x) (sizeof x / sizeof x[0])

gameDialog::gameDialog() {
	fMovingFwd = false;
	fMovingBwd = false;
	fMovingLeft = false;
	fMovingRight = false;
	fUsingTorch = false;
	fUnderWater = false;
	fMapWidth = 600;

	fEnterDebugText = false;
	fDrawMap = false;
	fShowInventory = false;
	fShowWeapon = true;
	fShader = 0;
	fBuildingBlocks = 0;
	fHealthBar = 0;
	fDrawTexture = 0;
	fCurrentEffect = EFFECT_NONE;
	fCalibrationMode = CALIB_NONE;
	fCurrentRocketContextInput = 0;
	fFPS_Element = 0;
	fPlayerStatsOneLiner_Element = 0;
	fInputLine = 0;
	// Use a value that will not match.
	fRequestActivatorChunk.x = fRequestActivatorChunk.y = fRequestActivatorChunk.z = std::numeric_limits<int>::max();
	fRequestActivatorX = fRequestActivatorY = fRequestActivatorZ = std::numeric_limits<int>::max();
}

gameDialog::~gameDialog() {
	fShader = 0; // Don't delete this singleton
	fBuildingBlocks = 0; // Singleton
	fHealthBar = 0; // Singleton
	if (fPlayerStatsOneLiner_Element)
		fPlayerStatsOneLiner_Element->RemoveReference();
	if (fFPS_Element)
		fFPS_Element->RemoveReference();
	if (fInputLine)
		fInputLine->RemoveReference();
}

// In build mode, find the block at the given screen position. Return the chunk and the
// data about it to the pointers.
View::Chunk *gameDialog::FindSelectedSurface(int x, int y, ChunkOffsetCoord *coc, int *surfaceDir) {
	gChunkShaderPicking.EnableProgram();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Use black sky for picking
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	DrawLandscape(0, DL_Picking);
	gChunkShaderPicking.DisableProgram();

	unsigned char pixel[4];
	glReadPixels(x,gViewport[3] - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel[0]);

	if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0)
		return 0; // Click on the sky, which is drawn black. No block will have this value (facing is 1-6).

	PickingData coding;
	coding.rgb[0] = pixel[0]; coding.rgb[1] = pixel[1]; coding.rgb[2] = pixel[2]; // Compensate for transformation in shader.
#if 0
	gMsgWindow.Add("Pick (%d,%d,%d) facing %d, d(%d,%d,%d)", coding.bitmap.x, coding.bitmap.y, coding.bitmap.z,
	               coding.bitmap.facing, coding.bitmap.dx-1, coding.bitmap.dy-1, coding.bitmap.dz-1);
	gMsgWindow.Add("RGB: %d,%d,%d", coding.rgb[0], coding.rgb[1], coding.rgb[2]);
#endif

	ChunkCoord cc;

	Model::gPlayer.GetChunkCoord(&cc);
	cc.x += coding.bitmap.dx-1;
	cc.y += coding.bitmap.dy-1;
	cc.z += coding.bitmap.dz-1;
	View::Chunk *cp = ChunkFind(&cc, false);
	coc->x = coding.bitmap.x; coc->y = coding.bitmap.y; coc->z = coding.bitmap.z;

	if (surfaceDir)
		*surfaceDir = coding.bitmap.facing;
	return cp;
}

void gameDialog::AttachBlockToSurface(int row, int col) {
	ChunkOffsetCoord coc;
	int surfaceDir = 0;
	View::Chunk *cp = FindSelectedSurface(row, col, &coc, &surfaceDir);
	if (cp == 0)
		return;
	// Tricky thing is that the 'coc' is not the place we want to place the new block.
	// It is the surface of a current existing block. Depending on the way
	// that the surface face, we get the actual block address.
	int x=coc.x, y=coc.y, z=coc.z;
	switch(surfaceDir) {
	case 0:
		return; // Not valid
	case CH_TopFace:
		z++;
		break;
	case CH_BottomFace:
		z--;
		break;
	case CH_LeftFace:
		x--;
		break;
	case CH_RightFace:
		x++;
		break;
	case CH_FrontFace:
		y--;
		break;
	case CH_BackFace:
		y++;
		break;
	}

	// It may be that the new block is outside chunk 'cp'.
	ChunkCoord cc = cp->cc;
	if (x < 0) {
		cc.x--;
		x += CHUNK_SIZE;
	}
	if (y < 0) {
		cc.y--;
		y += CHUNK_SIZE;
	}
	if (z < 0) {
		cc.z--;
		z += CHUNK_SIZE;
	}
	if (x == CHUNK_SIZE) {
		cc.x++;
		x -= CHUNK_SIZE;
	}
	if (y == CHUNK_SIZE) {
		cc.y++;
		y -= CHUNK_SIZE;
	}
	if (z == CHUNK_SIZE) {
		cc.z++;
		z -= CHUNK_SIZE;
	}
	// It may be that cc.z is now above the roof of the world. This will be handled by the server, that
	// will not allow blocks to be added. That way, the client doesn't have to know how high the world is.
	// gDebugWindow.Add("AttachBlockToSurface: chunk (%d,%d,%d) at (%d,%d,%d)\n", cc.x, cc.y, cc.z, x, y, z);

	unsigned char b[19];
	b[0] = sizeof b;
	b[1] = 0;
	b[2] = CMD_BLOCK_UPDATE;
	EncodeUint32(b+3, cc.x);
	EncodeUint32(b+7, cc.y);
	EncodeUint32(b+11, cc.z);
	b[15] = x;
	b[16] = y;
	b[17] = z;
	b[18] = fBuildingBlocks->CurrentBlockType();
	SendMsg(b, sizeof b);
	if (fBuildingBlocks->CurrentBlockType() == BT_Text) {
		fRequestActivatorChunk = cc;
		fRequestActivatorX = x;
		fRequestActivatorY = y;
		fRequestActivatorZ = z;
	}

    ChunkCoord c;
    c.x = cp->cc.x;
    c.y = cp->cc.y;
    c.z = cp->cc.z;
    fUndoOperator.addOperation(new Operation(CMD_BLOCK_UPDATE,
                                              BlockLocation(x, y, z),
                                              c,
                                              fBuildingBlocks->CurrentBlockType()
                                             )
                               );
}

void gameDialog::CreateActivatorMessage(int dx, int dy, int dz, const ChunkCoord &cc) {
	if (dx == fRequestActivatorX && dy == fRequestActivatorY && dz == fRequestActivatorZ &&
			cc.x == fRequestActivatorChunk.x &&
			cc.y == fRequestActivatorChunk.y &&
			cc.z == fRequestActivatorChunk.z)
	{
		fCurrentRocketContextInput = fMainUserInterface.GetRocketContext();
		gDialogFactory.Make(fCurrentRocketContextInput, "activator.rml");
		fRequestActivatorX = -1; // Ensure it will not match by accident
	}
}

// TODO: It is a sign of bad design when there is both a getter and a setter for the same data.
void gameDialog::GetActivator(int &dx, int &dy, int &dz, ChunkCoord &cc) {
	dx = fRequestActivatorX;
	dy = fRequestActivatorY;
	dz = fRequestActivatorZ;
	cc = fRequestActivatorChunk;
}

// The player clicked on an object. Depending on mode, we either
// allow for rebuilding the environment, or attack monsters.
// TODO: For now, can only use TAB to select objects
void gameDialog::ClickOnObject(int x, int y) {
	this->ClearSelection();
}

// The player clicked one something. Depending on mode, we either
// allow for rebuilding the environment, or attack monsters.
void gameDialog::ClickOnBlock(int x, int y) {
	static int prevX = -1, prevY = -1;
	if (prevX == x && prevY == y)
		return; // Ignore multiple clicks on the same block
	prevX = x; prevY = y;
	ChunkOffsetCoord coc;
	View::Chunk *cp = this->FindSelectedSurface(x, y, &coc, 0);
	if (cp == 0)
		return;
	// Use the block that the surface belongs to
	// gDebugWindow.Add("Pick: object (%d,%d,%d) at (%d,%d,%d)\n", cp->cc.x, cp->cc.y, cp->cc.z, coc.x, coc.y, coc.z);

	unsigned char b[18];
	b[0] = sizeof b;
	b[1] = 0;
	b[2] = CMD_HIT_BLOCK;
	EncodeUint32(b+3, cp->cc.x);
	EncodeUint32(b+7, cp->cc.y);
	EncodeUint32(b+11, cp->cc.z);
	b[15] = coc.x; // TODO: conversion from unsigned short to unsigned char.
	b[16] = coc.y;
	b[17] = coc.z;
	SendMsg(b, sizeof b);
	// printf("Click on block %d, %d, %d\n", coc.x, coc.y , coc.z);

    ChunkCoord ch;
    ch.x = cp->cc.x;
    ch.y = cp->cc.y;
    ch.z = cp->cc.z;

    fUndoOperator.addOperation(new Operation(CMD_HIT_BLOCK,
                                              BlockLocation(coc.x, coc.y, coc.z),
                                              ch,
                                              cp->GetBlock(coc.x, coc.y, coc.z) // add this type
                                             )
                               );
}

static bool sTurning = false;
int xStartTurn = 0, yStartTurn = 0;
float angleHorStartTurn = 0.0f, angleVertStartTurn;

static void handleMouseActiveMotion(int x, int y) {
	Controller::gGameDialog.handleMouseActiveMotion(x, y);
}

void gameDialog::handleMouseActiveMotion(int x, int y) {
	if (fCurrentRocketContextInput) {
		fCurrentRocketContextInput->ProcessMouseMove(x, y, 0);
		return;
	}
	if (fShowInventory) {
		sTurning = false;
	} else if (sTurning) {
		int deltax = xStartTurn - x;
		int deltay = yStartTurn - y;
		// printf("Turning dx %d dy %d\n", deltax, deltay);
		_angleHor = angleHorStartTurn + deltax/mouseSens;
		if (_angleHor > 360.0) _angleHor -= 360.0f;
		if (_angleHor < 0.0f) _angleHor += 360.0f;
		_angleVert = angleVertStartTurn + deltay/mouseSens;
		if (_angleVert > 90.0f) _angleVert = 90.0f;
		if (_angleVert < -90.0f) _angleVert = -90.0f;
		unsigned char b[7];
		unsigned short angleHorRad = (unsigned short)(_angleHor / 360.0f * 2.0f * M_PI * 100);
		signed short angleVertRad = (unsigned short)(_angleVert / 360.0f * 2.0f * M_PI * 100);
		// printf("Dir (%f, %f), (%d, %d)\n", _angleHor, _angleVert, angleHorRad, angleVertRad);
		b[0] = sizeof b;
		b[1] = 0;
		b[2] = CMD_SET_DIR;
		EncodeUint16(b+3, angleHorRad);
		EncodeUint16(b+5, angleVertRad);
		// DumpBytes(b, sizeof b);
		SendMsg(b, sizeof b);
		Model::gPlayer.fAngleHor = _angleHor; // Update player data with current looking direction
		Model::gPlayer.fAngleVert = _angleVert;
	}
}

static void dialogHandleMouse(int button, int action) {
	gGameDialog.handleMouse(button, action);
}

void gameDialog::handleMouse(int button, int action) {
	if (fCurrentRocketContextInput) {
		int idx = 2;
		switch (button) {
		case GLFW_MOUSE_BUTTON_RIGHT:
			idx = 1;
			break;
		case GLFW_MOUSE_BUTTON_LEFT:
			idx = 0;
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			idx = 2;
			break;
		}
		if (action == GLFW_PRESS)
			fCurrentRocketContextInput->ProcessMouseButtonDown(idx, 0);
		else
			fCurrentRocketContextInput->ProcessMouseButtonUp(idx, 0);
		return;
	}
	int x, y;
	glfwGetMousePos(&x, &y);
	if (fShowInventory) {
		// Override the usual mouse handling
		bool close = gInventory->HandleMouseClick(button, action, x, y);
		if (close) {
			fShowInventory = false;
			gMsgWindow.SetAlternatePosition(0,0,false);
		}
		return;
	}
	// printf("Mouse function: button %d, state %d, at (%d,%d)\n", button, action, x, y);
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		if (action == GLFW_PRESS) {
			xStartTurn = x;
			yStartTurn = y;
			angleHorStartTurn = _angleHor;
			angleVertStartTurn = _angleVert;
			sTurning = true;
		} else {
			sTurning = false;
		}
	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && gMode.Get() == GameMode::GAME) {
		ClickOnObject(x, y);
	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && gMode.Get() == GameMode::TELEPORT) {
		int x, y;
		glfwGetMousePos(&x, &y);
		const ChunkCoord *cc = TeleportClick(fHealthBar, _angleHor, renderViewAngle, x, y, true);
		if (cc != 0) {
			// printf("TP to chunk %d,%d,%d\n", cc->x, cc->y, cc->z);
			unsigned char b[6];
			b[0] = 6;
			b[1] = 0;
			b[2] = CMD_TELEPORT;
			b[3] = cc->x & 0xFF;
			b[4] = cc->y & 0xFF;
			b[5] = cc->z & 0xFF;
			SendMsg(b, sizeof b);
			// this->RequestEffect(EFFECT_ZOOM1); // TODO: Doesn't look good.
		}
	}
}

void handleResize(int w, int h) {
	// printf("::handleResize: %dx%d", w, h);
	if (w == 0 || h == 0)
		return; // This will happen when window is iconified.
	gGameDialog.handleResize(w, h);
}

glm::mat4 gProjectionMatrix;
void gameDialog::handleResize(int w, int h) {
	float aspectRatio = (float)w / (float)h;
	// In full screen mode, the window is stretched to match the desktop mode.
	if (gOptions.fFullScreen)
		aspectRatio = gDesktopAspectRatio;
	gProjectionMatrix  = glm::perspective(renderViewAngle, aspectRatio, 0.01f, maxRenderDistance);  // Create our perspective projection matrix
	glViewport(0, 0, w, h);
	gViewport = glm::vec4(0.0f, 0.0f, (float)w, (float)h );
	fRenderControl.Resize(w, h);
	fMainUserInterface.Resize(w, h);
	Options::sfSave.fWindowWidth = gViewport[2]; // This will override any option dialog changes.
	Options::sfSave.fWindowHeight = gViewport[3];
}

static void handleCharacter(int character, int action) {
	// printf("handleCharacter %d, action %d\n", character, action);
	gGameDialog.HandleCharacter(character, action);
}

static int rocketKeyModifiers = 0;

void gameDialog::HandleCharacter(int key, int action) {
	if (fCurrentRocketContextInput) {
		if (action == GLFW_PRESS && (rocketKeyModifiers & (Rocket::Core::Input::KM_CTRL | Rocket::Core::Input::KM_ALT)) == 0) {
			fCurrentRocketContextInput->ProcessTextInput(key);
		}
		return;
	}
}

static void handleKeypress(int key, int action) {
	int modifier = 0;
	switch(key) {
	case GLFW_KEY_LSHIFT:
	case GLFW_KEY_RSHIFT:
		modifier = Rocket::Core::Input::KM_SHIFT;
		break;
	case GLFW_KEY_LCTRL:
	case GLFW_KEY_RCTRL:
		modifier = Rocket::Core::Input::KM_CTRL;
		break;
	case GLFW_KEY_CAPS_LOCK:
		modifier = Rocket::Core::Input::KM_CAPSLOCK;
		break;
	case GLFW_KEY_KP_NUM_LOCK:
		modifier = Rocket::Core::Input::KM_NUMLOCK;
		break;
	case GLFW_KEY_LALT:
		modifier = Rocket::Core::Input::KM_ALT;
		break;
	case GLFW_KEY_SCROLL_LOCK:
		modifier = Rocket::Core::Input::KM_SCROLLLOCK;
		break;
	}

	if (action == GLFW_PRESS) {
		rocketKeyModifiers |= modifier;
		gGameDialog.HandleKeyPress(key);
	} else {
		rocketKeyModifiers &= ~modifier;
		gGameDialog.HandleKeyRelease(key);
	}
}

string prevCommand;

void gameDialog::HandleKeyPress(int key) {
	if (key == GLFW_KEY_F12) {
		// Special override, whether activating input to a Rocket context or not.
		if (fCurrentRocketContextInput)
			fCurrentRocketContextInput = 0;
		else
			fCurrentRocketContextInput = fMainUserInterface.GetRocketContext();
		return;
	}

	if (key == GLFW_KEY_F11) {
        this->SaveScreen();
        return;
	}

	if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) && fEnterDebugText && fCurrentRocketContextInput) {
		fCurrentRocketContextInput = 0; // Override. Use ENTER key to submit the input text.
	}

	if (key == GLFW_KEY_ESC && fEnterDebugText && fCurrentRocketContextInput) {
		// Override, cancel input of text.
		fCurrentRocketContextInput = 0;
		fInputLine->SetAttribute("value", "");
		fInputLine->Blur();
		fEnterDebugText = false;
		return;
	}

	if (key == GLFW_KEY_KP_0 && gDebugOpenGL) {
		gToggleTesting = !gToggleTesting;
		Rocket::Debugger::SetVisible(gToggleTesting);
		return;
	}

	if (fCurrentRocketContextInput) {
		if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)
			BaseDialog::DispatchDefault();
		else if (key == GLFW_KEY_ESC)
			BaseDialog::DispatchCancel();
		else
			fCurrentRocketContextInput->ProcessKeyDown(RocketGui::KeyMap(key), rocketKeyModifiers);
		return;
	}

	if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F11) {
		gInventory->UseObjectFunctionKey(key);
		return;
	}
	if (fEnterDebugText) {
		glfwDisable(GLFW_KEY_REPEAT);
		Rocket::Core::String def = "";
		Rocket::Core::String str = fInputLine->GetAttribute("value", def); // This call generates 100 lines of warnings
		size_t len = str.Length();
		const char *begin = str.CString();
		const char *end = strchr(begin, ' ');
		if (end == 0)
			prevCommand = begin;
		else
			prevCommand = string(begin, end-begin+1); // Including the space character
		unsigned char cmd[4];
		cmd[0] = len+4;
		cmd[1] = 0;
		cmd[2] = CMD_DEBUG;
		cmd[3] = '/';
		SendMsg(cmd, 4);
		SendMsg((const unsigned char *)begin, len);
		fEnterDebugText = false;
		fInputLine->SetAttribute("value", "");
		fInputLine->Blur();
		return;
	}

	int x, y;
	glfwGetMousePos(&x, &y);
	switch (key) {
	case GLFW_KEY_ESC:
		if (fDrawMap)
			fDrawMap = false;
		else if (fShowInventory) {
			fShowInventory = false;
			gMsgWindow.SetAlternatePosition(0,0,false);
		} else {
			fCurrentRocketContextInput = fMainUserInterface.GetRocketContext();
			gDialogFactory.Make(fCurrentRocketContextInput, "topleveldialog.rml");
		}
		break;
	case 'C':
		if (gMode.Get() == GameMode::CONSTRUCT) { // Toggle construction mode
			gMode.Set(GameMode::GAME);
			gShowFramework = false;
		} else {
			gMode.Set(GameMode::CONSTRUCT);
		}
		if (Model::gPlayer.KnownPosition()) {
			ChunkCoord player_cc;
			// Force the current chunk to be redrawn, adapted for construction mode
			Model::gPlayer.GetChunkCoord(&player_cc);
			for (int dx=-1; dx<2; dx++) for (int dy=-1; dy<2; dy++) for (int dz=-1; dz < 2; dz++) {
						ChunkCoord cc = player_cc;
						cc.x += dx; cc.y += dy; cc.z += dz;
						View::Chunk *cp = ChunkFind(&cc, true);
						cp->SetDirty(true);
					}
		}
		break;
	case GLFW_KEY_TAB:
		if (fDrawMap && gDebugOpenGL) {
			// A debug feature to iterate through various interesting bitmaps
			if (!sShowAlternateBitmap) {
				sShowAlternateBitmap = true;
				sTextureIterator = gDebugTextures.begin();
			} else
				++sTextureIterator;
			if (sTextureIterator == gDebugTextures.end()) {
				sShowAlternateBitmap = false;
			}
			break;
		}
		// Find next monster after the selected one.
		fSelectedObject = Model::gMonsters->GetNext(fSelectedObject);
		break;
	case GLFW_KEY_LALT: // ALT key
		if (gMode.Get() == GameMode::CONSTRUCT) {
			gShowFramework = true; // Only when in construction mode
		} else if (gMode.Get() == GameMode::GAME && Model::gPlayer.fAdmin > 0) {
			gMode.Set(GameMode::TELEPORT);
			gAdminTP = true;
		}
		break;
	case '1': // Autoattack
		// Find next monster after the selected one. Works fine even if nothing is currently selected
		if (!fSelectedObject)
			fSelectedObject = Model::gMonsters->GetNext(fSelectedObject);
		if (fSelectedObject) { // Initiate attack on a monster, if any.
			unsigned char b[] = { 7, 0, CMD_ATTACK_MONSTER, 0, 0, 0, 0 };
			EncodeUint32(b+3, fSelectedObject->GetId());
			SendMsg(b, sizeof b);
		}
		break;
	case '2': { // Heal self
		unsigned char b[] = { 0x04, 0x00, CMD_PLAYER_ACTION, UserActionHeal };
		SendMsg(b, sizeof b);
		break;
	}
	case '3': { // Instant extra attack
		unsigned char b[] = { 0x04, 0x00, CMD_PLAYER_ACTION, UserActionCombAttack };
		SendMsg(b, sizeof b);
		break;
	}
	case 'I': {
		// Toggle inventory screen
		fShowInventory = !fShowInventory;
		if (!fShowInventory)
			gMsgWindow.SetAlternatePosition(0,0,false);
		break;
	}
	case 'T':
		dumpGraphicsMemoryStats();
		break;
	case GLFW_KEY_KP_ENTER:
	case GLFW_KEY_ENTER: // ENTER key
		fEnterDebugText = true;
		glfwEnable(GLFW_KEY_REPEAT);
		fInputLine->SetAttribute("value", prevCommand.c_str());
		fInputLine->Focus();
		fCurrentRocketContextInput = fMainUserInterface.GetRocketContext();
		fCurrentRocketContextInput->ProcessKeyDown(Rocket::Core::Input::KI_END, Rocket::Core::Input::KM_SHIFT);
		break;
	case GLFW_KEY_SPACE: { // space key
		unsigned char b[] = { 0x03, 0x00, CMD_JUMP };
		SendMsg(b, sizeof b);
		gSoundControl.RequestSound(SoundControl::SPlayerJump);
		break;
	}
	case 'W':
	case GLFW_KEY_UP:
		if (!fMovingFwd) {
			fMovingFwd = true;
			unsigned char b[] = { 0x03, 0x00, 10 };
			SendMsg(b, sizeof b);
			// printf("Start fwd\n");
		}
		break;
	case 'A':
	case GLFW_KEY_LEFT:
		if (!fMovingLeft) {
			fMovingLeft = true;
			unsigned char b[] = { 0x03, 0x00, 14 };
			SendMsg(b, sizeof b);
			// printf("Start left\n");
		}
		break;
	case 'S':
	case GLFW_KEY_DOWN:
		if (!fMovingBwd) {
			fMovingBwd = true;
			unsigned char b[] = { 0x03, 0x00, 12 };
			SendMsg(b, sizeof b);
			// printf("Start bwd\n");
		}
		break;
	case 'D':
	case GLFW_KEY_RIGHT:
		if (!fMovingRight) {
			fMovingRight = true;
			unsigned char b[] = { 0x03, 0x00, 16 };
			SendMsg(b, sizeof b);
			// printf("Start right\n");
		}
		break;
	case GLFW_KEY_INSERT: // Insert, closer to delete on a full keyboard
	case 'B':
		if (gMode.Get() == GameMode::CONSTRUCT) {
			this->AttachBlockToSurface(x, y);
		}

		break;
	case GLFW_KEY_DEL: // Delete
	case 'V': // Closer to B when building
		if (gMode.Get() == GameMode::CONSTRUCT) {
			ClickOnBlock(x, y);
		}
		break;
    case 'U':
            if (gMode.Get() == GameMode::CONSTRUCT) {
                fUndoOperator.undoOperation();
            }
		break;
       case 'R':
            if (gMode.Get() == GameMode::CONSTRUCT) {
                fUndoOperator.redoOperation();
            }
		break;
	case GLFW_KEY_KP_SUBTRACT:
		switch(fCalibrationMode) {
		case CALIB_AMBIENT:
			gOptions.fAmbientLight -= 1;
			gMsgWindow.Add("Ambient light: %f", gOptions.fAmbientLight/100.0);
			break;
		case CALIB_EXPOSURE:
			gOptions.fExposure /= 1.1f;
			gMsgWindow.Add("Exposure: %f", gOptions.fExposure);
			break;
		case CALIB_WHITE_POINT:
			gOptions.fWhitePoint /= 1.1f;
			gMsgWindow.Add("White point: %f", gOptions.fWhitePoint);
			break;
		case CALIB_NONE:
			break;
		}
		break;
	case '-': {
		maxRenderDistance -= 5.0;
		if (maxRenderDistance < 5.0f && gDebugOpenGL)
			maxRenderDistance = 5.0f;
		if (maxRenderDistance < 40.0f && !gDebugOpenGL)
			maxRenderDistance = 40.0f;
		handleResize(gViewport[2], gViewport[3]);
		gMsgWindow.Add("Viewing distance: %f m", maxRenderDistance/2);
		break;
	}
	case GLFW_KEY_KP_ADD:
		switch(fCalibrationMode) {
		case CALIB_AMBIENT:
			gOptions.fAmbientLight += 1;
			gMsgWindow.Add("Ambient light: %f", gOptions.fAmbientLight/100.0);
			break;
		case CALIB_EXPOSURE:
			gOptions.fExposure *= 1.1f;
			gMsgWindow.Add("Exposure: %f", gOptions.fExposure);
			break;
		case CALIB_WHITE_POINT:
			gOptions.fWhitePoint *= 1.1f;
			gMsgWindow.Add("White point: %f", gOptions.fWhitePoint);
			break;
		case CALIB_NONE:
			break;
		}
		break;
	case '+': {
		// #255 long distances are not handled very well by neither server nor client
		maxRenderDistance += 5.0;
		if (maxRenderDistance > MAXRENDERDISTANCE)
			maxRenderDistance = MAXRENDERDISTANCE;
		handleResize(gViewport[2], gViewport[3]);
		gMsgWindow.Add("Viewing distance: %f m", maxRenderDistance/2);
		break;
	}
	case 'M': {
		fDrawMap = !fDrawMap;
		break;
	}
	case '\'': {
		if (Model::gPlayer.fAdmin > 0)
			fUsingTorch = !fUsingTorch;
	}
	default:
		gMsgWindow.Add("Unknown key '%d'", key);
		break;
	}
	gGameDialog.UpdateRunningStatus(false);
}

void gameDialog::HandleKeyRelease(int key) {
	if (fCurrentRocketContextInput) {
		if (key != 0)
			fCurrentRocketContextInput->ProcessKeyUp(RocketGui::KeyMap(key), rocketKeyModifiers);
		return;
	}

	switch (key) {
	case 'W':
	case 283:
		if (fMovingFwd) {
			fMovingFwd = false;
			unsigned char b[] = { 0x03, 0x00, 11 };
			SendMsg(b, sizeof b);
			// printf("stop fwd\n");
		}
		break;
	case 'A':
	case 285:
		if (fMovingLeft) {
			fMovingLeft = false;
			unsigned char b[] = { 0x03, 0x00, 15 };
			SendMsg(b, sizeof b);
			// printf("stop left\n");
		}
		break;
	case 'S':
	case 284:
		if (fMovingBwd) {
			fMovingBwd = false;
			unsigned char b[] = { 0x03, 0x00, 13 };
			SendMsg(b, sizeof b);
			// printf("stop bwd\n");
		}
		break;
	case 'D':
	case 286:
		if (fMovingRight) {
			fMovingRight = false;
			unsigned char b[] = { 0x03, 0x00, 17 };
			SendMsg(b, sizeof b);
			// printf("stop right\n");
		}
		break;
	case GLFW_KEY_LALT: // ALT key
		if (gMode.Get() == GameMode::CONSTRUCT)
			gShowFramework = false; // Only when in construction mode
		else if (gMode.Get() == GameMode::TELEPORT && Model::gPlayer.fAdmin > 0 && gAdminTP) {
			gMode.Set(GameMode::GAME);
			gAdminTP = false;
		}
		break;
	}
	gGameDialog.UpdateRunningStatus(false);
}

void gameDialog::ClearSelection(void) {
	fSelectedObject.reset();
}

// This function is called every time the player gets aggro
void gameDialog::AggroFrom(boost::shared_ptr<const Model::Object> o) {
	// It may be called from more than one monster. For now, use the information to set a selection for the first monster.
	if (!fSelectedObject)
		fSelectedObject = o;
}

static void revive(void) {
	char msg[] = "   /revive";
	msg[0] = 10;
	msg[1] = 0;
	msg[2] = CMD_DEBUG;
	SendMsg((unsigned char *)msg, (sizeof msg) - 1);
}

glm::mat4 gViewMatrix; // Store the view matrix

void gameDialog::render(bool hideGUI) {
	if (fCurrentRocketContextInput == 0 && gMode.Get() == GameMode::LOGIN) {
		// Login mode, get the login dialog.
		fCurrentRocketContextInput = fMainUserInterface.GetRocketContext();
		gDialogFactory.Make(fCurrentRocketContextInput, "login.rml");
	}

	static bool first = true; // Only true first time function is called
	// Clear list of special effects. It will be added again automatically every frame
	gShadows.Clear();
	gFogs.Clear();

	// Can't select objects that are dead
	if (fSelectedObject && fSelectedObject->IsDead())
		ClearSelection();
	glm::vec3 playerOffset = Model::gPlayer.GetOffsetToChunk();

	gDrawnQuads = 0;
	gNumDraw = 0;

	gUniformBuffer.Update();

	if (first) {
		gBillboard.InitializeTextures(fShader);
	}

	fRenderControl.Draw(fUnderWater, fSelectedObject, fDrawMap && !gDebugOpenGL, fMapWidth, (hideGUI && fCurrentRocketContextInput == 0) ? 0 : &fMainUserInterface);

	//=========================================================================
	// Various effects drawn after the deferred shader
	//=========================================================================

	if (!fDrawMap) {
		if (fShowWeapon && gMode.Get() != GameMode::CONSTRUCT && !fShowInventory && !fRenderControl.ThirdPersonView() && !hideGUI)
			this->DrawWeapon();
		static bool wasDead = false;
		if (Model::gPlayer.IsDead() && !wasDead) {
			wasDead = true;
			sgPopupTitle = "Oops";
			sgPopup = "You are dead.\n\nYou will be revived, and transported back to your starting place.\n\nThe place can be changed with a scroll of resurrection point.";
			fCurrentRocketContextInput = fMainUserInterface.GetRocketContext();
			gDialogFactory.Make(fCurrentRocketContextInput, "messagedialog.rml", revive);
			this->ClearForDialog();
		} else if (fShowInventory)
			gInventory->DrawInventory(fDrawTexture);
		else if (sgPopup.length() > 0) {
			// There are some messages that shall be shown in a popup dialog.
			fCurrentRocketContextInput = fMainUserInterface.GetRocketContext();
			gDialogFactory.Make(fCurrentRocketContextInput, "messagedialog.rml");
			this->ClearForDialog();
		}

		if (!Model::gPlayer.IsDead())
			wasDead = false;
	}

	if (fDrawMap && gDebugOpenGL) {
		// Very inefficient algorithm, computing the map every frame.
		std::unique_ptr<Map> map(new Map);
		float alpha = 1.0f;
		if (!sShowAlternateBitmap)
			sTextureIterator = gDebugTextures.begin();
		glBindTexture(GL_TEXTURE_2D, *sTextureIterator); // Override
		map->Draw(alpha);
	}

	if (fSelectedObject) {
		fSelectedObject->RenderHealthBar(fHealthBar, _angleHor);
	}
	Model::gOtherPlayers->RenderPlayerStats(fHealthBar, _angleHor);
	bool newHealing = false;
	if (Model::gPlayer.fFlags & UserFlagHealed) {
		newHealing = true;
		Model::gPlayer.fFlags &= ~UserFlagHealed;
	}
	this->DrawHealingAnimation(newHealing);

	static double slAverageFps = 0.0;
	double tm = gCurrentFrameTime;
	static double prevTime = 0.0;
	double deltaTime = tm - prevTime;
	// Use a decay filter on the FPS
	slAverageFps = 0.97*slAverageFps + 0.03/deltaTime;
	prevTime = tm;

	if (gMode.Get() == GameMode::TELEPORT) {
		// Draw the teleport mode
		TeleportClick(fHealthBar, _angleHor, renderViewAngle, 0, 0, false);
	}

	//=========================================================================
	// Various text messages
	//=========================================================================
	{
		char buff[1000]; // TODO: Ugly way to create dynamic strings
		sprintf(buff, "Level %ld, Hp %d%%, Exp %d%%", Model::gPlayer.fLevel, (int)(Model::gPlayer.fHp * 100), (int)(Model::gPlayer.fExp * 100));
		static string sPrevStat;
		if (sPrevStat != buff) {
			// Only update if it changed
			sPrevStat = buff;
			fPlayerStatsOneLiner_Element->SetInnerRML(buff);
		}

		if (gCurrentPing == 0.0 || !gShowPing)
			sprintf(buff, "Triangles %7d, draw calls %d. Fps %03d", gDrawnQuads, gNumDraw, int(slAverageFps));
		else
			sprintf(buff, "Triangles %7d, draw calls %d. Fps %03d, ping %.1f ms", gDrawnQuads, gNumDraw, int(slAverageFps), gCurrentPing*1000.0);

		if (gMode.Get() == GameMode::CONSTRUCT) {
			ChunkCoord cc;
			Model::gPlayer.GetChunkCoord(&cc);
			const View::Chunk *cp = ChunkFind(&cc, false);
			unsigned int uid = 1000000;
			if (cp) {
				auto cb = cp->fChunkBlocks;
				if (cb != nullptr)
					uid = cb->fOwner;
			}
			// This will override other the other text
			sprintf(buff, "Construction mode, Chunk (%d,%d,%d) offset: %.1f,%.1f,%.1f, coord(%.1f,%.1f,%.1f) owner %d",
			        cc.x, cc.y, cc.z, playerOffset.x, playerOffset.y, playerOffset.z, Model::gPlayer.x/100.0, Model::gPlayer.y/100.0, Model::gPlayer.z/100.0, uid);
		}

		static string sPrevFPS;
		if (sPrevFPS != buff) {
			// Only update if it changed
			sPrevFPS = buff;
			fFPS_Element->SetInnerRML(buff);
		}
	}

	gEntityComponentSystem.Update();

	if (!fDrawMap && !fShowInventory) {
		if (gMode.Get() == GameMode::CONSTRUCT)
			fBuildingBlocks->Draw(gProjectionMatrix);
	}

	if (gDebugOpenGL) {
		checkError("gameDialog::render debug", false);
		static double prevPrint = 0.0;
		if (gCurrentFrameTime > prevPrint + 5.0) {
			if (gToggleTesting)
				WorstTime::Report();
			else
				TimeMeasure::Report();
			prevPrint = gCurrentFrameTime;
		}
	}
	View::Chunk::DegradeBusyList_gl();
	first = false;
}

// Catch the window close request by the player.
static int GLFWCALL CloseWindowCallback(void) {
	gMode.Set(GameMode::ESC); // This will initiate a proper shutdown
	return GL_FALSE;          // Prevent the window from closing immediately.
}

void gameDialog::init(void) {
	fRenderControl.Init(8);

	Model::ChunkBlocks::InitStatic();
	maxRenderDistance = (float)gOptions.fViewingDistance;
	if (maxRenderDistance > MAXRENDERDISTANCE) {
		// Someone tried to trick the program by editing the ini file.
		// #255 long distances are not handled very well by neither server nor client
		maxRenderDistance = MAXRENDERDISTANCE;
	}
	if (maxRenderDistance < 5.0f)
		maxRenderDistance = 5.0f;
	gTranspShader.Init();
	fBuildingBlocks = View::BuildingBlocks::Make(7); // TODO: Need something more adaptive than a constant.
	fShader = ChunkShader::Make(); // Singleton
	fHealthBar = View::HealthBar::Make(); // Singleton
	fDrawTexture = DrawTexture::Make();
	glGetFloatv(GL_VIEWPORT, &gViewport[0]);
	handleResize(gViewport[2], gViewport[3]); // Simple way to set the projection
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //Set the blend function

	glEnable(GL_CULL_FACE); // Always enabled by default

	glfwDisable(GLFW_KEY_REPEAT);
	glfwSetWindowSizeCallback(::handleResize);
	glfwSetMousePosCallback(::handleMouseActiveMotion);
	glfwSetKeyCallback(handleKeypress);
	glfwSetCharCallback(handleCharacter);
	glfwSetMouseButtonCallback(dialogHandleMouse);
	glfwSetWindowCloseCallback(CloseWindowCallback);

	fRocketGui.Init();

	fMainUserInterface.Init();
	gMsgWindow.Init(fMainUserInterface.GetElement("chat"));
	fFPS_Element = fMainUserInterface.GetElement("fps");
	if (fFPS_Element == 0)
		ErrorDialog("Missing UI definition for FPS");
	fFPS_Element->AddReference();

	fPlayerStatsOneLiner_Element = fMainUserInterface.GetElement("playerstatsoneliner");
	if (fPlayerStatsOneLiner_Element == 0)
		ErrorDialog("Missing UI definition for one line player stats");
	fPlayerStatsOneLiner_Element->AddReference();

	fInputLine = fMainUserInterface.GetElement("inputline");
	if (fInputLine == 0)
		ErrorDialog("Missing input line in main user interface");
	fInputLine->AddReference();
	fInputLine->Blur(); // Don't want a flashing cursor until player wants to input text.
	gEntityComponentSystem.Init();

	checkError("gameDialog::init");
}

void gameDialog::Update() {
	static int wheel = 0;
	static bool inWater = false;
	static bool inAir = false;

	unsigned char bl;

	// Detect usage of the mouse wheel
	int newWheel = glfwGetMouseWheel();
	int zoomDelta = 0;
	if (newWheel != wheel) {
		if (fCurrentRocketContextInput) {
			fCurrentRocketContextInput->ProcessMouseWheel(wheel-newWheel, rocketKeyModifiers);
		} else if (gMode.Get() == GameMode::CONSTRUCT) {
			fBuildingBlocks->UpdateSelection(newWheel);
		} else if (fDrawMap) {
			float fact = 1.0f + (wheel-newWheel)/10.0f;
			fMapWidth = fMapWidth * fact;
			if (fMapWidth < 100)
				fMapWidth = 100;
		} else {
			zoomDelta = wheel-newWheel;
		}
		wheel = newWheel;
	}

	Model::gPlayer.UpdatePositionSmooth();

	fRenderControl.UpdateCameraPosition(zoomDelta);

	if (gMode.Get() == GameMode::CONSTRUCT && (glfwGetKey(GLFW_KEY_DEL) == GLFW_PRESS || glfwGetKey('V') == GLFW_PRESS) && glfwGetMouseButton(GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
		int x, y;
		glfwGetMousePos(&x, &y);
		this->ClickOnBlock(x, y);
	}

	// Determine if player head is under water
	bl = View::Chunk::GetChunkAndBlock(Model::gPlayer.x, Model::gPlayer.y, Model::gPlayer.z);
	if (bl == BT_Air && fUnderWater) {
		gSoundControl.SetSoundFxStatus(SoundControl::SEnvironmentUnderWater,false);
		fUnderWater = false;
	}
	if ((bl == BT_Water || bl == BT_BrownWater) && !fUnderWater) {
		gSoundControl.SetSoundFxStatus(SoundControl::SEnvironmentUnderWater,true);
		fUnderWater = true;
		// Not in water when under water
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerFeetInWater,false);
		inWater = false;
	}

	// Determine if feet are wet
	// TODO: Change "magic number" for player height!
	bl = View::Chunk::GetChunkAndBlock(Model::gPlayer.x, Model::gPlayer.y, Model::gPlayer.z-350);
	if ((bl != BT_Water && bl != BT_BrownWater) && inWater && !fUnderWater) {
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerFeetInWater,false);
		inWater = false;
	}
	if ((bl == BT_Water || bl == BT_BrownWater) && !inWater && !fUnderWater) {
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerFeetInWater,true);
		inWater = true;
	}

	// TODO: Change "magic number" for player height!
	bl = View::Chunk::GetChunkAndBlock(Model::gPlayer.x, Model::gPlayer.y, Model::gPlayer.z-400);
	if (bl != BT_Air && inAir) {
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerInAir,false);
		inAir = false;
	}
	if (bl == BT_Air && !inAir) {
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerInAir,true);
		inAir = true;
	}
	gGameDialog.UpdateRunningStatus(false);

	if (gOptions.fFullScreen) {
		// Full screen, which means mouse is usually disabled. But there are cases when it is needed.
		bool showMouseFullscreen = false;
		static bool wasShowingMouse = false;
		if (gMode.Get() == GameMode::CONSTRUCT || fShowInventory || gMode.Get() == GameMode::TELEPORT || fCurrentRocketContextInput)
			showMouseFullscreen = true;
		if (wasShowingMouse && !showMouseFullscreen) {
			glfwDisable(GLFW_MOUSE_CURSOR);
			wasShowingMouse = false;
		}
		if (!wasShowingMouse && showMouseFullscreen) {
			glfwEnable(GLFW_MOUSE_CURSOR);
			wasShowingMouse = true;
		}
	}

	this->UpdateEffect();
	gChunkProcess.Poll(); // Update all results
}

void gameDialog::SetMessage(const char *str) {
	gMsgWindow.Add(str);
}

void gameDialog::DrawWeapon(void) const {
	GLuint text = GameTexture::WEP1;
	switch(Model::gPlayer.fWeaponType) {
	case 0:
		return; // No weapon equipped
	case 1:
		text = GameTexture::WEP1;
		break;
	case 2:
		text = GameTexture::WEP2;
		break;
	case 3:
		text = GameTexture::WEP3;
		break;
	case 4:
		text = GameTexture::WEP4;
		break;
	}
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.536f, -0.85640, 0.0f));
	float yScale = 731.0f/226.0f; // Ratio from bitmap
	float scale = 0.5f;
	model = glm::scale(model, glm::vec3(scale, scale*yScale, 1.0f));
	static const glm::mat4 id(1.0f); // No need to create a new one every time.
	glBindTexture(GL_TEXTURE_2D, text);
	fDrawTexture->Draw(id, model);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// Restart the drawing when flag is true.
// This is a simple animation, just moving a texture over the screen.
void gameDialog::DrawHealingAnimation(bool restart) const {
	const float height = 2.0f;
	const double visibleTime = 0.1;
	static double timerStart = -100.0; // The initial deltatime will be big enough not to show anything.
	if (restart)
		timerStart = gCurrentFrameTime;
	double deltaTime = gCurrentFrameTime - timerStart;
	float deltaY = -1.0f-height+float(deltaTime / visibleTime);
	if (deltaY > 1.0f)
		return; // The drawing is now outside
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, deltaY, 0.0f));
	model = glm::scale(model, glm::vec3(2.0f, height, 1.0f));
	static const glm::mat4 id(1.0f); // No need to create a new one every time.
	glBindTexture(GL_TEXTURE_2D, GameTexture::LightBallsHeal);
	fDrawTexture->Draw(id, model);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void gameDialog::ClearForDialog(void) {
	// TODO: Much of this is duplicate logic. That is not good.
	sTurning = false;
	if (fMovingFwd) {
		fMovingFwd = false;
		unsigned char b[] = { 0x03, 0x00, 11 };
		SendMsg(b, sizeof b);
	}
	if (fMovingLeft) {
		fMovingLeft = false;
		unsigned char b[] = { 0x03, 0x00, 15 };
		SendMsg(b, sizeof b);
	}
	if (fMovingBwd) {
		fMovingBwd = false;
		unsigned char b[] = { 0x03, 0x00, 13 };
		SendMsg(b, sizeof b);
	}
	if (fMovingRight) {
		fMovingRight = false;
		unsigned char b[] = { 0x03, 0x00, 17 };
		SendMsg(b, sizeof b);
	}
	if (gMode.Get() == GameMode::CONSTRUCT)
		gShowFramework = false; // Only when in construction mode
	else if (gMode.Get() == GameMode::TELEPORT && Model::gPlayer.fAdmin > 0 && gAdminTP) {
		gMode.Set(GameMode::GAME);
		gAdminTP = false;
	}
	gGameDialog.UpdateRunningStatus(false);
	this->ClearSelection();
}

// Manage the running status of the player
void gameDialog::UpdateRunningStatus(bool disable) {
	static bool wasRunning = false;

	// There is a timeout if the player hasn't moved for some time, which is used to cancel the sound
	bool tryingToMove = Model::gPlayer.PlayerIsMoving();

	if (!wasRunning && tryingToMove) {
		// Turn off the running sound.
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerRunning, true);
	}

	if (wasRunning && !tryingToMove) {
		// Turn off the running sound.
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerRunning,false);
	}

	wasRunning = tryingToMove;
}

// Configure for how long the zoom effect shall be active.
static const double sZoomTime = 0.5; // How long time, in seconds, that the zooming effect will be active.

void gameDialog::RequestEffect(Effect eff) {
	CancelCurrentEffect(); // Cancel previous effect, if there was one.
	switch(eff) {
	case EFFECT_NONE:
		break;
	case EFFECT_ZOOM1:
		sStartZoom = gCurrentFrameTime;
		fCurrentEffect = EFFECT_ZOOM1;
		break;
	case EFFECT_ZOOM2:
		sStartZoom = gCurrentFrameTime + sZoomTime;
		fCurrentEffect = EFFECT_ZOOM2;
		break;
	}
}

void gameDialog::CancelCurrentEffect(void) {
	switch(fCurrentEffect) {
	case EFFECT_NONE:
		break;
	case EFFECT_ZOOM1: // Fall through
	case EFFECT_ZOOM2:
		renderViewAngle = defaultRenderViewAngle;   // Restore default view angle
		this->handleResize(gViewport[2], gViewport[3]); // Restore normal projection
		break;
	}
	fCurrentEffect = EFFECT_NONE;
}

// Only used for saving picture to file
#if defined(_WIN32)
#include <windows.h>
#include <Shlobj.h>
#endif

void gameDialog::SaveScreen() {
	/// @todo Fix this for Windows also.
    int w = gViewport[2];
    w = w/4*4; /// @todo For some reason, snapshot doesn't work if width is not a factor of 4.
    int h = gViewport[3];

    // Make the BYTE array, factor of 3 because it's RBG.
    // unsigned char pixels[ 4 * w * h*2];
    std::unique_ptr<unsigned char[]> pixels(new unsigned char[3*w*h]);
    this->render(true);

    glReadPixels(0, 0, w, h, GL_BGR, GL_UNSIGNED_BYTE, pixels.get());

    FILE *f;
	int bytesPerRow = ((w * 3 + 3) / 4) * 4;
	int size = bytesPerRow * h;
    std::unique_ptr<unsigned char[]> img(new unsigned char[size]);
    int filesize = 54 + size;

    for(int i=0; i<w; i++)
    {
        for(int j=0; j<h; j++)
        {
            int x=i; int y=(h-1)-j;
            ASSERT(bytesPerRow * y + 3 * x + 2 < size);
            ASSERT(y>=0);
            img[bytesPerRow * y + 3 * x + 2] = pixels[(i+j*w)*3+2];
            img[bytesPerRow * y + 3 * x + 1] = pixels[(i+j*w)*3+1];
            img[bytesPerRow * y + 3 * x + 0] = pixels[(i+j*w)*3+0];
        }
    }

    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
    unsigned char bmppad[3] = {0,0,0};

    bmpfileheader[ 2] = (unsigned char)(filesize    );
    bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
    bmpfileheader[ 4] = (unsigned char)(filesize>>16);
    bmpfileheader[ 5] = (unsigned char)(filesize>>24);

    bmpinfoheader[ 4] = (unsigned char)(       w    );
    bmpinfoheader[ 5] = (unsigned char)(       w>> 8);
    bmpinfoheader[ 6] = (unsigned char)(       w>>16);
    bmpinfoheader[ 7] = (unsigned char)(       w>>24);
    bmpinfoheader[ 8] = (unsigned char)(       h    );
    bmpinfoheader[ 9] = (unsigned char)(       h>> 8);
    bmpinfoheader[10] = (unsigned char)(       h>>16);
    bmpinfoheader[11] = (unsigned char)(       h>>24);

	string filename;
#ifdef unix
	// Save Linux Path
	const char *home = getenv("HOME");
	filename = string(home) + "/Pictures/EphenationSnapShot.bmp";
#else
	TCHAR home[MAX_PATH];
	HRESULT res = SHGetFolderPath(NULL, CSIDL_MYPICTURES, 0, 0, home);
	if (res != S_OK) {
		View::gMsgWindow.Add("Failed to find home folder for pictures");
		return;
	}
	filename = string(home) + "\\EphenationSnapShot.bmp";
	printf("File: '%s'\n", filename.c_str());
#endif // unix

    f = fopen(filename.c_str(),"wb");
    if (f == NULL) {
		View::gMsgWindow.Add("Failed to open %s", filename.c_str());
		return;
    }
    fwrite(bmpfileheader,1,14,f);
    fwrite(bmpinfoheader,1,40,f);
    for(int i=0; i<h; i++)
    {
        fwrite(img.get()+(w*(h-i-1)*3),3,w,f);
        fwrite(bmppad,1,(4-(w*3)%4)%4,f);
    }
    fclose(f);

	View::gMsgWindow.Add("Picture saved to %s", filename.c_str());

#ifdef unix
	string viewer = "gimp " + filename + "&";
    (void)system(viewer.c_str());
#endif
}

void gameDialog::UpdateEffect(void) {
	double deltaStart = gCurrentFrameTime - sStartZoom;
	switch(fCurrentEffect) {
	case EFFECT_NONE:
		break;
	case EFFECT_ZOOM1:
		// Modify viewing angle to get zooming effects.
		if (deltaStart < sZoomTime) {
			// Active for a limited time.
			renderViewAngle = defaultRenderViewAngle - deltaStart*50.0/sZoomTime;
			this->handleResize(gViewport[2], gViewport[3]);
		} else {
			this->CancelCurrentEffect();
		}
		break;
	case EFFECT_ZOOM2:
		// Modify viewing angle to get zooming effects.
		if (deltaStart < 0) {
			// Active for a limited time.
			renderViewAngle = defaultRenderViewAngle - deltaStart*50.0/sZoomTime;
			this->handleResize(gViewport[2], gViewport[3]);
		} else {
			this->CancelCurrentEffect();
		}
		break;
	}
}

void gameDialog::CalibrateMode(Calibration cal) {
	// TODO: The dialog should go away temporarily
	fCalibrationMode = cal;
}

void gameDialog::ClearInputRedirect(void) {
	fCurrentRocketContextInput = 0;
}
