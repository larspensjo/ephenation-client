/* --------------------------------------------------

Lighthouse3D

VSML - Very Simple Font Library

http://www.lighthouse3d.com/very-simple-libs

----------------------------------------------------*/

#include <string>

// VSFL requires TinyXML
#include <tinyxml.h>

#include <glbinding/gl/functions33.h>
#include <glbinding/gl/enum33.h>

#include "vsfl.h"
#include <glm/glm.hpp>
#include "../shaders/SimpleTextureShader.h"
#include "../imageloader.h"
#include "../primitives.h"

using namespace gl33;

SimpleTextureShader *VSFLFont::sShader = 0;

// Constructor
// init Devil
// grab the singleton for VSML
VSFLFont::VSFLFont():
	mHeight(0),
	mNumChars(0),
	mFontTex(0),
	mPrevDepth(GL_FALSE) {
}

// Clear chars info, and sentences
VSFLFont::~VSFLFont() {
	if (mFontTex == 0)
		return;
	mChars.clear();

	// Free resources for each sentence
	std::vector<VSFLSentence>::iterator iter = mSentences.begin();

	for ( ; iter != mSentences.end(); ++iter) {
		(*iter).clear();
	}

	glDeleteTextures(1, &mFontTex);
}

// Generate slots for sentences
// If there are deleted slots use them, otherwise
// create a new slot
// returns the index of the new slot
unsigned int
VSFLFont::genSentence() {
	unsigned int index;
	VSFLSentence aSentence;

	// Are there deleted slots?
	if (mDeletedSentences.size()) {
		// use the last deleted slot
		index = mDeletedSentences[mDeletedSentences.size()-1];
		// remove the slot from the deleted list
		mDeletedSentences.pop_back();
	}
	// if not create a new slot
	else {
		index = mSentences.size();
		// add a slot
		mSentences.push_back(aSentence);
	}
	// return the index of the slot
	return index;
}

// Delete a Sentence
void
VSFLFont::deleteSentence(unsigned int index) {
	// if the index refers to a valid slot
	// i.e. the slot is within range and it has a sentence
	if (index < mSentences.size() && mSentences[index].getVAO()) {
		// clear deletes the VAO and buffers
		mSentences[index].clear();
		// add the index of the deleted slot to the list
		mDeletedSentences.push_back(index);
	}
}

// Send to VSFLFont the attributes of the Vertex Coordinates, and
// texture coordinate attributes.
void
VSFLFont::setShader(SimpleTextureShader *sh) {
	VSFLFont::sShader = sh;
}

// A font is specified by two files: a TGA file with the rendered
// chars for the font, and a XML file which contains global info
// about the font and the texture coordinates and width of each char
// The parameter fontName is the filename without extension.
// It is assumed that the files are "fontName.xml" and "fontName.tga"
bool
VSFLFont::loadFont(const std::string &fontName, bool fixedSize) {
	mFixedSize = fixedSize;
	std::string s = fontName + ".bmp";

	auto image = loadBMP(s.c_str());
	if (!image)
		return false;

	glDeleteTextures(1, &mFontTex);

	glGenTextures(1,&mFontTex);
	glBindTexture(GL_TEXTURE_2D, mFontTex);
	glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_S,		static_cast<int>(GL_REPEAT));
	glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_WRAP_T,		static_cast<int>(GL_REPEAT));
	glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MAG_FILTER,   static_cast<int>(GL_LINEAR));
	glTexParameteri(GL_TEXTURE_2D,	GL_TEXTURE_MIN_FILTER,   static_cast<int>(GL_LINEAR));
	glTexImage2D(GL_TEXTURE_2D, 0, static_cast<int>(GL_RGBA), image->width, image->height, 0,
	             image->fFormat, GL_UNSIGNED_BYTE, image->pixels.get());

	glBindTexture(GL_TEXTURE_2D,0);

	s = fontName + ".xml";
	TiXmlDocument doc(s.c_str());
	bool loadOK = doc.LoadFile();

	if (!loadOK)
		return false;

	TiXmlHandle hDoc(&doc);
	TiXmlHandle hRoot(0);
	TiXmlElement *pElem;

	pElem = hDoc.FirstChildElement().Element();
	if (0 == pElem)
		return false;

	hRoot = TiXmlHandle(pElem);

	pElem->QueryIntAttribute("numchars",&mNumChars);

	if (mNumChars == 0)
		return false;

	hRoot = hRoot.FirstChild("characters");
	pElem = hRoot.FirstChild("chardata").Element();
	if (pElem)
		pElem->QueryIntAttribute("hgt",&mHeight);
	VSFLChar aChar;
	int charCode;
	for(; 0 != pElem; pElem = pElem->NextSiblingElement()) {

		pElem->QueryIntAttribute("char",&charCode);
		pElem->QueryIntAttribute("wid",&(aChar.width));
		pElem->QueryFloatAttribute("X1", &(aChar.x1));
		pElem->QueryFloatAttribute("X2", &(aChar.x2));
		pElem->QueryFloatAttribute("Y1", &(aChar.y1));
		pElem->QueryFloatAttribute("Y2", &(aChar.y2));
		pElem->QueryIntAttribute("A", &(aChar.A));
		pElem->QueryIntAttribute("C", &(aChar.C));
		mChars[(unsigned char)charCode] = aChar;
	}
	return true;
}

// Matrix and OpenGL settings for rendering
void
VSFLFont::prepareRender() {
	// get previous depth test setting
	glGetIntegerv(GL_DEPTH_TEST, (GLint *)&mPrevDepth);
	// disable depth testing
	glDisable(GL_DEPTH_TEST);

	// get previous blend settings
	glGetIntegerv(GL_BLEND, (GLint *)&mPrevBlend);
	glGetIntegerv(GL_BLEND_DST, (GLint *)&mPrevBlendDst);
	glGetIntegerv(GL_BLEND_SRC, (GLint *)&mPrevBlendSrc);
	// set blend for transparency
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void
VSFLFont::restoreRender() {
	// restore previous depth test settings
	if (mPrevDepth != GL_FALSE)
		glEnable(GL_DEPTH_TEST);

	// restore previous blend settings
	if (mPrevBlend != GL_FALSE)
		glDisable(GL_BLEND);

	glBlendFunc(mPrevBlendSrc, mPrevBlendDst);
}

int VSFLFont::numberOfPixels(const std::string &sentence) {
	int hDisp = 0.0f;
	int size = sentence.length();
	int count;
	for (count = 0; count < size; count++) {
		// get char at position count
		char c = sentence[count];
		// if char exists in the font definition
		if (mChars.count(c)) {
			if (mFixedSize)
				hDisp += mChars[c].C + mChars[c].A;
			else
				hDisp += mChars[c].C;
		}
	}
	return hDisp;
}

unsigned VSFLFont::numberOfCharacters(const std::string &sentence, float width) {
	float hDisp = 0.0f;
	unsigned size = sentence.length();
	unsigned count;
	for (count = 0; count < size; count++) {
		// get char at position count
		char c = sentence[count];
		// if char exists in the font definition
		if (mChars.count(c)) {
			if (mFixedSize)
				hDisp += mChars[c].C + mChars[c].A;
			else
				hDisp += mChars[c].C;
		}
		if (hDisp > width)
			break; // This character was past the limit and will not fit
	}
	return count;
}

void
VSFLFont::prepareSentence(unsigned int index, const std::string &sentence) {
	float *positions, *texCoords;
	float hDisp = 0.0f, vDisp = 0.0f;
	GLuint vao, buffer[2];

	// if index is not within range
	// this should never happen if using genSentence
	if (index >= mSentences.size())
		return;

	// clear previous sentence data if reusing
	mSentences[index].clear();

	// allocate temporary arrays for vertex and texture coordinates
	int size = sentence.length();
	positions = (float *)malloc(sizeof(float) * size * 6 * 3);
	texCoords = (float *)malloc(sizeof(float) * size * 6 * 2);

	int i = 0;
	for (int count = 0; count < size; count++) {

		// get char at position count
		char c = sentence[count];
		// if char exists in the font definition
		if (mChars.count(c)) {
			positions[18 * i + 0] = hDisp;
			positions[18 * i + 1] = vDisp + mHeight;
			positions[18 * i + 2] = 0.0f;

			positions[18 * i + 3] = hDisp + mChars[c].width;
			positions[18 * i + 4] = vDisp + 0.0f;
			positions[18 * i + 5] = 0.0f;

			positions[18 * i + 6] = hDisp;
			positions[18 * i + 7] = vDisp + 0.0f;
			positions[18 * i + 8] = 0.0f;

			positions[18 * i + 9] = hDisp + mChars[c].width;
			positions[18 * i + 10] = vDisp + 0.0f;
			positions[18 * i + 11] = 0.0f;

			positions[18 * i + 12] = hDisp;
			positions[18 * i + 13] = vDisp + mHeight;
			positions[18 * i + 14] = 0.0f;

			positions[18 * i + 15] = hDisp + mChars[c].width;
			positions[18 * i + 16] = vDisp + mHeight;
			positions[18 * i + 17] = 0.0f;

			texCoords[12 * i + 0] = mChars[c].x1;
			texCoords[12 * i + 1] = 1-mChars[c].y2;

			texCoords[12 * i + 2] = mChars[c].x2;
			texCoords[12 * i + 3] = 1-mChars[c].y1;

			texCoords[12 * i + 4] = mChars[c].x1;
			texCoords[12 * i + 5] = 1-mChars[c].y1;

			texCoords[12 * i + 6] = mChars[c].x2;
			texCoords[12 * i + 7] = 1-mChars[c].y1;

			texCoords[12 * i + 8] = mChars[c].x1;
			texCoords[12 * i + 9] = 1-mChars[c].y2;

			texCoords[12 * i + 10] = mChars[c].x2;
			texCoords[12 * i + 11] = 1-mChars[c].y2;

			if (mFixedSize)
				hDisp += mChars[c].C + mChars[c].A;
			else
				hDisp += mChars[c].C;
			i++;
		}
		// newline
		else if (c == '\n') {
			vDisp += mHeight/2; // TODO: Fonts are too big, I don't know why.
			hDisp = 0.0f;
		}
	}
	// real number of chars (excluding '\n')
	size = i;

	// create VAO
	glGenVertexArrays(1,&vao);
	glBindVertexArray(vao);
	VSFLFont::sShader->EnableVertexAttribArray();
	// create vertex buffers
	glGenBuffers(2,buffer);

	// positions
	glBindBuffer(GL_ARRAY_BUFFER, buffer[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * size * 6 * 3, positions,GL_STATIC_DRAW);
	VSFLFont::sShader->VertexAttribPointer(GL_FLOAT, 3, 0, 0);

	// texCoords
	glBindBuffer(GL_ARRAY_BUFFER, buffer[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * size * 6 * 2, texCoords,GL_STATIC_DRAW);

	VSFLFont::sShader->TextureAttribPointer(GL_FLOAT, 0, 0);
	glBindVertexArray(0);
	// glBindBuffer(GL_ARRAY_BUFFER, 0);  // Deprecated

	// init the sentence
	mSentences[index].initSentence(vao, buffer,size);

	// delete temporary arrays
	free(positions);
	free(texCoords);

}

// Render a previously prepared sentence.
void
VSFLFont::renderSentence(unsigned int index, const glm::vec3 &offs, float alpha) {
	prepareRender();

	glBindTexture(GL_TEXTURE_2D, mFontTex);
	glBindVertexArray(mSentences[index].getVAO());
	if (offs.x != 0 || offs.y != 0 || offs.z != 0)
		sShader->SetColorOffset(offs);
	if (alpha != 1.0f)
		sShader->ForceTransparent(alpha);
	glDrawArrays(GL_TRIANGLES, 0, mSentences[index].getSize()*6);
	if (alpha != 1.0f)
		sShader->ForceTransparent(1.0f); // Restore default
	if (offs.x != 0 || offs.y != 0 || offs.z != 0)
		sShader->SetColorOffset(glm::vec3(0,0,0));
	gNumDraw++;
	glBindVertexArray(0);
	// glBindTexture(GL_TEXTURE_2D,0);  // Deprecated

	restoreRender();
}

/* This is a shortcut to easily render a string once */
void
VSFLFont::renderAndDiscard(const std::string &sentence, const glm::vec3 &offs, float alpha) {
	unsigned int s = genSentence();
	prepareSentence(s, sentence);
	renderSentence(s, offs, alpha);
	deleteSentence(s);
}

/* ---------------------------------------------------------------------------

	                        VSFLSentence (inner class)

----------------------------------------------------------------------------*/

// Init mVAO and mSize
VSFLFont::VSFLSentence::VSFLSentence() {
	mVAO = 0;
	mSize = 0;
}


VSFLFont::VSFLSentence::~VSFLSentence() {
}


void
VSFLFont::VSFLSentence::clear() {
	if (mSize > 0) {
		if (glDeleteVertexArrays != 0)
			glDeleteVertexArrays(1, &mVAO);
		glDeleteBuffers(2, mBuffers);
		mVAO = 0;
		mSize = 0;
	}
}


void
VSFLFont::VSFLSentence::initSentence(GLuint vao, GLuint *buffers, int size) {
	mVAO = vao;
	mSize = size;
	mBuffers[0] = buffers[0];
	mBuffers[1] = buffers[1];
}


GLuint
VSFLFont::VSFLSentence::getVAO() {
	return mVAO;
}


int
VSFLFont::VSFLSentence::getSize() {
	return mSize;
}


GLuint
VSFLFont::VSFLSentence::getVertexBuffer() {
	return mBuffers[0];
}


GLuint
VSFLFont::VSFLSentence::getTexCoordBuffer() {
	return mBuffers[1];
}
