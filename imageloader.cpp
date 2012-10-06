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

#include <fstream>
#include <stdlib.h>
#include <GL/glew.h>
#include <math.h>

#include "imageloader.h"
#include "ui/Error.h"
#include "assert.h"

using namespace std;

Image::Image(unique_ptr<unsigned char[]> ps, int w, int h, GLenum format) : pixels(std::move(ps)), width(w), height(h), fFormat(format) {

}

//Converts a four-character array to an integer, using little-endian form
int toInt(const char* bytes) {
	return (int)(((unsigned char)bytes[3] << 24) |
	             ((unsigned char)bytes[2] << 16) |
	             ((unsigned char)bytes[1] << 8) |
	             (unsigned char)bytes[0]);
}

//Converts a two-character array to a short, using little-endian form
short toShort(const char* bytes) {
	return (short)(((unsigned char)bytes[1] << 8) |
	               (unsigned char)bytes[0]);
}

//Reads the next four bytes as an integer, using little-endian form
int readInt(ifstream &input) {
	char buffer[4];
	input.read(buffer, 4);
	return toInt(buffer);
}

//Reads the next two bytes as a short, using little-endian form
short readShort(ifstream &input) {
	char buffer[2];
	input.read(buffer, 2);
	return toShort(buffer);
}

shared_ptr<Image> loadBMP(const char* filename, bool booleanAlpha) {
	ifstream input;

	input.open(filename, ifstream::binary);
	if (input.fail())
		ErrorDialog("Failed to load texture file %s", filename);
	char buffer[2];
	input.read(buffer, 2);
	ASSERT((buffer[0] == 'B' && buffer[1] == 'M') || !"Not a bitmap file");
	input.ignore(8);
	int dataOffset = readInt(input);

	//Read the header
	int headerSize = readInt(input);
	int width = 0;
	int height = 0;
	int colorSize = 24;
	switch(headerSize) {
	case 40:
		//V3
		width = readInt(input);
		height = readInt(input);
		input.ignore(2);
		colorSize = readShort(input);
		// if (colorSize == 32) printf("%s\n", filename);
		ASSERT(colorSize == 24 || colorSize == 32 || !"loadBMP: Image is not 24 or 32 bits per pixel");
		ASSERT(readShort(input) == 0 || !"loadBMP: Image is compressed");
		break;
	case 12:
		//OS/2 V1
		width = readShort(input);
		height = readShort(input);
		input.ignore(2);
		colorSize = readShort(input);
		ASSERT(colorSize == 24 || colorSize == 32 || !"loadBMP: Image is not 24 bits per pixel");
		break;
	case 64:
		//OS/2 V2
		ASSERT(!"loadBMP: Can't load OS/2 V2 bitmaps");
		break;
	case 108:
		//Windows V4
		ASSERT(!"loadBMP: Can't load Windows V4 bitmaps");
		break;
	case 124:
		//Windows V5
		ASSERT(!"loadBMP: Can't load Windows V5 bitmaps");
		break;
	default:
		ASSERT(!"loadBMP: Unknown bitmap format");
	}
	int colorDepth = colorSize/8;
	GLenum format = GL_BGR;
	if (colorDepth == 4)
		format = GL_BGRA;

	//Read the data
	int bytesPerRow = ((width * colorDepth + 3) / 4) * 4;
	int size = bytesPerRow * height;
	unique_ptr<unsigned char[]> pixels(new unsigned char[size]);
	input.seekg(dataOffset, ios_base::beg);
	input.read((char *)(pixels.get()), size);
	ASSERT(input.gcount() == size);

	//Get the data into the right format
	unique_ptr<unsigned char[]> pixels2(new unsigned char[width * height * colorDepth]);
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			for(int c = 0; c < colorDepth; c++) {
				pixels2[colorDepth * (width * y + x) + c] =
				    pixels[bytesPerRow * y + colorDepth * x + c];
			}
			if (booleanAlpha && colorDepth == 4) {
				unsigned alpha = pixels[bytesPerRow * y + colorDepth * x + 3];
				if (alpha < 128)
					alpha = 0;
				else
					alpha = 255;
				pixels2[colorDepth * (width * y + x) + 3] = alpha;
			}
		}
	}

	if (colorDepth == 4) {
		// Apply premultiplied alpha
		for(int y = 0; y < height; y++) {
			for(int x = 0; x < width; x++) {
				float alpha = pixels2[colorDepth * (width * y + x) + 3] / 255.0f;
				for(int c = 0; c < colorDepth-1; c++) {
					pixels2[colorDepth * (width * y + x) + c] *= alpha;
				}
			}
		}
	}

	input.close();
	return std::make_shared<Image>(std::move(pixels2), width, height, format);
}

shared_ptr<Image> Image::MipMapNextLevel(void) {
	int colorDepth = 4;
	if (fFormat == GL_BGR || fFormat == GL_RGB)
		colorDepth = 3;
	int newWidth = width/2, newHeight = height/2;
	ASSERT(newWidth*2 == width && newHeight*2 == height);
	unique_ptr<unsigned char[]> newPixels(new unsigned char[newWidth * newHeight * colorDepth]);
	for(int y = 0; y < newHeight; y++) {
		for(int x = 0; x < newWidth; x++) {
			// Compute the sumamry of each color channel, as well as the number of pixels with alpha==1
			int G = 0, R = 0, B = 0, num = 0;
			for (int sx = x*2; sx < x*2+2; sx++) {
				for (int sy = y*2; sy < y*2+2; sy++) {
					if (colorDepth == 3 || pixels[colorDepth * (width * sy + sx) + 3] == 255) {
						// This pixel was not transparent
						num++;
						B += pixels[colorDepth * (width * sy + sx) + 0];
						G += pixels[colorDepth * (width * sy + sx) + 1];
						R += pixels[colorDepth * (width * sy + sx) + 2];
					}
				}
			}
			bool makeTransparent = false;
			if (num < 2)
				makeTransparent = true;
			// If 2 texels (50%) are transparent, then use a random
			if (num == 2 && ((x+y)&1) == 0)
				makeTransparent = true;
			if (makeTransparent) {
				// All pixels were transparent. Use black background.
				newPixels[colorDepth * (newWidth * y + x) + 0] = 0;
				newPixels[colorDepth * (newWidth * y + x) + 1] = 0;
				newPixels[colorDepth * (newWidth * y + x) + 2] = 0;
				newPixels[colorDepth * (newWidth * y + x) + 3] = 0;
			} else {
				B /= num; G /= num; R /= num;
				newPixels[colorDepth * (newWidth * y + x) + 0] = B;
				newPixels[colorDepth * (newWidth * y + x) + 1] = G;
				newPixels[colorDepth * (newWidth * y + x) + 2] = R;
				if (colorDepth == 4)
					newPixels[colorDepth * (newWidth * y + x) + 3] = 255;
			}
		}
	}
	return std::make_shared<Image>(std::move(newPixels), newWidth, newHeight, fFormat);
}

void Image::MakeLinear() {
	int colorDepth = 4;
	if (fFormat == GL_BGR || fFormat == GL_RGB)
		colorDepth = 3;
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			for(int c = 0; c < 3; c++) {
				double color = pixels[colorDepth * (width * y + x) + c]/255.0;
				double newColor = pow(color, 2.2);
				ASSERT(color >= 0.0 && color <= 1.0);
				pixels[colorDepth * (width * y + x) + c] = newColor*255;
			}
		}
	}
}

void Image::MakeNonLinear() {
	int colorDepth = 4;
	if (fFormat == GL_BGR || fFormat == GL_RGB)
		colorDepth = 3;
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			for(int c = 0; c < 3; c++) {
				double color = pixels[colorDepth * (width * y + x) + c]/255.0;
				double newColor = pow(color, 0.45);
				ASSERT(color >= 0.0 && color <= 1.0);
				pixels[colorDepth * (width * y + x) + c] = newColor*255;
			}
		}
	}
}
