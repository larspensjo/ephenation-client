#pragma once

#include "RocketSystemInterface.h"

class ScriptManager;

class RocketGui
{
public:
	void Init();

	void RegisterScripting(ScriptManager& scriptmgr);

	void LoadFonts(const std::string& dir);

	~RocketGui();
private:
	RocketSystemInterface fRocketSystemInterface;
};
