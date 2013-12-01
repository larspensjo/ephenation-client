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
#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>

#include "RocketGui.h"


static const int KEYMAP_SIZE = GLFW_KEY_LAST+1;
static Rocket::Core::Input::KeyIdentifier key_identifier_map[KEYMAP_SIZE];

static void InitialiseKeymap(void)
{
	// Initialise the key map with default values.
	memset(key_identifier_map, 0, sizeof(key_identifier_map));

	key_identifier_map[GLFW_KEY_BACKSPACE] = Rocket::Core::Input::KI_BACK;
	key_identifier_map[GLFW_KEY_TAB] = Rocket::Core::Input::KI_TAB;
//	key_identifier_map[XK_Clear & 0xFF] = Rocket::Core::Input::KI_CLEAR;
	key_identifier_map[GLFW_KEY_ENTER] = Rocket::Core::Input::KI_RETURN;
	key_identifier_map[GLFW_KEY_PAUSE] = Rocket::Core::Input::KI_PAUSE;
	key_identifier_map[GLFW_KEY_SCROLL_LOCK] = Rocket::Core::Input::KI_SCROLL;
	key_identifier_map[GLFW_KEY_ESC] = Rocket::Core::Input::KI_ESCAPE;
	key_identifier_map[GLFW_KEY_DEL] = Rocket::Core::Input::KI_DELETE;

//	key_identifier_map[XK_Kanji & 0xFF] = Rocket::Core::Input::KI_KANJI;
//	key_identifier_map[XK_Muhenkan & 0xFF] = Rocket::Core::Input::; /* Cancel Conversion */
//	key_identifier_map[XK_Henkan_Mode & 0xFF] = Rocket::Core::Input::; /* Start/Stop Conversion */
//	key_identifier_map[XK_Henkan & 0xFF] = Rocket::Core::Input::; /* Alias for Henkan_Mode */
//	key_identifier_map[XK_Romaji & 0xFF] = Rocket::Core::Input::; /* to Romaji */
//	key_identifier_map[XK_Hiragana & 0xFF] = Rocket::Core::Input::; /* to Hiragana */
//	key_identifier_map[XK_Katakana & 0xFF] = Rocket::Core::Input::; /* to Katakana */
//	key_identifier_map[XK_Hiragana_Katakana & 0xFF] = Rocket::Core::Input::; /* Hiragana/Katakana toggle */
//	key_identifier_map[XK_Zenkaku & 0xFF] = Rocket::Core::Input::; /* to Zenkaku */
//	key_identifier_map[XK_Hankaku & 0xFF] = Rocket::Core::Input::; /* to Hankaku */
//	key_identifier_map[XK_Zenkaku_Hankaku & 0xFF] = Rocket::Core::Input::; /* Zenkaku/Hankaku toggle */
//	key_identifier_map[XK_Touroku & 0xFF] = Rocket::Core::Input::KI_OEM_FJ_TOUROKU;
//	key_identifier_map[XK_Massyo & 0xFF] = Rocket::Core::Input::KI_OEM_FJ_MASSHOU;
//	key_identifier_map[XK_Kana_Lock & 0xFF] = Rocket::Core::Input::; /* Kana Lock */
//	key_identifier_map[XK_Kana_Shift & 0xFF] = Rocket::Core::Input::; /* Kana Shift */
//	key_identifier_map[XK_Eisu_Shift & 0xFF] = Rocket::Core::Input::; /* Alphanumeric Shift */
//	key_identifier_map[XK_Eisu_toggle & 0xFF] = Rocket::Core::Input::; /* Alphanumeric toggle */

	key_identifier_map[GLFW_KEY_HOME] = Rocket::Core::Input::KI_HOME;
	key_identifier_map[GLFW_KEY_LEFT] = Rocket::Core::Input::KI_LEFT;
	key_identifier_map[GLFW_KEY_UP] = Rocket::Core::Input::KI_UP;
	key_identifier_map[GLFW_KEY_RIGHT] = Rocket::Core::Input::KI_RIGHT;
	key_identifier_map[GLFW_KEY_DOWN] = Rocket::Core::Input::KI_DOWN;
	key_identifier_map[GLFW_KEY_PAGEUP] = Rocket::Core::Input::KI_PRIOR;
	key_identifier_map[GLFW_KEY_PAGEDOWN] = Rocket::Core::Input::KI_NEXT;
	key_identifier_map[GLFW_KEY_END] = Rocket::Core::Input::KI_END;
//	key_identifier_map[XK_Begin & 0xFF] = Rocket::Core::Input::KI_HOME;

//	key_identifier_map[XK_Print & 0xFF] = Rocket::Core::Input::KI_SNAPSHOT;
	key_identifier_map[GLFW_KEY_INSERT] = Rocket::Core::Input::KI_INSERT;
	key_identifier_map[GLFW_KEY_KP_NUM_LOCK] = Rocket::Core::Input::KI_NUMLOCK;

	key_identifier_map[GLFW_KEY_SPACE] = Rocket::Core::Input::KI_SPACE;
//	key_identifier_map[XK_KP_Tab & 0xFF] = Rocket::Core::Input::KI_TAB;
	key_identifier_map[GLFW_KEY_KP_ENTER] = Rocket::Core::Input::KI_NUMPADENTER;
//	key_identifier_map[XK_KP_F1 & 0xFF] = Rocket::Core::Input::KI_F1;
//	key_identifier_map[XK_KP_F2 & 0xFF] = Rocket::Core::Input::KI_F2;
//	key_identifier_map[XK_KP_F3 & 0xFF] = Rocket::Core::Input::KI_F3;
//	key_identifier_map[XK_KP_F4 & 0xFF] = Rocket::Core::Input::KI_F4;
//	key_identifier_map[XK_KP_Home & 0xFF] = Rocket::Core::Input::KI_NUMPAD7;
//	key_identifier_map[XK_KP_Left & 0xFF] = Rocket::Core::Input::KI_NUMPAD4;
//	key_identifier_map[XK_KP_Up & 0xFF] = Rocket::Core::Input::KI_NUMPAD8;
//	key_identifier_map[XK_KP_Right & 0xFF] = Rocket::Core::Input::KI_NUMPAD6;
//	key_identifier_map[XK_KP_Down & 0xFF] = Rocket::Core::Input::KI_NUMPAD2;
//	key_identifier_map[XK_KP_Prior & 0xFF] = Rocket::Core::Input::KI_NUMPAD9;
//	key_identifier_map[XK_KP_Next & 0xFF] = Rocket::Core::Input::KI_NUMPAD3;
//	key_identifier_map[XK_KP_End & 0xFF] = Rocket::Core::Input::KI_NUMPAD1;
//	key_identifier_map[XK_KP_Begin & 0xFF] = Rocket::Core::Input::KI_NUMPAD5;
//	key_identifier_map[XK_KP_Insert & 0xFF] = Rocket::Core::Input::KI_NUMPAD0;
//	key_identifier_map[XK_KP_Delete & 0xFF] = Rocket::Core::Input::KI_DECIMAL;
	key_identifier_map[GLFW_KEY_KP_EQUAL] = Rocket::Core::Input::KI_OEM_NEC_EQUAL;
	key_identifier_map[GLFW_KEY_KP_MULTIPLY] = Rocket::Core::Input::KI_MULTIPLY;
	key_identifier_map[GLFW_KEY_KP_ADD] = Rocket::Core::Input::KI_ADD;
//	key_identifier_map[XK_KP_Separator & 0xFF] = Rocket::Core::Input::KI_SEPARATOR;
	key_identifier_map[GLFW_KEY_KP_SUBTRACT] = Rocket::Core::Input::KI_SUBTRACT;
	key_identifier_map[GLFW_KEY_KP_DECIMAL] = Rocket::Core::Input::KI_DECIMAL;
	key_identifier_map[GLFW_KEY_KP_DIVIDE] = Rocket::Core::Input::KI_DIVIDE;

	for (int i=0; i<25; i++)
		key_identifier_map[GLFW_KEY_F1+i] = Rocket::Core::Input::KeyIdentifier(Rocket::Core::Input::KI_F1 + i);

	key_identifier_map[GLFW_KEY_LSHIFT] = Rocket::Core::Input::KI_LSHIFT;
	key_identifier_map[GLFW_KEY_RSHIFT] = Rocket::Core::Input::KI_RSHIFT;
	key_identifier_map[GLFW_KEY_LCTRL] = Rocket::Core::Input::KI_LCONTROL;
	key_identifier_map[GLFW_KEY_RCTRL] = Rocket::Core::Input::KI_RCONTROL;
	key_identifier_map[GLFW_KEY_CAPS_LOCK] = Rocket::Core::Input::KI_CAPITAL;

//	key_identifier_map[GLFW_KEY_MENU] = Rocket::Core::Input::KI_LMENU;
	key_identifier_map[GLFW_KEY_MENU] = Rocket::Core::Input::KI_RMENU;

	key_identifier_map['\''] = Rocket::Core::Input::KI_OEM_7; // Apostrophe
	key_identifier_map[','] = Rocket::Core::Input::KI_OEM_COMMA;
	key_identifier_map['-'] = Rocket::Core::Input::KI_OEM_MINUS;
	key_identifier_map['.'] = Rocket::Core::Input::KI_OEM_PERIOD;
	key_identifier_map['/'] = Rocket::Core::Input::KI_OEM_2;
	for (int i=0; i<10; i++)
		key_identifier_map['0'+i] = Rocket::Core::Input::KeyIdentifier(Rocket::Core::Input::KI_0 + i);

	key_identifier_map[';'] = Rocket::Core::Input::KI_OEM_1;
	key_identifier_map['+'] = Rocket::Core::Input::KI_OEM_PLUS;
	key_identifier_map['['] = Rocket::Core::Input::KI_OEM_4;
	key_identifier_map['\\'] = Rocket::Core::Input::KI_OEM_5;
	key_identifier_map[']'] = Rocket::Core::Input::KI_OEM_6;
	key_identifier_map['`'] = Rocket::Core::Input::KI_OEM_3;
	// Map lowercase to libRocket characters. Not sure if this is really needed.
	for (int i='a'; i<='z'; i++)
		key_identifier_map[i] = Rocket::Core::Input::KeyIdentifier(Rocket::Core::Input::KI_A + i - 'a');
	// Map upper to libRocket characters.
	for (int i='A'; i<='Z'; i++)
		key_identifier_map[i] = Rocket::Core::Input::KeyIdentifier(Rocket::Core::Input::KI_A + i - 'A');
}

Rocket::Core::Input::KeyIdentifier RocketGui::KeyMap(int key) {
	return key_identifier_map[key];
}

void RocketGui::Init(bool stereoView, float fieldOfView)
{
	fRocketRenderInterface.Init(stereoView, fieldOfView);
	Rocket::Core::SetRenderInterface(&fRocketRenderInterface);
	Rocket::Core::SetSystemInterface(&fRocketSystemInterface);

	Rocket::Core::Initialise();
	Rocket::Controls::Initialise();

	LoadFonts("fonts/");

	InitialiseKeymap();
}

RocketGui::~RocketGui() {
	Rocket::Core::Shutdown();
	Rocket::Core::SetSystemInterface(0);
	Rocket::Core::SetRenderInterface(0);
}

void RocketGui::LoadFonts(const std::string& dir)
{
	Rocket::Core::FontDatabase::LoadFontFace((dir + "Gabriola.ttf").c_str());
	Rocket::Core::FontDatabase::LoadFontFace((dir + "georgia.ttf").c_str());
}
