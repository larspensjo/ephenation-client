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

#ifndef MONSTERDEF_H_
#define MONSTERDEF_H_

class MonsterShader;

class MonsterDef {
public:
	struct coord;
	struct bodyPart;
	struct attachment;
	struct VertexData;
	struct HeadDescription;
	MonsterDef();
	virtual ~MonsterDef();
	bool Init(const char *); // Initialize a monster from a JSON definition. Return success status.
	void Draw(const glm::mat4 &model, const glm::mat4 &view, bool upsideDown, GLuint faceTexture, float sun, float ambient);
	static void Projection(const glm::mat4 &);

	// Setters
	void SetVersion(int);
	void SetScale(int);
	void SetFlags(unsigned long);
	void SetColor(int n, float f);
	void SetLegs(bodyPart *, int size); void SetArms(bodyPart *, int size); void SetHeads(bodyPart *, int size); void SetTrunk(coord *, int size);
	void SetLegsAttach(attachment*, int size); void SetArmsAttach(attachment*, int size); void SetHeadsAttach(attachment*, int size);
	bool Valid(void) const;
private:
	void TraverseCoords(coord *, int size, void (*func)(MonsterDef::coord*, HeadDescription *hdesc), HeadDescription *hdesc);
	void FindAllTriangles(void);

	int fVersion;
	unsigned long fFlags;
	float fScale;
	float fColorAddon[4];
	bool fValid;
	bodyPart *fLegs; int fNumLegs;
	bodyPart *fArms; int fNumArms;
	bodyPart *fHeads; int fNumHeads;
	coord *fBody; int fBodySize;
	// The count of attachments match the count of body parts.
	attachment *fLegsAttach, *fArmsAttach, *fHeadsAttach;
	static MonsterShader *fShader;
	GLuint fBufferId; // Only one needed for all vertex info
	GLuint fIndexBufferId;
	GLuint fVao; // For the Vertex array object

	// Data used for drawing
	unsigned short int fBeginLegIndicies, fNumLegIndicies;
	unsigned short int fBeginArmIndicies, fNumArmIndicies;
	unsigned short int fBeginHeadIndicies, fNumHeadIndicies;
	unsigned short int fBeginBodyIndicies, fNumBodyIndicies;
};

// For now, there is only one monster
extern MonsterDef gMonsterDef;

#endif /* MONSTERDEF_H_ */
