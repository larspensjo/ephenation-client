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

#include <map>
#include <vector>
#include <set>

#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "MonsterDef.h"
#include "shaders/MonsterShader.h"
#include "primitives.h"
#include "textures.h"
#include "ui/Error.h"

// #define DEBYAJL

enum State {
	SInit = 0,
	STopMap = 1, // Seen the top level "start map"
	SVersionKey = 2, // Waiting for the version key
	SLegsKey = 3,
	SLegsArray = 301,
	SLegPart = 302,
	SArmsKey = 4,
	SArmsArray = 401,
	SArmsPart = 402,
	SHeadsKey = 5,
	SHeadsArray = 501,
	SHeadsPart = 502,
	SLegsAttachKey = 6,
	SLegsAttachPart = 601,
	SArmsAttachKey = 7,
	sArmsAttachPart = 701,
	SHeadsAttachKey = 8,
	sHeadsAttachPart = 801,
	SCoordinate = 10,
	SXCoord = 1001,
	SYCoord = 1002,
	SZCoord = 1003,
	SAttachment = 11,
	SB1 = 1101,
	SB2 = 1102,
	SScaleKey = 12,
	STrunkKey = 13,
	STrunkPart = 1301,
	SFlagsKey = 14,
	SRedKey = 15,
	SGreenKey = 16,
	SBlueKey = 17,
};

static enum State state;

static int maxX, maxY, maxZ; // Find the bounding size

// State stack
static enum State stateStack[10];
static int stateStackPointer = 0;
static void pushStateStack(enum State s) {
	stateStack[stateStackPointer++] = state;
	state = s;
}
static void popStateStack(void) {
	state = stateStack[--stateStackPointer];
}

static struct MonsterDef::coord {
	int x, y, z; // TODO: int is more than needed.
} coord;

struct MonsterDef::bodyPart { int fSize; MonsterDef::coord *fCoord; };

struct MonsterDef::attachment { MonsterDef::coord b1, b2; };

static MonsterDef::attachment attach;
static int numAttachs;
static int maxAttachList = 100;
static MonsterDef::attachment *currentAttachList = 0;
static void addAttach(void) {
	if (currentAttachList == 0) {
		currentAttachList = (MonsterDef::attachment *)malloc(maxAttachList * sizeof (MonsterDef::attachment));
		numAttachs = 0;
	}
	if (numAttachs == maxAttachList) {
		maxAttachList *= 2;
		currentAttachList = (MonsterDef::attachment*)realloc(currentAttachList, maxAttachList * sizeof (MonsterDef::attachment));
	}
	memcpy(&currentAttachList[numAttachs], &attach, sizeof attach);
	numAttachs++;
}

static int numCoords;
static int maxCoordList = 100;
static MonsterDef::coord *currentCoordList = 0;
static void addCoord(void) {
	if (currentCoordList == 0) {
		currentCoordList = (MonsterDef::coord *)malloc(maxCoordList * sizeof (MonsterDef::coord));
		numCoords = 0;
	}
	if (numCoords == maxCoordList) {
		maxCoordList *= 2;
		currentCoordList = (MonsterDef::coord*)realloc(currentCoordList, maxCoordList * sizeof (MonsterDef::coord));
	}
	currentCoordList[numCoords] = coord;
	if (coord.x > maxX)
		maxX = coord.x;
	if (coord.y > maxY)
		maxY = coord.y;
	if (coord.z > maxZ)
		maxZ = coord.z;
	numCoords++;
}

static int numBodyParts;
static int maxBodyPartList = 10;
static MonsterDef::bodyPart *currentBodyPartList = 0;
static void addBodyPart(void) {
	if (currentBodyPartList == 0) {
		currentBodyPartList = (MonsterDef::bodyPart *)malloc(maxBodyPartList * sizeof (MonsterDef::bodyPart));
		numBodyParts = 0;
	}
	if (numBodyParts == maxBodyPartList) {
		maxBodyPartList *= 2;
		currentBodyPartList = (MonsterDef::bodyPart*)realloc(currentBodyPartList, maxBodyPartList * sizeof (MonsterDef::bodyPart));
	}
	currentBodyPartList[numBodyParts].fCoord = currentCoordList;
	currentBodyPartList[numBodyParts].fSize = numCoords;
	currentCoordList = 0;
	numBodyParts++;
}

static int reformat_null(void * ctx) {
	ErrorDialog("yajl NULL\n");
	return 0;
}

static int reformat_double(void * ctx, double number) {
	ErrorDialog("yajl double %lg\n", number);
	return 0;
}

static int reformat_int(void * ctx, long number) {
	MonsterDef *p = (MonsterDef*)ctx;
#ifdef DEBYAJL
	printf("yajl int %ld\n", number);
#endif
	switch(state) {
	case SVersionKey:
		p->SetVersion(number);
		state = STopMap;
		return 1;
	case SXCoord:
		coord.x = number;
		state = SCoordinate;
		return 1;
	case SYCoord:
		coord.y = number;
		state = SCoordinate;
		return 1;
	case SZCoord:
		coord.z = number;
		state = SCoordinate;
		return 1;
	case SScaleKey:
		p->SetScale(number);
		state = STopMap;
		return 1;
	case SFlagsKey:
		p->SetFlags(number);
		state = STopMap;
		return 1;
	case SRedKey:
		p->SetColor(0, number / 255.0f);
		state = STopMap;
		return 1;
	case SGreenKey:
		p->SetColor(1, number / 255.0f);
		state = STopMap;
		return 1;
	case SBlueKey:
		p->SetColor(2, number / 255.0f);
		state = STopMap;
		return 1;
	default:
		ErrorDialog("reformat_int: Unexpected int in state %d\n", state);
	}
	return 0;
}

static int reformat_boolean(void * ctx, int boolean) {
	ErrorDialog("yajl unexpected bool %d\n", boolean);
	return 0;
}

static int reformat_string(void * ctx, const unsigned char * stringVal,
                           unsigned int stringLen) {
	char buff[100];
	strncpy(buff, (const char*)stringVal, stringLen);
	buff[stringLen] = 0;
	ErrorDialog("yajl unexpected string \"%s\"\n", buff);
	return 0;
}

static int reformat_map_key(void * ctx, const unsigned char * stringVal,
                            unsigned int stringLen) {
	char buff[100];
	strncpy(buff, (const char *)stringVal, stringLen);
	buff[stringLen] = 0;
#ifdef DEBYAJL
	printf("yajl map key \"%s\"\n", buff);
#endif
	switch(state) {
	case STopMap:
		if (strcmp(buff, "Version") == 0) state = SVersionKey;
		if (strcmp(buff, "Scale") == 0) state = SScaleKey;
		if (strcmp(buff, "Red") == 0) state = SRedKey;
		if (strcmp(buff, "Green") == 0) state = SGreenKey;
		if (strcmp(buff, "Blue") == 0) state = SBlueKey;
		if (strcmp(buff, "Flag") == 0) state = SFlagsKey;
		if (strcmp(buff, "Legs") == 0) state = SLegsKey;
		if (strcmp(buff, "Arms") == 0) state = SArmsKey;
		if (strcmp(buff, "Heads") == 0) state = SHeadsKey;
		if (strcmp(buff, "Trunk") == 0) state = STrunkKey;
		if (strcmp(buff, "LegsAttach") == 0) state = SLegsAttachKey;
		if (strcmp(buff, "ArmsAttach") == 0) state = SArmsAttachKey;
		if (strcmp(buff, "HeadsAttach") == 0) state = SHeadsAttachKey;
		if (state == STopMap) {
			ErrorDialog("Unexpected map key \"%s\" in state %d\n", buff, state);
			return 0;
		}
		return 1;
	case SCoordinate:
		if (strcmp(buff, "X") == 0) state = SXCoord;
		if (strcmp(buff, "Y") == 0) state = SYCoord;
		if (strcmp(buff, "Z") == 0) state = SZCoord;
		if (state == SCoordinate) {
			ErrorDialog("Unexpected map key \"%s\" in state %d\n", buff, state);
			return 0;
		}
		return 1;
	case SAttachment:
		if (strcmp(buff, "B1") == 0) state = SB1;
		if (strcmp(buff, "B2") == 0) state = SB2;
		if (state == SAttachment) {
			ErrorDialog("Unexpected map key \"%s\" in state %d\n", buff, state);
			return 0;
		}
		return 1;
	default:
		ErrorDialog("Unexpected map key \"%s\" in state %d\n", buff, state);
		break;
	}
	return 0;
}

static int reformat_start_map(void * ctx) {
#ifdef DEBYAJL
	printf("yajl start map\n");
#endif
	switch (state) {
	case SInit: state = STopMap; return 1;
	case SLegPart: case SArmsPart: case SHeadsPart: case STrunkPart:
		pushStateStack(SCoordinate); coord.x = 0; coord.y = 0; coord.z = 0; return 1;
	case SLegsAttachPart: case sArmsAttachPart: case sHeadsAttachPart:
		pushStateStack(SAttachment); return 1;
	case SB1: case SB2:
		pushStateStack(SCoordinate); return 1;
	default:
		ErrorDialog("Unexpected start map in state %d\n", state);
		break;
	}
	return 0;
}

static int reformat_end_map(void * ctx) {
#ifdef DEBYAJL
	printf("yajl end map\n");
#endif
	switch(state) {
	case SCoordinate:
		popStateStack();
		switch (state) {
		case SB1: attach.b1 = coord; state = SAttachment; return 1;
		case SB2: attach.b2 = coord; state = SAttachment; return 1;
		default: addCoord(); return 1;
		}
	case STopMap:
		state = SInit;
		return 1;
	case SAttachment:
		popStateStack();
		addAttach();
		return 1;
	default:
		printf("Unexpected end map in state %d\n", state);
		break;
	}
	return 0;
}

static int reformat_start_array(void * ctx) {
#ifdef DEBYAJL
	printf("yajl start array\n");
#endif
	switch (state) {
	case SLegsKey: state = SLegsArray; return 1;
	case SLegsArray: state = SLegPart; numCoords = 0; return 1;
	case SArmsKey: state = SArmsArray; return 1;
	case SArmsArray: state = SArmsPart; numCoords = 0; return 1;
	case SHeadsKey: state = SHeadsArray; return 1;
	case SHeadsArray: state = SHeadsPart; numCoords = 0; return 1;
	case STrunkKey: state = STrunkPart; numAttachs = 0; return 1;
	case SLegsAttachKey: state = SLegsAttachPart; numAttachs = 0; return 1;
	case SArmsAttachKey: state = sArmsAttachPart; numAttachs = 0; return 1;
	case SHeadsAttachKey: state = sHeadsAttachPart; numAttachs = 0; return 1;
	default:
		ErrorDialog("Unexpected start array in state %d\n", state);
		break;
	}
	return 0;
}

static int reformat_end_array(void * ctx) {
	MonsterDef *p = (MonsterDef*)ctx;
#ifdef DEBYAJL
	printf("yajl end array\n");
#endif
	switch (state) {
	case SLegPart: state = SLegsArray; addBodyPart(); return 1;
	case SLegsArray:
		state = STopMap;
		p->SetLegs(currentBodyPartList, numBodyParts); currentBodyPartList = 0;
		return 1;
	case SArmsPart: state = SArmsArray; addBodyPart(); return 1;
	case SArmsArray:
		state = STopMap;
		p->SetArms(currentBodyPartList, numBodyParts); currentBodyPartList = 0;
		return 1;
	case SHeadsPart: state = SHeadsArray; addBodyPart(); return 1;
	case SHeadsArray:
		state = STopMap;
		p->SetHeads(currentBodyPartList, numBodyParts); currentBodyPartList = 0;
		return 1;
	case STrunkPart:
		p->SetTrunk(currentCoordList, numCoords);
		currentCoordList = 0;
		state = STopMap;
		return 1;
	case SLegsAttachPart:
		p->SetLegsAttach(currentAttachList, numAttachs);
		currentAttachList = 0;
		state = STopMap;
		return 1;
	case sArmsAttachPart:
		p->SetArmsAttach(currentAttachList, numAttachs);
		currentAttachList = 0;
		state = STopMap;
		return 1;
	case sHeadsAttachPart:
		p->SetHeadsAttach(currentAttachList, numAttachs);
		currentAttachList = 0;
		state = STopMap;
		return 1;
	default:
		ErrorDialog("Unexpected end array in state %d\n", state);
		break;
	}
	return 0;
}

static yajl_callbacks callbacks = {
	reformat_null,
	reformat_boolean,
	reformat_int,
	reformat_double,
	NULL,
	reformat_string,
	reformat_start_map,
	reformat_map_key,
	reformat_end_map,
	reformat_start_array,
	reformat_end_array
};

MonsterDef::MonsterDef() {
	fVersion = 0; // First legal version is 1.
	fLegs = 0;
	fArms = 0;
	fHeads = 0;
	fLegsAttach = 0;
	fArmsAttach = 0;
	fHeadsAttach = 0;
	fValid = false;
	fScale = 0.0f;
	fBody = 0;
	fBodySize = 0;
	fFlags = 0;
	fNumArms = 0;
	fNumHeads = 0;
	fBufferId = 0;
	fIndexBufferId = 0;
	fVao = 0;
	fColorAddon[0] = 0.0f;
	fColorAddon[1] = 0.0f;
	fColorAddon[2] = 0.0f;
	fColorAddon[3] = 1.0f; // Initialize to no transparency
}

MonsterDef::~MonsterDef() {
	free(fLegsAttach);
	free(fArmsAttach);
	free(fHeadsAttach);
	if (glDeleteBuffers != 0) {
		// Sometimes destructed before OpenGL has been initialized.
		glDeleteBuffers(1, &fBufferId);
		glDeleteBuffers(1, &fIndexBufferId);
		glDeleteVertexArrays(1, &fVao);
	}
}

void MonsterDef::SetVersion(int v) {
	this->fVersion = v;
	if (v == 1) {
		fValid = true;
	} else {
		ErrorDialog("Invalid monster definition format version %d\n", v);
	}
}

void MonsterDef::SetScale(int v) {
	this->fScale = v/256.0f;
}

void MonsterDef::SetArmsAttach(attachment *a, int size) {
	if (size != this->fNumArms) {
		// TODO: Fix arms attachments
		ErrorDialog("Found %d arm attachements for %d arms\n", numAttachs, this->fArms->fSize);
		fValid = false;
	}
}

void MonsterDef::SetLegsAttach(attachment *a, int size) {
	if (size != this->fNumLegs) {
		// TODO: Fix legs attachments
		ErrorDialog("Found %d leg attachements for %d legs\n", numAttachs, this->fLegs->fSize);
		fValid = false;
	}
}

void MonsterDef::SetHeadsAttach(attachment *a, int size) {
	if (size != this->fNumHeads) {
		// TODO: Fix heads attachments
		ErrorDialog("Found %d head attachements for %d heads\n", numAttachs, this->fHeads->fSize);
		fValid = false;
	}
}

void MonsterDef::SetLegs(MonsterDef::bodyPart *b, int size) {
	this->fLegs = b;
	this->fNumLegs = size;
}

void MonsterDef::SetArms(MonsterDef::bodyPart *a, int size) {
	this->fArms = a;
	this->fNumArms = size;
}

void MonsterDef::SetHeads(MonsterDef::bodyPart *h, int size) {
	this->fHeads = h;
	this->fNumHeads = size;
}

void MonsterDef::SetTrunk(MonsterDef::coord *body, int size) {
	fBody = body;
	fBodySize = size;
}

// Used for testing
static const char *fileData = "{\"Version\":1,\"Scale\":128,\"Flag\":0,\"Legs\":[[{\"X\":0,\"Y\":0,\"Z\":0},{\"X\":0,\"Y\":1,\"Z\":0}],[{\"X\":2,\"Y\":0,\"Z\":0},{\"X\":2,\"Y\":1,\"Z\":0}]],\"Arms\":[[{\"X\":0,\"Y\":3,\"Z\":0},{\"X\":0,\"Y\":4,\"Z\":0}],[{\"X\":2,\"Y\":3,\"Z\":0},{\"X\":2,\"Y\":4,\"Z\":0}]],\"Heads\":[[{\"X\":0,\"Y\":6,\"Z\":0},{\"X\":0,\"Y\":7,\"Z\":0},{\"X\":1,\"Y\":6,\"Z\":0},{\"X\":1,\"Y\":7,\"Z\":0},{\"X\":2,\"Y\":6,\"Z\":0},{\"X\":2,\"Y\":7,\"Z\":0}]],\"Trunk\":[{\"X\":1,\"Y\":1,\"Z\":0},{\"X\":1,\"Y\":2,\"Z\":0},{\"X\":1,\"Y\":3,\"Z\":0},{\"X\":1,\"Y\":4,\"Z\":0},{\"X\":1,\"Y\":5,\"Z\":0}],\"LegsAttach\":[{\"B1\":{\"X\":0,\"Y\":1,\"Z\":0},\"B2\":{\"X\":0,\"Y\":1,\"Z\":0}},{\"B1\":{\"X\":2,\"Y\":1,\"Z\":0},\"B2\":{\"X\":2,\"Y\":1,\"Z\":0}}],\"ArmsAttach\":[{\"B1\":{\"X\":0,\"Y\":4,\"Z\":0},\"B2\":{\"X\":0,\"Y\":4,\"Z\":0}},{\"B1\":{\"X\":2,\"Y\":4,\"Z\":0},\"B2\":{\"X\":2,\"Y\":4,\"Z\":0}}],\"HeadsAttach\":[{\"B1\":{\"X\":1,\"Y\":5,\"Z\":0},\"B2\":{\"X\":1,\"Y\":5,\"Z\":0}}]}";

MonsterShader *MonsterDef::fShader = 0;

bool MonsterDef::Init(const char *json) {
	if (json == 0)
		json = fileData;
	yajl_handle hand;
	yajl_status stat;
	/* allow comments */
	yajl_parser_config cfg = { 1, 1 };

	maxX = maxY = maxZ = 0;
	/* ok.  open file.  let's read and parse */
	hand = yajl_alloc(&callbacks, &cfg, NULL, this);
	state = SInit;
	stat = yajl_parse(hand, (unsigned char*)json, strlen(json));
	stat = yajl_parse_complete(hand);
	if (stat != yajl_status_ok)
		this->fValid = false;
	yajl_free(hand);
	fShader = MonsterShader::Make(); // Singleton class
	this->FindAllTriangles();
	checkError("MonsterDef::Init");
	return this->fValid;
}

bool MonsterDef::Valid(void) const {
	return fValid;
}

void MonsterDef::SetFlags(unsigned long f) {
	this->fFlags = f;
}

void MonsterDef::SetColor(int n, float f) {
	this->fColorAddon[n] = f;
}

// Define a set of coordinates. This set will contain all coordinates for one body part.
// The purpose is to enable tests if a coordinate is used or not. A 3D array would be
// quicker, but we don't know how big they need to be.
struct cmp_coord {
	bool operator()(const MonsterDef::coord &v1, const MonsterDef::coord &v2) {
		if (v1.x < v2.x)
			return true;
		if (v1.x > v2.x)
			return false;
		if (v1.y < v2.y)
			return true;
		if (v1.y > v2.y)
			return false;
		if (v1.z < v2.z)
			return true;
		return false;
	}
};

static std::set<MonsterDef::coord, cmp_coord> coordSet;

void MonsterDef::TraverseCoords(MonsterDef::coord *c, int size, void (*func)(MonsterDef::coord *, HeadDescription *hdesc), HeadDescription *hdesc) {
	coordSet.clear();
	for (int i=0; i<size; i++)
		coordSet.insert(c[i]);
	for (int i=0; i<size; i++)
		func(&c[i], hdesc);
}

// A structure with all information needed by the draw() function. Unsigned char is used to
// get compact data.
struct MonsterDef::VertexData {
	float x, y, z; // The relative coordinate
	float xt, yt; // texture coordinate, scaled by 1/255
	float nx, ny, nz;
	float ax, ay, az; // The attachment coordinate
	float rx, ry, rz; // The rotating axis at the attachment
};

struct cmp_str {
	bool operator()(const MonsterDef::VertexData &v1, const MonsterDef::VertexData &v2) {
		if (v1.x < v2.x)
			return true;
		if (v1.x > v2.x)
			return false;
		if (v1.y < v2.y)
			return true;
		if (v1.y > v2.y)
			return false;
		if (v1.z < v2.z)
			return true;
		if (v1.z > v2.z)
			return false;

		if (v1.nx < v2.nx)
			return true;
		if (v1.nx > v2.nx)
			return false;
		if (v1.ny < v2.ny)
			return true;
		if (v1.ny > v2.ny)
			return false;
		if (v1.nz < v2.nz)
			return true;
		if (v1.nz > v2.nz)
			return false;

		if (v1.xt < v2.xt)
			return true;
		if (v1.xt > v2.xt)
			return false;
		if (v1.yt < v2.yt)
			return true;
		return false;
	}
};

static std::map<MonsterDef::VertexData, unsigned short, cmp_str> triangleMap;
static std::vector<unsigned short> indicies;
static std::vector<MonsterDef::VertexData> vertices;

// Helper struct for finding all faces.
struct transform {
	MonsterDef::coord v[4];
	MonsterDef::coord n;
};

#define ARRAY_SIZE(v) (sizeof v / sizeof v[0])
// Enumerate all transformations (all 6 square faces of a cube)
// All quads are defined in clockwise order.
transform trans[] = {
	{{{0,0,1},{0,1,1},{1,1,1},{1,0,1}},{0,0,1}}, // Near face
	{{{0,0,0},{0,1,0},{0,1,1},{0,0,1}},{-1,0,0}}, // Left face
	{{{1,0,1},{1,1,1},{1,1,0},{1,0,0}},{1,0,0}}, // Right face
	{{{1,0,0},{1,1,0},{0,1,0},{0,0,0}},{0,0,-1}}, // Far face
	{{{0,1,1},{0,1,0},{1,1,0},{1,1,1}},{0,1,0}}, // Top face
	{{{1,0,1},{1,0,0},{0,0,0},{0,0,1}},{0,-1,0}}, // Bottom face
};

// These match the order the faces are defined in 'trans[]' above.
enum { FACE_NEAR, FACE_LEFT, FACE_RIGHT, FACE_FAR, FACE_TOP, FACE_BOTTOM };

// Only save every vertex once, and get the index of it. Use a mp to know if the vertex has
// already been saved and, if so, what the index was.
unsigned short getIndex(const MonsterDef::VertexData &v) {
	// The value 0 is reserved for recognizing a not defined value
	unsigned short ind = triangleMap[v];
	if (ind == 0) {
		// This vertex was not defined. Generate a new index.
		vertices.push_back(v);
		ind = vertices.size();
		triangleMap[v] = ind; // 1 too big, to reserve for the value 0.
	}
	return ind-1;
}

struct MonsterDef::HeadDescription {
	int xoffset, xsize; // Offset from x=0 to beginning of head
	int yoffset, ysize; // Offset from y=0 to beginning of head
	int zoffset, zsize; // Offset from z=0 to beginning of head
};

#define MHRES 60 // Width of monster texture face. This is the same as the monster shader texture size divided by 3
#define TEX0(x,offs,size) (MHRES+int(x-offs)*MHRES/size)
#define TEX1OFFSET(size) (MHRES/size)
// For a block at a coordinate, find all 6 faces and 6x2 triangles.
// If this body part is the head, special handling should be done for the texture mapping
static void FindBodyPartTriangles(MonsterDef::coord *c, MonsterDef::HeadDescription *hdesc) {
	if (hdesc) {
		// printf("FindBodyPartTriangles head %d,%d,%d offset %d,%d,%d, size %d,%d,%d\n", c->x, c->y, c->z, hdesc->xoffset, hdesc->yoffset, hdesc->zoffset, hdesc->xsize, hdesc->ysize, hdesc->zsize);
		// Can't handle bigger face just now
		if (hdesc->xsize > MHRES)
			hdesc->xsize = MHRES;
		if (hdesc->ysize > MHRES)
			hdesc->ysize = MHRES;
		if (hdesc->zsize > MHRES)
			hdesc->zsize = MHRES;
	}
	// Every block has 6 sides (faces), with 2 triangles each
	for (unsigned int face=0; face<ARRAY_SIZE(trans); face++) {
		MonsterDef::coord neighbor;
		neighbor.x = c->x + trans[face].n.x;
		neighbor.y = c->y + trans[face].n.y;
		neighbor.z = c->z + trans[face].n.z;
		if (coordSet.find(neighbor) != coordSet.end()) {
			// If this neighbor exists, there is no meaning drawing a square facing in that direction.
			continue;
		}

		// Each face has 4 vertices
		MonsterDef::coord v[4];
		for (int j=0; j<4; j++) {
			v[j].x = c->x + trans[face].v[j].x;
			v[j].y = c->y + trans[face].v[j].y;
			v[j].z = c->z + trans[face].v[j].z;
		}
		int xt0, yt0;
		switch(face) { // Matching the 6 faces from trans[].
		case FACE_NEAR: xt0 = v[0].x; yt0 = v[0].y; break;
		case FACE_LEFT: xt0 = v[0].z; yt0 = v[0].y; break;
		case FACE_RIGHT: xt0 = v[0].z; yt0 = v[0].y; break;
		case FACE_FAR: xt0 = v[0].x; yt0 = v[0].y; break;
		case FACE_TOP: xt0 = v[0].x; yt0 = v[0].z; break;
		case FACE_BOTTOM: xt0 = v[0].x; yt0 = v[0].z; break;
		}
		unsigned char xt1 = xt0+MHRES, yt1 = yt0+MHRES;
		if (hdesc) {
			switch(face) {
			case FACE_NEAR:
				xt0 = TEX0(xt0,hdesc->xoffset, hdesc->xsize);
				yt0 = TEX0(yt0,hdesc->yoffset, hdesc->ysize);
				xt1 = xt0 + TEX1OFFSET(hdesc->xsize);
				yt1 = yt0 + TEX1OFFSET(hdesc->ysize);
				break;
			case FACE_RIGHT:
				xt0 = 2*MHRES + (hdesc->zsize+hdesc->zoffset-xt0)*TEX1OFFSET(hdesc->zsize);
				yt0 = TEX0(yt0,hdesc->yoffset, hdesc->ysize);
				xt1 = xt0 + TEX1OFFSET(hdesc->zsize);
				yt1 = yt0 + TEX1OFFSET(hdesc->ysize);
				break;
			case FACE_LEFT:
				xt0 = (xt0-hdesc->zoffset)*TEX1OFFSET(hdesc->zsize);
				yt0 = TEX0(yt0,hdesc->yoffset, hdesc->ysize);
				xt1 = xt0 + TEX1OFFSET(hdesc->zsize);
				yt1 = yt0 + TEX1OFFSET(hdesc->ysize);
				break;
			case FACE_TOP:
				xt0 = MHRES+(xt0-hdesc->xoffset)*TEX1OFFSET(hdesc->xsize);
				yt0 = 2*MHRES + (hdesc->zsize+hdesc->zoffset-yt0)*TEX1OFFSET(hdesc->zsize);
				xt1 = xt0 + TEX1OFFSET(hdesc->xsize);
				yt1 = yt0 + TEX1OFFSET(hdesc->zsize);
				break;
			case FACE_BOTTOM:
				xt0 = MHRES+(xt0-hdesc->xoffset-1)*TEX1OFFSET(hdesc->xsize);
				yt0 = MHRES - (hdesc->zsize+hdesc->zoffset-yt0)*TEX1OFFSET(hdesc->zsize);
				xt1 = xt0 + TEX1OFFSET(hdesc->xsize);
				yt1 = yt0 - TEX1OFFSET(hdesc->zsize);
				break;
			}
		}

		// Define 3 vertices for the first triangle, which is 6 in total for a square.
		// The monsters triangles are drawn CW, which is not the default for culling
		MonsterDef::VertexData vert;
		vert.x = v[0].x; vert.y = v[0].y; vert.z = v[0].z; vert.xt = xt0; vert.yt = yt0;
		vert.nx = trans[face].n.x; vert.ny = trans[face].n.y; vert.nz = trans[face].n.z;
		// TODO: Fix 'vert' attachment and rotating axis
		indicies.push_back(getIndex(vert));

		vert.x = v[1].x; vert.y = v[1].y; vert.z = v[1].z; vert.xt = xt0; vert.yt = yt1;
		indicies.push_back(getIndex(vert));

		vert.x = v[2].x; vert.y = v[2].y; vert.z = v[2].z; vert.xt = xt1; vert.yt = yt1;
		indicies.push_back(getIndex(vert));

		indicies.push_back(getIndex(vert)); // Use the same vertex for the start of the second triangle

		vert.x = v[3].x; vert.y = v[3].y; vert.z = v[3].z; vert.xt = xt1; vert.yt = yt0;
		indicies.push_back(getIndex(vert));

		vert.x = v[0].x; vert.y = v[0].y; vert.z = v[0].z; vert.xt = xt0; vert.yt = yt0;
		indicies.push_back(getIndex(vert));
	}
}

static unsigned short *ConvertIndiciesToArray() {
	int size = indicies.size();
	unsigned short *ret = new unsigned short[size];
	for (int i=0; i<size; i++)
		ret[i] = indicies[i];
	indicies.clear();
	return ret;
}

static MonsterDef::VertexData *ConvertVerticesToArray() {
	int size = vertices.size();
	MonsterDef::VertexData *ret = new MonsterDef::VertexData[size];
	for (int i=0; i<size; i++)
		ret[i] = vertices[i];
	vertices.clear();
	return ret;
}

// Find the offset and size of a body part.
static void FindBodyPartSize(MonsterDef::coord *c, int size, MonsterDef::HeadDescription *hdesc) {
	int xMin = c[0].x, yMin = c[0].y, zMin = c[0].z;
	int xMax = xMin, yMax = yMin, zMax = zMin;
	for (int i=1; i<size; i++) {
		int x = c[i].x, y = c[i].y, z = c[i].z;
		if (x > xMax) xMax = x;
		if (y > yMax) yMax = y;
		if (z > zMax) zMax = z;
		if (x < xMin) xMin = x;
		if (y < yMin) yMin = y;
		if (z < zMin) zMin = z;
	}
	hdesc->xoffset = xMin; hdesc->yoffset = yMin; hdesc->zoffset = zMin;
	hdesc->xsize = xMax-xMin+1; hdesc->ysize = yMax-yMin+1; hdesc->zsize = zMax-zMin+1;
}

void MonsterDef::FindAllTriangles(void) {
	triangleMap.clear();
	indicies.clear();
	vertices.clear();

	fBeginLegIndicies = indicies.size();
	for (int i=0; i<fNumLegs; i++)
		TraverseCoords(fLegs[i].fCoord, fLegs[i].fSize, FindBodyPartTriangles, 0);
	// printf("For legs, Indicies: %d, vertices: %d\n", indicies.size(), vertices.size());
	fNumLegIndicies = indicies.size() - fBeginLegIndicies;

	fBeginArmIndicies = indicies.size();
	for (int i=0; i<fNumArms; i++)
		TraverseCoords(fArms[i].fCoord, fArms[i].fSize, FindBodyPartTriangles, 0);
	// printf("For arms accumulated, Indicies: %d, vertices: %d\n", indicies.size(), vertices.size());
	fNumArmIndicies = indicies.size() - fBeginArmIndicies;

	fBeginHeadIndicies = indicies.size();
	struct HeadDescription hdesc;
	for (int i=0; i<fNumHeads; i++) {
		FindBodyPartSize(fHeads[i].fCoord, fHeads[i].fSize, &hdesc);
		TraverseCoords(fHeads[i].fCoord, fHeads[i].fSize, FindBodyPartTriangles, &hdesc);
	}
	// printf("For heads accumulated, Indicies: %d, vertices: %d\n", indicies.size(), vertices.size());
	fNumHeadIndicies = indicies.size() - fBeginHeadIndicies;

	fBeginBodyIndicies = indicies.size();
	TraverseCoords(fBody, fBodySize, FindBodyPartTriangles, 0);
	// printf("For body accumulated, Indicies: %d, vertices: %d\n", indicies.size(), vertices.size());
	fNumBodyIndicies = indicies.size() - fBeginBodyIndicies;

	int sizeIndices = indicies.size();
	unsigned short *arrayIndices = ConvertIndiciesToArray();
	// Only use one buffer for all vertices
	int sizeVertices = vertices.size();
	MonsterDef::VertexData *arrayVertices = ConvertVerticesToArray();
	glGenVertexArrays(1, &fVao);
	glBindVertexArray(fVao);
	fShader->EnableVertexAttribArray();
	glGenBuffers(1, &fBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, fBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeVertices * sizeof (MonsterDef::VertexData), arrayVertices, GL_STATIC_DRAW);
	delete []arrayVertices;
	int bufferSize = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if ((unsigned)bufferSize != sizeVertices * sizeof (MonsterDef::VertexData)) {
		checkError("MonsterDef::FindAllTriangles data size mismatch");
		glDeleteBuffers(1, &fBufferId);
		fBufferId = 0;
		ErrorDialog("MonsterDef::FindAllTriangles: Data size is mismatch with input array\n");
	}
	MonsterDef::VertexData *p = 0;
	fShader->VertexAttribPointer(sizeof (MonsterDef::VertexData), &p->x);
	fShader->TextureAttribPointer(sizeof (MonsterDef::VertexData), &p->xt);
	fShader->NormalAttribPointer(sizeof (MonsterDef::VertexData), &p->nx);
	// glBindBuffer(GL_ARRAY_BUFFER, 0);  // Deprecated

	glGenBuffers(1, &fIndexBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fIndexBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeIndices * sizeof (unsigned short), arrayIndices, GL_STATIC_DRAW);
	delete []arrayIndices;
	bufferSize = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
	if ((unsigned)bufferSize != sizeIndices * sizeof (unsigned short)) {
		checkError("MonsterDef::FindAllTriangles data size mismatch");
		glDeleteBuffers(1, &fBufferId);
		fBufferId = 0;
		ErrorDialog("MonsterDef::FindAllTriangles: Data size is mismatch with input array\n");
	}
	glBindVertexArray(0);
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // Not needed after disabling VAO.
}

void MonsterDef::Draw(const glm::mat4 &model, const glm::mat4 &view, bool upsideDown, GLuint faceTexture, float sun, float ambient) {
	glBindVertexArray(fVao);
	fShader->EnableProgram();
	glm::mat4 newModel = glm::translate(model,glm::vec3(-1.0f, 0.0f, 0.0f)); // This will move the monster above the selection circle, almost. TODO: a more exact algorithm is needed.
	if (upsideDown) {
		newModel = glm::rotate(newModel, 90.0f, glm::vec3(0.0f, 0.0f, 1.0f));
		fShader->ModelView(glm::scale(newModel, glm::vec3(fScale, fScale, fScale)), view, sun, ambient);
	} else {
		fShader->ModelView(glm::scale(newModel, glm::vec3(fScale, fScale, fScale)), view, sun, ambient);
	}
	fShader->ColorAddon(this->fColorAddon);
	glBindTexture(GL_TEXTURE_2D, GameTexture::RedScalesId);
	glDrawElements(GL_TRIANGLES, fNumLegIndicies, GL_UNSIGNED_SHORT, (void*)(fBeginLegIndicies * sizeof (unsigned short)));
	glDrawElements(GL_TRIANGLES, fNumArmIndicies, GL_UNSIGNED_SHORT, (void*)(fBeginArmIndicies * sizeof (unsigned short)));
	glBindTexture(GL_TEXTURE_2D, faceTexture);
	glDrawElements(GL_TRIANGLES, fNumHeadIndicies, GL_UNSIGNED_SHORT, (void*)(fBeginHeadIndicies * sizeof (unsigned short)));
	glBindTexture(GL_TEXTURE_2D, GameTexture::Fur1Id);
	glDrawElements(GL_TRIANGLES, fNumBodyIndicies, GL_UNSIGNED_SHORT, (void*)(fBeginBodyIndicies * sizeof (unsigned short)));
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);  // Deprecated
	glBindVertexArray(0);
	fShader->DisableProgram();
	gDrawnQuads += (fNumLegIndicies+fNumArmIndicies+fNumHeadIndicies+fNumBodyIndicies)/3;
	checkError("MonsterDef::Draw");
}

MonsterDef gMonsterDef;
