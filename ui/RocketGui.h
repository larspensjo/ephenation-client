#pragma once

class ScriptManager;

class RocketGui
{
public:
	void Init();

	void RegisterScripting(ScriptManager& scriptmgr);

	void LoadFonts(const std::string& dir);
};
