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

#ifndef RANDOMMONSTER_H_
#define RANDOMMONSTER_H_

#include <map>

// This is kind of a singleton factory, creating MonsterDef from a given level

class MonsterDef;

class RandomMonster {
public:
	MonsterDef *GetMonsterDef(unsigned int level); // Find the monster definition for the specified level
	static RandomMonster *Make(void);
	static float Size(unsigned int level); // Get the size of a monster at the specified level
private:
	MonsterDef *MakeMonsterDef(unsigned int level) const; // Create a new monster definition
	RandomMonster(); // Force singleton behavior
	static RandomMonster fgRandomMonster; // Only one of these, for the singleton
	std::map<unsigned int, MonsterDef*> fMap;
};

#endif /* RANDOMMONSTER_H_ */
