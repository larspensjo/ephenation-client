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

#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <Rocket/Debugger.h>

#include "RocketGui.h"

#if 0
#include "GUI/ScriptInterface.h"
#include "GUI/ASEventInstancer.h"
#include "GUI/ScriptableDocumentInstancer.h"
#include "Scripting/ScriptManager.h"
#endif

void RocketGui::Init()
{
	fRocketRenderInterface.Init();
	Rocket::Core::SetRenderInterface(&fRocketRenderInterface);
	Rocket::Core::SetSystemInterface(&fRocketSystemInterface);

	Rocket::Core::Initialise();
	Rocket::Controls::Initialise();

	LoadFonts("TBD/");

   /*
    * Register custom instancers

   Rocket::Core::Factory::RegisterEventListenerInstancer(new Rocket::Angelscript::EventListenerInstancer(*m_ScriptManager, "main"))->RemoveReference();
   Rocket::Core::Factory::RegisterElementInstancer("body", new Rocket::Angelscript::ScriptableDocumentInstancer(*m_ScriptManager, "main"))->RemoveReference();

   */
}

RocketGui::~RocketGui() {
	Rocket::Core::SetSystemInterface(0);
	Rocket::Core::SetRenderInterface(0);
}

void RocketGui::LoadFonts(const std::string& dir)
{
#if 0
   StringVector fonts;
   fonts.push_back("Delicious-Bold.otf");
   fonts.push_back("Delicious-Roman.otf");
   fonts.push_back("Delicious-Italic.otf");
   fonts.push_back("LindenHill.ttf");
   fonts.push_back("Anonymous.ttf");

   std::for_each(fonts.begin(), fonts.end(), [&](const std::string& font) {
      Rocket::Core::FontDatabase::LoadFontFace((dir + font).c_str());
   });
#endif
}

void RocketGui::RegisterScripting(ScriptManager& scriptmgr)
{
#if 0
   asIScriptEngine* engine = scriptmgr.GetEngine();

   Rocket::Angelscript::RegisterTypes(engine);
   Rocket::Angelscript::RegisterVariantType(engine);
   Rocket::Angelscript::RegisterContextInterface(engine);
   Rocket::Angelscript::RegisterElementInterface(engine);
   Rocket::Angelscript::RegisterElementDocumentInterface(engine);
   Rocket::Angelscript::RegisterEventInterface(engine);
#endif
}
