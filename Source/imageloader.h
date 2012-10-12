/* Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* File for "Putting It All Together" lesson of the OpenGL tutorial on
 * www.videotutorialsrock.com
 */

#pragma once

#include <memory>

using std::unique_ptr;
using std::shared_ptr;

//Represents an image
class Image {
public:
	Image(unique_ptr<unsigned char[]> ps, int w, int h, GLenum format);

	/* An array of the form (R1, G1, B1, R2, G2, B2, ...) indicating the
	 * color of each pixel in image.  Color components range from 0 to 255.
	 * The array starts the bottom-left pixel, then moves right to the end
	 * of the row, then moves up to the next column, and so on.  This is the
	 * format in which OpenGL likes images.
	 */
	unique_ptr<unsigned char[]> pixels;
	int width;
	int height;
	GLenum fFormat; // Currently support is GL_RGB, GL_RGBA, GL_BGR and GL_BGRA.

	// Make a new image, which is a scaled down version. 'width' and 'height' must be a factor of 2.
	// The algorithm only accepts alpha of 0 or 1. Averaging of colors ignore pixels with alpha 0.
	shared_ptr<Image> MipMapNextLevel(void);

	// Add gama correction to make the image linear
	void MakeLinear(void);

	void MakeNonLinear(void);
};

//Reads a bitmap image from file. With 'booleanAlpha', the alpha will be set to either 0 or 255.
shared_ptr<Image> loadBMP(const char* filename, bool booleanAlpha = false);
