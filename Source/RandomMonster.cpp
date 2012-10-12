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
#include <map>
#include <stdio.h>
#include <string>

#include <glm/glm.hpp>
#include "RandomMonster.h"
#include "MonsterDef.h"
#include "simplexnoise1234.h"
#include "ui/Error.h"

using namespace std;

RandomMonster::RandomMonster() {
}

MonsterDef *RandomMonster::GetMonsterDef(unsigned int level) {
	MonsterDef *p = fMap[level];

	if (p == 0) {
		p = MakeMonsterDef(level);
		fMap[level] = p;
	}
	return p;
}

RandomMonster RandomMonster::fgRandomMonster;

RandomMonster *RandomMonster::Make(void) {
	// No Init() needed for this class
	return &fgRandomMonster;
}

// Create a random number that depends on 'level' and 'n'. It would be possible to use snoise2(), but
// it would be a waste as there is no need for gradual change between points.
static int rand(unsigned int level, int n, int min, int max) {
	float f = snoise1(level*5.123872f + n*11.12387f + 23.123137f);
	if (f < 0.0f)
		f = -f;
	int rnd = int(f * 10000);
	int diff = max-min+1;
	return (rnd % diff) + min;
}

#define NELEM(x) (sizeof x / sizeof x[0])

float RandomMonster::Size(unsigned int level) {
	// This algorithm is the same one used by the server. Do not change it, as the server must have the same
	// knowledge of monster sizes as the client
	unsigned int rnd = (level + 137) * 871; // pseudo random 32-bit number
	float rnd2 = float(rnd & 0xff) / 255.0f; // Random number 0-1
	rnd2 *= rnd2;
	rnd2 *= rnd2;
	rnd2 = 1.0f + rnd2*4.0f; // The monster size will range from 1 blocks to 5 blocks
	return rnd2;
}

MonsterDef *RandomMonster::MakeMonsterDef(unsigned int level) const {
	int q = 1; // A simple way to get differernt random number seeds.
	int legLength = rand(level, q++, 2, 6);
	int bodyLength = rand(level, q++, 3, 10);
	int bodyWidth = rand(level, q++, 1, 5);
	int bodyHeight = rand(level, q++, 1, 6);
	int headWidth = bodyWidth-2; // A little smaller than the body
	if (headWidth < 1)
		headWidth += 2;
	int headHeight = bodyHeight-1;
	if (headHeight < 1)
		headHeight = 1;
	int headLength = bodyLength/3+1;
	int redDelta = rand(level, q++, -40, 60);
	int greenDelta = rand(level, q++, -40, 60);
	int blueDelta = rand(level, q++, -40, 60);
	// The monster size algorithm is known by the server. If it is changed, it has to be changed at both places.
	float rnd2 = Size(level);
	int totalBlockHeight = bodyHeight + legLength - 1;
	int scale = int(256 * rnd2 / totalBlockHeight);
	// printf("Monster lvl %d size %.1f (%d)\n", level, rnd2, scale);
#if 0
	printf("RandomMonster::MakeMonsterDef level %d, legLength %d, body (%d,%d,%d), scale %d, head (%d,%d,%d)\n",
	       level, legLength, bodyWidth, bodyHeight, bodyLength, scale, headWidth, headHeight, headLength);
	printf("    Color addon (%d,%d,%d)\n", redDelta, greenDelta, blueDelta);
#endif
	char buff[1000]; // TODO: Ugly way to reserve enough space
	sprintf(buff, "{\"Version\":1,\"Scale\":%d,\"Flag\":0,\"Legs\":[[", scale);
	string str = buff;
	int x, y, z;
	for (int limb=0; limb<4; limb++) {
		switch(limb) {
		case 0: x = z = 0; break;
		case 1: x = bodyWidth+1; z = 0; break;
		case 2: x = bodyWidth+1; z = 1-bodyLength; break;
		case 3: x = 0; z = 1-bodyLength; break;
		}
		for (int i=0; i < legLength; i++) {
			sprintf(buff, "{\"X\":%d,\"Y\":%d,\"Z\":%d}%s", x, i, z, i<legLength-1 ? "," : "");
			str += buff;
		}
		if (limb < 3)
			str += "],[";
	}
	str += "]],\"Trunk\":[";
	bool first = true;
	for (x=0; x<bodyWidth; x++) for (y=0; y<bodyHeight; y++) for (z=0; z<bodyLength; z++) {
				sprintf(buff, "%s{\"X\":%d,\"Y\":%d,\"Z\":%d}", first?"":",", x+1, y+legLength-1, -z);
				str += buff;
				first = false;
			}
	str += "],\"Heads\":[[";
	// It is possible to define multiple heads, which is not used here.
	first = true;
	int dx = (bodyWidth - headWidth)/2 + 1;
	for (x=0; x<headWidth; x++) for (y=0; y<headHeight; y++) for (z=0; z<headLength; z++) {
				sprintf(buff, "%s{\"X\":%d,\"Y\":%d,\"Z\":%d}", first?"":",", x+dx, y+legLength-1, headLength-z);
				str += buff;
				first = false;
			}
	sprintf(buff, "]],\"Red\":%d,\"Green\":%d,\"Blue\":%d", redDelta, greenDelta, blueDelta);
	str += buff;
	str += "}";
	MonsterDef *mp = new MonsterDef;
	// printf("%s\n", str.c_str());
	mp->Init(str.c_str());
	if (!mp->Valid()) {
		ErrorDialog("Invalid random monster %s\n", str.c_str());
	}
	return mp;
}
