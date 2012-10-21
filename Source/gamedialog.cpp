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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <memory>
#include <GL/glew.h>
#include <GL/glfw.h>
#include <math.h>
#include <Rocket/Debugger.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "primitives.h"
#include "object.h"
#include "player.h"
#include "gamedialog.h"
#include "connection.h"
#include "client_prot.h"
#include "render.h"
#include "chunk.h"
#include "msgwindow.h"
#include "parse.h"
#include "textures.h"
#include "modes.h"
#include "monsters.h"
#include "otherplayers.h"
#include "vsfl/vsfl.h"
#include "DrawText.h"
#include "shaders/ChunkShader.h"
#include "shaders/ChunkShaderPicking.h"
#include "shaders/DeferredLighting.h"
#include "shaders/TranspShader.h"
#include "shaders/AnimationShader.h"
#include "HealthBar.h"
#include "MonsterDef.h"
#include "BuildingBlocks.h"
#include "DrawTexture.h"
#include "SoundControl.h"
#include "Options.h"
#include "Inventory.h"
#include "Map.h"
#include "ScrollingMessages.h"
#include "Teleport.h"
#include "ui/dialog.h"
#include "ui/messagedialog.h"
#include "ui/RocketGui.h"
#include "ui/Error.h"
#include "shadowrender.h"
#include "uniformbuffer.h"
#include "shadowconfig.h"
#include "billboard.h"
#include "rendercontrol.h"
#include "timemeasure.h"
#include "worsttime.h"
#include "ChunkProcess.h"

const float defaultRenderViewAngle = 60.0f;
float renderViewAngle = defaultRenderViewAngle;

// The time stamp when the player clicked on a magical portal.
static double sStartZoom;

static std::vector<unsigned>::iterator sTextureIterator;
static bool sShowAlternateBitmap = false;

static float mouseSens = -5.0f;
gameDialog gGameDialog; // For now, there will only be one

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
	fCameraDistance = 0.0f;
	fRequestedCameraDistance = 0.0f;
	fMapWidth = 600;

	fEnterDebugText = false;
	fDrawMap = false;
	fShowInventory = false;
	fShowWeapon = true;
	fShowMainDialog = false;
	fHideDialog = false; // This will override the fShowMainDialog
	fSelectedObject = 0;
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

// In game mode, find the object that the player clicked on.
// TODO: should also draw landscape so as not to allow free sight and selecting monsters behind walls.
Object *gameDialog::FindSelectedObject(int x, int y) {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Use black sky for picking
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// gMonsters.RenderMonsters(true); // Selecting monsters not supported for now
	// gOtherPlayers.RenderPlayers(true); // selecting players not supported for now
	unsigned char pixel[3];
	glReadPixels(x, gViewport[3] - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
	// printf("gameDialog::FindSelectedObject: %d,%d,%d\n", pixel[0], pixel[1], pixel[2]);
	Object *obj = 0;
	switch(pixel[0]) {
	case 1:
		obj = gMonsters.GetSelection(pixel[0], pixel[1], pixel[2]);
		break;
	case 2:
		obj = gOtherPlayers.GetSelection(pixel[0], pixel[1], pixel[2]);
		break;
	}
	return obj;
}

// In build mode, find the block at the given screen position. Return the chunk and the
// data about it to the pointers.
chunk *gameDialog::FindSelectedSurface(int x, int y, ChunkOffsetCoord *coc, int *surfaceDir) {
	ChunkShaderPicking *pickShader = ChunkShaderPicking::Make();
	pickShader->EnableProgram();
	pickShader->View(gViewMatrix);
	pickShader->Projection(gProjectionMatrix);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Use black sky for picking
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	DrawLandscape(0, DL_Picking);
	pickShader->DisableProgram();

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

	gPlayer.GetChunkCoord(&cc);
	cc.x += coding.bitmap.dx-1;
	cc.y += coding.bitmap.dy-1;
	cc.z += coding.bitmap.dz-1;
	chunk *cp = ChunkFind(&cc, false);
	coc->x = coding.bitmap.x; coc->y = coding.bitmap.y; coc->z = coding.bitmap.z;

	if (surfaceDir)
		*surfaceDir = coding.bitmap.facing;

	checkError("gameDialog::FindSelectedSurface");
	return cp;
}

ChunkCoord gRequestActivatorChunk;
int gRequestActivatorX, gRequestActivatorY, gRequestActivatorZ;

void gameDialog::AttachBlockToSurface(int row, int col) {
	ChunkOffsetCoord coc;
	int surfaceDir = 0;
	chunk *cp = FindSelectedSurface(row, col, &coc, &surfaceDir);
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
		gRequestActivatorChunk = cc;
		gRequestActivatorX = x;
		gRequestActivatorY = y;
		gRequestActivatorZ = z;
	}
}

// The player clicked on an object. Depending on mode, we either
// allow for rebuilding the environment, or attack monsters.
void gameDialog::ClickOnObject(int x, int y) {
	Object *op = this->FindSelectedObject(x, y);
	fSelectedObject = op;
}

// The player clicked one something. Depending on mode, we either
// allow for rebuilding the environment, or attack monsters.
void gameDialog::ClickOnBlock(int x, int y) {
	static int prevX = -1, prevY = -1;
	if (prevX == x && prevY == y)
		return; // Ignore multiple clicks on the same block
	prevX = x; prevY = y;
	ChunkOffsetCoord coc;
	chunk *cp = this->FindSelectedSurface(x, y, &coc, 0);
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
}

static bool sTurning = false;
int xStartTurn = 0, yStartTurn = 0;
float angleHorStartTurn = 0.0f, angleVertStartTurn;

static void handleMouseActiveMotion(int x, int y) {
	gGameDialog.handleMouseActiveMotion(x, y);
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
		gPlayer.fAngleHor = _angleHor; // Update player data with current looking direction
		gPlayer.fAngleVert = _angleVert;
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
		bool close = gInventory.HandleMouseClick(button, action, x, y);
		if (close) {
			fShowInventory = false;
			gMsgWindow.SetAlternatePosition(0,0,false);
		}
		return;
	} else if (fShowMainDialog && !fHideDialog) {
		if (action == GLFW_RELEASE) {
			fShowMainDialog = dialog::DispatchClick();
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
	if (Options::fgOptions.fFullScreen)
		aspectRatio = gDesktopAspectRatio;
	gProjectionMatrix  = glm::perspective(renderViewAngle, aspectRatio, 0.01f, maxRenderDistance);  // Create our perspective projection matrix
	gTranspShader.EnableProgram();
	gTranspShader.Projection((float)w, (float)h);
	gTranspShader.DisableProgram();
	glViewport(0, 0, w, h);
	gViewport = glm::vec4(0.0f, 0.0f, (float)w, (float)h );
	gDrawFont.UpdateProjection();
	fRenderControl.Resize(w, h);
	fMainUserInterface.Resize(w, h);
}

static void handleCharacter(int character, int action) {
	// printf("handleCharacter %d, action %d\n", character, action);
	gGameDialog.HandleCharacter(character, action);
}

void gameDialog::HandleCharacter(int key, int action) {
	if (fCurrentRocketContextInput) {
		if (action == GLFW_PRESS)
			fCurrentRocketContextInput->ProcessTextInput(key);
		return;
	}

	if (fShowMainDialog && !fHideDialog) {
		dialog::DispatchChar(key);
		return;
	}
}

static int rocketKeyModifiers = 0;

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
		fCurrentRocketContextInput->ProcessKeyDown(RocketGui::KeyMap(key), rocketKeyModifiers);
		return;
	}

	if (fShowMainDialog && !fHideDialog && key != GLFW_KEY_ESC) {
		dialog::DispatchKey(key);
		return;
	}
	if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F11) {
		gInventory.UseObjectFunctionKey(key);
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
			fMessageDialog.LoadDialog("dialogs/topleveldialog.rml");
			fCurrentRocketContextInput = fMainUserInterface.GetRocketContext();
		}
		break;
	case 'C':
		if (gMode.Get() == GameMode::CONSTRUCT) { // Toggle construction mode
			gMode.Set(GameMode::GAME);
			gShowFramework = false;
		} else {
			gMode.Set(GameMode::CONSTRUCT);
		}
		if (gPlayer.fKnownPosition) {
			ChunkCoord player_cc;
			// Force the current chunk to be redrawn, adapted for construction mode
			gPlayer.GetChunkCoord(&player_cc);
			for (int dx=-1; dx<2; dx++) for (int dy=-1; dy<2; dy++) for (int dz=-1; dz < 2; dz++) {
						ChunkCoord cc = player_cc;
						cc.x += dx; cc.y += dy; cc.z += dz;
						chunk *cp = ChunkFind(&cc, true);
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
				sTextureIterator++;
			if (sTextureIterator == gDebugTextures.end()) {
				sShowAlternateBitmap = false;
			}
			break;
		}
		// Find next monster after the selected one.
		fSelectedObject = gMonsters.GetNext(fSelectedObject);
		break;
	case GLFW_KEY_LALT: // ALT key
		if (gMode.Get() == GameMode::CONSTRUCT) {
			gShowFramework = true; // Only when in construction mode
		} else if (gMode.Get() == GameMode::GAME && gPlayer.fAdmin > 0) {
			gMode.Set(GameMode::TELEPORT);
			gAdminTP = true;
		}
		break;
	case '1': // Autoattack
		if (!fSelectedObject) {
			// Find next monster after the selected one.
			fSelectedObject = gMonsters.GetNext(fSelectedObject);
		}
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
	case GLFW_KEY_KP_SUBTRACT:
		switch(fCalibrationMode) {
		case CALIB_AMBIENT:
			Options::fgOptions.fAmbientLight -= 1;
			gMsgWindow.Add("Ambient light: %f", Options::fgOptions.fAmbientLight/100.0);
			break;
		case CALIB_EXPOSURE:
			Options::fgOptions.fExposure /= 1.1f;
			gMsgWindow.Add("Exposure: %f", Options::fgOptions.fExposure);
			break;
		case CALIB_WHITE_POINT:
			Options::fgOptions.fWhitePoint /= 1.1f;
			gMsgWindow.Add("White point: %f", Options::fgOptions.fWhitePoint);
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
			Options::fgOptions.fAmbientLight += 1;
			gMsgWindow.Add("Ambient light: %f", Options::fgOptions.fAmbientLight/100.0);
			break;
		case CALIB_EXPOSURE:
			Options::fgOptions.fExposure *= 1.1f;
			gMsgWindow.Add("Exposure: %f", Options::fgOptions.fExposure);
			break;
		case CALIB_WHITE_POINT:
			Options::fgOptions.fWhitePoint *= 1.1f;
			gMsgWindow.Add("White point: %f", Options::fgOptions.fWhitePoint);
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
		if (gPlayer.fAdmin > 0)
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
			fCurrentRocketContextInput->ProcessKeyUp(RocketGui::KeyMap(key), 0);
		return;
	}
	if (fShowMainDialog && !fHideDialog) {
		dialog::DispatchKey(key);
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
		else if (gMode.Get() == GameMode::TELEPORT && gPlayer.fAdmin > 0 && gAdminTP) {
			gMode.Set(GameMode::GAME);
			gAdminTP = false;
		}
		break;
	}
	gGameDialog.UpdateRunningStatus(false);
}

void gameDialog::ClearSelection(void) {
	fSelectedObject = 0;
}

// This function is called every time the player gets aggro
void gameDialog::AggroFrom(Object *o) {
	// It may be called from more than one monster. For now, use the information to set a selection for the first monster.
	if (fSelectedObject == 0)
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

void gameDialog::render() {
	static bool first = true; // Only true first time function is called
	// Clear list of special effects. It will be added again automatically every frame
	gShadows.Clear();
	gFogs.Clear();
	gPlayer.UpdatePositionSmooth();

	// Can't select objects that are dead
	if (fSelectedObject && (fSelectedObject->IsDead() || !fSelectedObject->InGame()))
		ClearSelection();
	glm::vec3 playerOffset = gPlayer.GetOffsetToChunk();

	gViewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -fCameraDistance));
	gViewMatrix = glm::rotate(gViewMatrix, _angleVert, glm::vec3(1.0f, 0.0f, 0.0f));
	gViewMatrix = glm::rotate(gViewMatrix, _angleHor, glm::vec3(0.0f, 1.0f, 0.0f));
	gViewMatrix = glm::translate(gViewMatrix, -playerOffset);

	glm::mat4 T1 = glm::translate(glm::mat4(1), playerOffset);
	glm::mat4 R1 = glm::rotate(glm::mat4(1), _angleVert, glm::vec3(-1.0f, 0.0f, 0.0f));
	glm::mat4 R2 = glm::rotate(glm::mat4(1), _angleHor, glm::vec3(0.0f, -1.0f, 0.0f));
	glm::mat4 T2 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, fCameraDistance));
	glm::vec4 camera = T1 * R2 * R1 * T2 * glm::vec4(0,0,0,1);
	gUniformBuffer.Camera(camera);

	gDrawnQuads = 0;
	gNumDraw = 0;

	gUniformBuffer.Update();

	if (first) {
		gBillboard.InitializeTextures(fShader);
	}

	fRenderControl.Draw(fSelectedObject, fUnderWater, this->ThirdPersonView(), fSelectedObject, fDrawMap && !gDebugOpenGL, fMapWidth, &fMainUserInterface);

	//=========================================================================
	// Various effects drawn after the deferred shader
	//=========================================================================

	if (!fDrawMap) {
		this->DrawPlayerStats();
		if (fShowWeapon && gMode.Get() != GameMode::CONSTRUCT && !fShowInventory && !this->ThirdPersonView())
			this->DrawWeapon();
		static bool wasDead = false;
		if (gPlayer.IsDead() && !wasDead) {
			wasDead = true;
			fMessageDialog.Set("Oops", "You are dead.\n\nYou will be revived, and transported back to your starting place.\n\nThe place can be changed with a scroll of resurrection point.", revive);
			fCurrentRocketContextInput = fMainUserInterface.GetRocketContext();
			this->ClearForDialog();
		} else if (fShowInventory)
			gInventory.DrawInventory(fDrawTexture);
		else if (fShowMainDialog)
			dialog::DispatchDraw(fDrawTexture, fHideDialog ? 0.5f : 1.0f);
		else if (sgPopup.length() > 0) {
			// There are some messages that shall be shown in a popup dialog.
			fMessageDialog.Set(sgPopupTitle, sgPopup, 0);
			fCurrentRocketContextInput = fMainUserInterface.GetRocketContext();
			sgPopupTitle = "Ephenation"; // Reset to default.
			sgPopup.clear();
			this->ClearForDialog();
		}

		if (!gPlayer.IsDead())
			wasDead = false;
		DrawCompassRose();
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
	gOtherPlayers.RenderPlayerStats(fHealthBar, _angleHor);
	bool newHealing = false;
	if (gPlayer.fFlags & UserFlagHealed) {
		newHealing = true;
		gPlayer.fFlags &= ~UserFlagHealed;
	}
	this->DrawHealingAnimation(newHealing);

	char buff[1000]; // TODO: Ugly way to create dynamic strings
	static double slAverageFps = 0.0;
	double tm = gCurrentFrameTime;
	static double prevTime = 0.0;
	double deltaTime = tm - prevTime;
	// Use a decay filter on the FPS
	slAverageFps = 0.97*slAverageFps + 0.03/deltaTime;
	prevTime = tm;

	if (gMode.Get() == GameMode::TELEPORT) {
		TeleportClick(fHealthBar, _angleHor, renderViewAngle, 0, 0, false);
	}

	//=========================================================================
	// Various text messages
	//=========================================================================
	{
		sprintf(buff, "Level %ld, Hp %d%%, Exp %d%%", gPlayer.fLevel, (int)(gPlayer.fHp * 100), (int)(gPlayer.fExp * 100));
		static string sPrevStat;
		if (sPrevStat != buff) {
			// Only update if it changed
			sPrevStat = buff;
			fPlayerStatsOneLiner_Element->SetInnerRML(buff);
		}

		if (gCurrentPing == 0.0)
			sprintf(buff, "Triangles %7d, draw calls %d. Fps %03d", gDrawnQuads, gNumDraw, int(slAverageFps));
		else
			sprintf(buff, "Triangles %7d, draw calls %d. Fps %03d, ping %.1f ms", gDrawnQuads, gNumDraw, int(slAverageFps), gCurrentPing*1000.0);

		if (gMode.Get() == GameMode::CONSTRUCT) {
			ChunkCoord cc;
			gPlayer.GetChunkCoord(&cc);
			const chunk *cp = ChunkFind(&cc, false);
			unsigned int uid = 1000000;
			shared_ptr<ChunkBlocks> cb;
			if (cp)
				cb = cp->fChunkBlocks;
			if (cb)
				uid = cb->fOwner;
			// This will override other the other text
			sprintf(buff, "Construction mode, Chunk (%d,%d,%d) offset: %.1f,%.1f,%.1f, coord(%.1f,%.1f,%.1f) owner %d",
			        cc.x, cc.y, cc.z, playerOffset.x, playerOffset.y, playerOffset.z, gPlayer.x/100.0, gPlayer.y/100.0, gPlayer.z/100.0, uid);
		}

		static string sPrevFPS;
		if (sPrevFPS != buff) {
			// Only update if it changed
			sPrevFPS = buff;
			fFPS_Element->SetInnerRML(buff);
		}

		gScrollingMessages.Update();
	}

	if (!fDrawMap && !fShowInventory && (!fShowMainDialog || fHideDialog)) {
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
	chunk::DegradeBusyList_gl();
	first = false;
}

void gameDialog::init(void) {
	fRenderControl.Init();

	std::shared_ptr<DrawFont> gabriola18(new DrawFont);
	gabriola18->Init("textures/gabriola18");
	gScrollingMessages.Init(gabriola18);
	ChunkBlocks::InitStatic();
	maxRenderDistance = (float)Options::fgOptions.fViewingDistance;
	this->fRequestedCameraDistance = Options::fgOptions.fCameraDistance;
	if (maxRenderDistance > MAXRENDERDISTANCE) {
		// Someone tried to trick the program by editing the ini file.
		// #255 long distances are not handled very well by neither server nor client
		maxRenderDistance = MAXRENDERDISTANCE;
	}
	if (maxRenderDistance < 5.0f)
		maxRenderDistance = 5.0f;
	gTranspShader.Init();
	fBuildingBlocks = BuildingBlocks::Make(7); // TODO: Need something more adaptive than a constant.
	gMonsterDef.Init(0);
	fShader = ChunkShader::Make(); // Singleton
	fHealthBar = HealthBar::Make(); // Singleton
	fDrawTexture = DrawTexture::Make();
	glGetFloatv(GL_VIEWPORT, &gViewport[0]);
	handleResize(gViewport[2], gViewport[3]); // Simple way to set the projection
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //Set the blend function
	unsigned char b[] = { 0x03, 0x00, CMD_GET_COORDINATE }; // GET_COORDINATE
	SendMsg(b, sizeof b);

	glEnable(GL_CULL_FACE); // Always enabled by default

	glfwDisable(GLFW_KEY_REPEAT);
	glfwSetWindowSizeCallback(::handleResize);
	glfwSetMousePosCallback(::handleMouseActiveMotion);
	glfwSetKeyCallback(handleKeypress);
	glfwSetCharCallback(handleCharacter);
	glfwSetMouseButtonCallback(dialogHandleMouse);

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

	fMessageDialog.Init(fMainUserInterface.GetRocketContext());

	checkError("gameDialog::init");
}

void gameDialog::Update() {
	static int wheel = 0;
	static bool inWater = false;
	static bool inAir = false;

	unsigned char bl;

	// Detect usage of the mouse wheel
	int newWheel = glfwGetMouseWheel();
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
			this->fRequestedCameraDistance += wheel-newWheel;
			if (this->fRequestedCameraDistance < 0.0f)
				this->fRequestedCameraDistance = 0.0f;
			if (this->fRequestedCameraDistance > 20.0f)
				this->fRequestedCameraDistance = 20.0f; // Don't allow unlimited third person view distance
		}
		wheel = newWheel;
	}
	if (gMode.Get() == GameMode::CONSTRUCT && (glfwGetKey(GLFW_KEY_DEL) == GLFW_PRESS || glfwGetKey('V') == GLFW_PRESS) && glfwGetMouseButton(GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
		int x, y;
		glfwGetMousePos(&x, &y);
		this->ClickOnBlock(x, y);
	}
	gMonsters.Cleanup();
	gOtherPlayers.Cleanup();
	while (ListenForServerMessages())
		continue;

	// Determine if player head is under water
	bl = chunk::GetChunkAndBlock(gPlayer.x, gPlayer.y, gPlayer.z);
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
	bl = chunk::GetChunkAndBlock(gPlayer.x, gPlayer.y, gPlayer.z-350);
	if ((bl != BT_Water && bl != BT_BrownWater) && inWater && !fUnderWater) {
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerFeetInWater,false);
		inWater = false;
	}
	if ((bl == BT_Water || bl == BT_BrownWater) && !inWater && !fUnderWater) {
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerFeetInWater,true);
		inWater = true;
	}

	// TODO: Change "magic number" for player height!
	bl = chunk::GetChunkAndBlock(gPlayer.x, gPlayer.y, gPlayer.z-400);
	if (bl != BT_Air && inAir) {
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerInAir,false);
		inAir = false;
	}
	if (bl == BT_Air && !inAir) {
		gSoundControl.SetSoundFxStatus(SoundControl::SPlayerInAir,true);
		inAir = true;
	}

	this->UpdateCameraPosition();
	gGameDialog.UpdateRunningStatus(false);

	if (Options::fgOptions.fFullScreen) {
		// Full screen, which means mouse is usually disabled. But there are cases when it is needed.
		bool showMouseFullscreen = false;
		static bool wasShowingMouse = false;
		if ((fShowMainDialog && !fHideDialog) || gMode.Get() == GameMode::CONSTRUCT || fShowInventory || gMode.Get() == GameMode::TELEPORT || fCurrentRocketContextInput)
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

void gameDialog::DrawPlayerStats(void) const {
	// The coordinates and sizes used here are measured from the graphical UI. No view or projection matrix is used,
	// which means the screen is from -1.0 to +1.0 in both x and y.
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.69157f, -0.81982f, 0.0f));
	model = glm::scale(model, glm::vec3(0.43802f, 0.03804f, 1.0f));
	float dmg = gPlayer.fPreviousHp - gPlayer.fHp;
	static const glm::mat4 id(1.0f); // No need to create a new one every time.
	fHealthBar->DrawHealth(id, model, gPlayer.fHp, dmg, false);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.69194f, -0.85735f, 0.0f));
	model = glm::scale(model, glm::vec3(0.38361f, 0.01788f, 1.0f));
	fHealthBar->DrawMana(model, gPlayer.fMana);

	model = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(2.0f, 0.02856f, 1.0f));
	fHealthBar->DrawExp(model, gPlayer.fExp);
}

void gameDialog::DrawWeapon(void) const {
	GLuint text = GameTexture::WEP1;
	switch(gPlayer.fWeaponType) {
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

void gameDialog::DrawCompassRose(void) const {
	static const glm::mat4 id(1.0f); // No need to create a new one every time.

	// Move compass to lower left corner.
	glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.85f, -0.80f, 0.0f));

	// Scale it down, and remove dependency of screen width/height ratio
	float scale = 0.2f;
	float screenRatio = gViewport[2] / gViewport[3];
	model = glm::scale(model, glm::vec3(scale/screenRatio, scale, scale));

	// Rotate according to vertical viewing direction
	model = glm::rotate(model, gPlayer.fAngleVert, glm::vec3(1.0f, 0.0f, 0.0f));

	// Rotate according to horisontal viewing direction
	model = glm::rotate(model, gPlayer.fAngleHor, glm::vec3(0.0f, 0.0f, 1.0f));

	// Move the center of the compass, so rotation is done around the center, not the lower left corner of the bitmap.
	model = glm::translate(model, glm::vec3(-0.5f, -0.5f, 0.0f));

	glBindTexture(GL_TEXTURE_2D, GameTexture::CompassRose);
	fDrawTexture->Draw(id, model);
	// glBindTexture(GL_TEXTURE_2D, 0);  // Deprecated

	gMonsters.RenderMinimap(model, fHealthBar);
	gOtherPlayers.RenderMinimap(model, fHealthBar);
}

void gameDialog::UpdateCameraPosition(void) {
	static double last = 0.0;
	double delta = (gCurrentFrameTime - last)*20; // Number of blocks/s
	last = gCurrentFrameTime;
	if (delta > fRequestedCameraDistance - fCameraDistance)
		delta = fRequestedCameraDistance - fCameraDistance;
	// If the requested camera distance is bigger than the current, then slowly move out.
	if (fRequestedCameraDistance > fCameraDistance)
		fCameraDistance += delta;
	else
		fCameraDistance = fRequestedCameraDistance;
	Options::fgOptions.fCameraDistance = fRequestedCameraDistance; // Make sure it is saved
	if (fCameraDistance == 0.0f)
		return;
	ChunkCoord cc;
	gPlayer.GetChunkCoord(&cc);

	glm::vec3 playerOffset = gPlayer.GetOffsetToChunk();
	glm::vec3 pd(playerOffset.x, -playerOffset.z, playerOffset.y); // Same offset, but in Ephenation server coordinates

	glm::mat4 T1 = glm::translate(glm::mat4(1), playerOffset);
	glm::mat4 R1 = glm::rotate(glm::mat4(1), _angleVert, glm::vec3(-1.0f, 0.0f, 0.0f));
	glm::mat4 R2 = glm::rotate(glm::mat4(1), _angleHor, glm::vec3(0.0f, -1.0f, 0.0f));
	glm::mat4 T2 = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, fCameraDistance));
	glm::vec4 pos = T1 * R2 * R1 * T2 * glm::vec4(0,0,0,1);

	glm::vec3 cd(pos.x, -pos.z, pos.y); // Camera delta in Ephenation coordinates.

	// printf("Player: %.2f, %.2f, %.2f. Camera: %.2f,%.2f,%.2f\n", pd.x, pd.y, pd.z, cd.x, cd.y, cd.z);
	glm::vec3 norm = glm::normalize(cd-pd); // Vector from player to camera

	const float step = 0.1f;
	for (float d = step; d <= this->fCameraDistance; d = d + step) {
		signed long long x, y, z;
		x = gPlayer.x + (signed long long)(d * norm.x*BLOCK_COORD_RES);
		y = gPlayer.y + (signed long long)(d * norm.y*BLOCK_COORD_RES);
		z = gPlayer.z + (signed long long)(d * norm.z*BLOCK_COORD_RES);
		if (chunk::GetChunkAndBlock(x, y, z) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (chunk::GetChunkAndBlock(x+1, y, z) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (chunk::GetChunkAndBlock(x, y+1, z) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (chunk::GetChunkAndBlock(x, y, z+1) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (chunk::GetChunkAndBlock(x-1, y, z) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (chunk::GetChunkAndBlock(x, y-1, z) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
		if (chunk::GetChunkAndBlock(x, y, z-1) != BT_Air) {
			this->fCameraDistance = d-step;
			break;
		}
	}
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
	else if (gMode.Get() == GameMode::TELEPORT && gPlayer.fAdmin > 0 && gAdminTP) {
		gMode.Set(GameMode::GAME);
		gAdminTP = false;
	}
	gGameDialog.UpdateRunningStatus(false);
	fSelectedObject = 0;
}

void gameDialog::NotifyMessage(void) {
	// If the dialog is dimmed down, bring it back again. This will make sure the player
	// understands there are more messages.
	if (fShowMainDialog && fHideDialog) {
		fHideDialog = false;
		this->ClearForDialog();
	}
}

// Manage the running status of the player
void gameDialog::UpdateRunningStatus(bool disable) {
	static bool wasRunning = false;

	// There is a timeout if the player hasn't moved for some time, which is used to cancel the sound
	bool tryingToMove = gPlayer.PlayerIsMoving();

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
	if (fShowMainDialog)
		fHideDialog = true;
	fCalibrationMode = cal;
}

void gameDialog::ClearInputRedirect(void) {
	fCurrentRocketContextInput = 0;
}
