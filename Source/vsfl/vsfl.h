/** ----------------------------------------------------------
 * \class VSFLFont
 *
 * Lighthouse3D
 *
 * VSFL - Very Simple Font Library
 *
 * Full documentation at
 * http://www.lighthouse3d.com/very-simple-libs
 *
 * This class aims at displaying text strings using
 * bitmap fonts for core versions of OpenGL.
 *
 * This lib requires:
 *
 * VSML (http://www.lighthouse3d.com/very-simple-libs)
 *
 * and the following third party libs:
 *
 * GLEW (http://glew.sourceforge.net/),
 * TinyXML (http://sourceforge.net/projects/tinyxml/), and
 * DevIL (http://openil.sourceforge.net/)
 *
 * Note: The font files (xml and tga) are produced by an
 * old software called FontStudio by Michael Pote from Nitrogen
---------------------------------------------------------------*/

#pragma once

#include <map>
#include <vector>
#include <string>
#include <glbinding/gl/types.h>
#include <glm/glm.hpp>

using gl::GLuint;
using gl::GLint;
using gl::GLenum;
using gl::GLboolean;

class SimpleTextureShader;

class VSFLFont {
public:
	VSFLFont();
	~VSFLFont();

	/** Loads the font data. It assumes that there is a file
	  * fontName.xml with the font data and a fontName.tga with the
	  * font image
	  * \param fontName the file name without extension
	  * \param fixedSize indicates if the font is to be displayed
	  * with all chars having the same length
	  * \return true if successfull, false otherwise
	*/
	bool loadFont(const std::string &fontName, bool fixedSize = false);

	/** Set attribute locations for vertices and texture coords
	  * \param vertexLoc the vertex coordinates attribute location
	  * \param texCoordLoc the texture coordinates attribute location
	*/
	void setShader(SimpleTextureShader *);

	/** Allocate a slot for a sentence
	  * \return a slot index
	*/
	unsigned int genSentence();

	/** Clear a slot
	  * \param index a previously allocated slot index
	*/
	void deleteSentence(unsigned int index);

	/** Compute the number of characters that will fit
	 *  \param sentence the string to test
	 *  \param width the width, in pixels, of the screen that is allowed to use
	 */
	unsigned numberOfCharacters(const std::string &sentence, float width);

	/** Compute the number of pixels needed
	 *  \param sentence the string to test
	 */
	int numberOfPixels(const std::string &sentence);

	/** Get the height of the font in pixels
	 */
	int height(void) const { return mHeight; }

	/** Prepares a sentence in a given slot index
	  * \param index the index where the sentence will be stored
	  * \param sentence the string to be displayed
	*/
	void prepareSentence(unsigned int index, const std::string &sentence);

	/** Render a sentence. Note that screen
	  * coordinates (x,y), have (0,0)
	  * as the top left of the screen
	  * \param x the x screen cordinate
	  * \param y the y screen coordinate
	  * \param index a previously allocated slot index
	*/
	void renderSentence(unsigned int index, const glm::vec3 &offs = glm::vec3(0,0,0), float alpha = 1.0f);

	/** Shortcut to render a string with a single function call
	  * This is equivalent to calling genSentence,
	  * prepareSentence, renderSentence, and finally
	  * deleteSentence
	*/
	void renderAndDiscard(const std::string &sentence, const glm::vec3 &offs = glm::vec3(0,0,0), float alpha = 1.0f);

private:

	/// Fixed size?
	bool mFixedSize;

	/** This class contains information for individual chars
	  * x1,x2,y1,y2 are the texture coordinates, width, A, and C
	  * the remaining char properties
	*/
	class VSFLChar {
	public:
		// TexCoords
		float x1,x2,y1,y2;
		/// Char width
		int width;
		int A,C;

	};

	/** \brief Stores the information to render a string
	  *
	  * A sentence stores the VAO index required to render the string
	  * and free the resources both the VAO and the attribute buffers).
	*/
	class VSFLSentence {

	private:
		/// VAO index
		GLuint mVAO;
		/// Vertex and texcoord buffers
		GLuint mBuffers[2];
		/// String size
		int mSize;

	public:
		VSFLSentence();
		~VSFLSentence();
		/// sets the instance variables
		void initSentence(GLuint vao, GLuint *buffers, int size);
		/// deletes the VAO and the buffers
		void clear();

		/// returns the VAO index
		GLuint getVAO();
		/// returns the vertex buffer index
		GLuint getVertexBuffer();
		/// returns the texCoord buffer index
		GLuint getTexCoordBuffer();
		/// returns the
		int getSize();
	};

	/** Font char data, results from parsing
	 * the XML file for the font */
	std::map<char, VSFLChar> mChars;
	/// font char height
	int mHeight;
	/// total chars parsed
	int mNumChars;

	// Sentence managment
	/// Contains all the sentences
	std::vector<VSFLSentence> mSentences;
	/// contains a list of the indexes of free slots
	std::vector<unsigned int> mDeletedSentences;

	// OpenGL Settings
	/// Font Texture index
	GLuint mFontTex;
	/// Previous DEPTH_TEST settings
	GLboolean mPrevDepth;
	/// Previous BLEND settings
	GLboolean mPrevBlend;
	/// Previous BLEND_DST settings
	GLenum mPrevBlendDst;
	/// Previous BLEND_SRC settings
	GLenum mPrevBlendSrc;

	/** Prepare matrices for rendering
	*/
	void prepareRender();

	/// Restore the original matrices prior to prepareRender
	void restoreRender();

	static SimpleTextureShader *sShader;
};
