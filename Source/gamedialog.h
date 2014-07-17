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

#include "ui/mainuserinterface.h"
#include "ui/RocketGui.h"
#include "rendercontrol.h"
#include "chunk.h"
#include "UndoOp.h"
#include "RenderTarget.h"

namespace View {
	class Chunk;
	class RenderControl;
	class HealthBar;
    class BuildingBlocks;
	class RenderTarget;
}

namespace Model {
	class Object;
}

struct ChunkOffsetCoord;
struct ChunkCoord;
class ChunkShader;
class DrawTexture;

namespace Rocket {
	namespace Core {
		class Context; // Lots of work to forward declare this
	}
};

namespace Controller {

/// @brief This corresponds to the controller class in a Model/View/Controller.
/// - keyboard input
/// - mouse input
/// - decide what shall be shown on the screen
/// - window resize events
/// - a few special effects (zooming)
/// - manage construction of activator blocks
/// @todo This class has grown to be rather fat. A refactoring is needed.
/// @todo Some things here remains from old, and is not really part of the controller class.
class gameDialog {
public:
	gameDialog();
	/// Called every frame
	/// @param hideGUI Hide all GUI, for use when taking pictures, etc.
	void DrawScreen(bool hideGUI);

	/// Display a stereo view, using reprojection to correct for delay
	/// @param yaw The yaw angle used to create the original pictures
	/// @param yaw The pitch angle used to create the original pictures
	/// @param rightOriginal The texture for the right eye, not corrected
	/// @param leftOriginal The texture for the left ete, not corrected
	void DisplayReprojection(float yaw, float pitch, View::RenderTarget &rightOriginal, View::RenderTarget &leftOriginal);

	/// Render many things.
	/// @todo Nothing should be rendered from here, it should all go into the View of the MVC.
	/// @return The resulting render target
	std::unique_ptr<View::RenderTarget> render();

	/// Apply post render effects
	/// @param source The render target to add the effects to
	void postRender(bool hideGUI, int fps);
	void init(bool useOvr);
	~gameDialog();
	void handleMouse(int button, int action);
	/// Polled to update various states
	void Update(void);
	void SetMessage(const char *);
	void HandleKeyRelease(int key); // Keyboard event
	void HandleKeyPress(int key);	// Keyboard event
	void HandleCharacter(int key, int action);	// Character event
	void handleResize(int w, int h);
	void handleMouseActiveMotion(int x, int y);

	void ClickOnBlock(int x, int y);
	void ClickOnObject(int x, int y);
	void AttachBlockToSurface(int row, int col);
	void CreateActivatorMessage(int dx, int dy, int dz, const ChunkCoord &cc);
	void GetActivator(int &dx, int &dy, int &dz, ChunkCoord &cc); // Get the current activator location
	void ClearSelection(void); // Clear the selected object
	void AggroFrom(std::shared_ptr<const Model::Object>); // The player now has aggro from this monster
	enum class Calibration { Exposure, WhitePoint, Ambient, Factor, None };
	void CalibrateMode(Calibration);

	/// Manage the running status of the player
	void UpdateRunningStatus(bool disable);

	/// Define possible effects that can be requested. They will all terminate automatically.
	enum Effect {
		EFFECT_NONE,  /// Normal mode, no special effects.
		EFFECT_ZOOM1, /// Zoom from normal view to half viewing angle in 0.5s
		EFFECT_ZOOM2  /// Zoom from 120 degrees viewing angle to normal angle in 0.5s
	};
	void RequestEffect(Effect eff);
	void CancelCurrentEffect(void);
	void UpdateEffect(void);

	/// If there is a redirect of inputs to a Rocket context, then clear it.
	void ClearInputRedirect(void);

private:
	/// Compute the projection matrix.
	enum class ViewType { left, right, single };
	void UpdateProjection(ViewType v);

	/// For now, the y offset is always 0.
	void SetViewport(float x, float w, float h) {
		glViewport(x, 0, w, h);
		gViewport = glm::vec4(x, 0, w, h);
	}

	Effect fCurrentEffect;
	bool fMovingFwd;
	bool fMovingBwd;
	bool fMovingLeft;
	bool fMovingRight;
	bool fUsingTorch;

	float fAngleHor = 0.0f;            //The rotation of the player
	float fAngleVert = 0.0f;
	Calibration fCalibrationMode = Calibration::None;
	float fCalibrationFactor = 1.0f;
	void UpdateCalibrationConstant(bool increase);

	enum class GuiMode { Default, Inventory, Map, EnterText } fGuiMode = GuiMode::Default;
	bool fShowMouse = false; // Show the built-in mouse. Should only be used when Windows mouse is disabled.
	Rocket::Core::Context *fCurrentRocketContextInput; // All keyboard and mouse events are redirected to this one if it is non-null
	bool fShowWeapon = true;
	bool fUnderWater; // True when player is below water
	View::Chunk *FindSelectedSurface(int x, int y, ChunkOffsetCoord *, int *surfaceDir);
	/// Call this when a dialog will be showed, to stop movement, etc.
	/// All key presses will henceforth be forwarded to the dialog, so the player can't stop moving.
	void ClearForDialog(void);
	std::shared_ptr<const Model::Object> fSelectedObject; // Pointer to the object (monster, player, ...) that is selected
	ChunkShader *fShader;
	View::BuildingBlocks *fBuildingBlocks;
	View::HealthBar *fHealthBar;
	DrawTexture *fDrawTexture;
	View::RenderControl fRenderControl;

	RocketGui fRocketGui; // Has to be declared before the MainUserInterface

	MainUserInterface fMainUserInterface;
	Rocket::Core::Element *fFPS_Element, *fPlayerStatsOneLiner_Element, *fInputLine;

	// Save the place where a text activator was requested.
	ChunkCoord fRequestActivatorChunk;
	int fRequestActivatorX, fRequestActivatorY, fRequestActivatorZ;

	UndoOp fUndoOperator; //for undo/redo

	/// Save the game screen to a file
	void SaveScreen();

	bool fStereoView = false;

	//
	// TODO: The following parameters should be moved to the View.
	//
	int fScreenWidth = 0, fScreenHeight = 0;
	int fMapWidth;
	float fDefaultRenderViewAngle = 60.0f;
	float fRenderViewAngle = fDefaultRenderViewAngle;
	float fAspectRatio = 1.0f;

	/// Draw the player weapon
	/// @todo Should go int the View of the MVC.
	void DrawWeapon(void) const;
	/// Draw a healing self animation
	/// @todo Should go into the View of the MVC.
	void DrawHealingAnimation(bool restart) const;

	/// Draw a small square on the screen
	void DrawMarker(int x, int y);
	void DrawMarker(float x, float y);
};

extern gameDialog gGameDialog;

}

extern float renderViewAngle;

/// A string that automatically will be shown in a pop up window.
extern string sgPopup;
/// Use this title in the next popup message window.
extern string sgPopupTitle;
