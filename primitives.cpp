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

#include <stdio.h>
#include <GL/glew.h>

#include "glm/glm.hpp"
#include "primitives.h"

// Pure debug function to dump all triangles of one block type.
void DumpTriangles(TriangleSurfacef *t, int num) {
	for (int tri=0; tri<num; tri++) {
		const char *p1 = "{{ ", *p2 = "";
		for (int vert=0; vert<3; vert++) {
			VertexDataf *v = &t[tri].v[vert];
			printf("%s{ {", p1);
			p1 = "   ";
			for (int norm=0; norm<3; norm++) {
				printf("%.1f,", v->GetNormal()[norm]);
			}
			printf("},{");
			for (int tex=0; tex<2; tex++) {
				printf("%.1f,", v->GetTexture()[tex]);
			}
			printf("},{");
			for (int coord=0; coord<3; coord++) {
				printf("%.1f,", v->GetVertex()[coord]);
			}
			if (vert == 2) p2 = " }},";
			printf("},%3d}, %s\n", v->GetIntensity(), p2);
		}
	}
}

/* report GL errors, if any, to stderr */
void checkError(const char *functionName, bool ignore) {
	if (ignore)
		return; // Default is to ignore, to save execution time
	GLenum error;
	while (( error = glGetError() ) != GL_NO_ERROR) {
		switch(error) {
		case GL_INVALID_ENUM:
			fprintf (stderr, "GL error GL_INVALID_ENUM detected in %s\n", functionName);
			break;
		case GL_INVALID_VALUE:
			fprintf (stderr, "GL error GL_INVALID_VALUE detected in %s\n", functionName);
			break;
		case GL_INVALID_OPERATION:
			fprintf (stderr, "GL error GL_INVALID_OPERATION detected in %s\n", functionName);
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			fprintf (stderr, "GL error GL_INVALID_FRAMEBUFFER_OPERATION detected in %s\n", functionName);
			break;
		case GL_OUT_OF_MEMORY:
			fprintf (stderr, "GL error GL_OUT_OF_MEMORY detected in %s\n", functionName);
			break;
		case GL_STACK_OVERFLOW:
			fprintf (stderr, "GL error GL_STACK_OVERFLOW detected in %s\n", functionName);
			break;
		case GL_STACK_UNDERFLOW:
			fprintf (stderr, "GL error GL_STACK_UNDERFLOW detected in %s\n", functionName);
			break;
		case GL_TABLE_TOO_LARGE:
			fprintf (stderr, "GL error GL_TABLE_TOO_LARGE detected in %s\n", functionName);
			break;
		default:
			fprintf (stderr, "Undefined GL error 0x%X detected in %s\n", error, functionName);
			break;
		}
#ifdef WIN32
		return; // Should continue to loop, but it doesn't work in Windows
#endif
	}
}

const char *FrameBufferError(unsigned error) {
	switch(error) {
	case GL_FRAMEBUFFER_COMPLETE: return "GL_FRAMEBUFFER_COMPLETE";
	case GL_FRAMEBUFFER_UNDEFINED: return "GL_FRAMEBUFFER_UNDEFINED";
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT";
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER";
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER";
	case GL_FRAMEBUFFER_UNSUPPORTED: return "GL_FRAMEBUFFER_UNSUPPORTED";
	case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE";
	case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS";
	default: return "FrameBufferError undefined";
	}
}

bool vGL_3_3 = false;
int gDebugOpenGL = false;
int gVerbose;
glm::vec4 gViewport; // The current viewport
double gCurrentFrameTime; // This is updated once every frame
bool gShowFramework;
double gLastPing, gCurrentPing;
bool gShowPing;
bool gAdminTP = false;
bool gIgnoreOpenGLErrors = false;
std::vector<unsigned> gDebugTextures;
bool gToggleTesting; // Used for SW development
float gDesktopAspectRatio;
