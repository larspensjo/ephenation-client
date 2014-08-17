// Copyright 2013 The Ephenation Authors
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

#ifdef WIN32
#include <windows.h>
#endif
#include <iostream>
#include <sstream>
#include <cstring>
#include <glbinding/gl/functions33.h>
#include <glbinding/gl/enum33.h>
#include <glbinding/gl/enum33ext.h>
// Kludge to prevent glfw from including GL/gl.h
	#define __gl_h_
	#define GLFW_NO_GLU
#include <GL/glfw.h>

#include "Debug.h"
#include "errormanager.h"
#include "connection.h"
#include "client_prot.h"
#include "modes.h"

using namespace View;
using namespace gl33;
using namespace gl33ext;

using std::endl;

ErrorManager View::gErrorManager;

ErrorManager::ErrorManager() : fInhibitServer(false), fInhibitPopup(false)
{
}

std::stringstream &ErrorManager::GetStream(bool inhibitServer, bool inhibitPopoup) {
	if (inhibitServer)
		fInhibitServer = true;
	if (inhibitPopoup)
		fInhibitPopup = true;
	gMode.Set(GameMode::ESC); // Initiate graceful shutdown
	return fStream;
}

void ErrorManager::Report() const {
	std::string str = fStream.str();
	if (str == "")
		return; // There was no error to report

	LPLOG("%s", str.c_str());
	if (!fInhibitPopup)
		this->Win32MessageBox(str);

	if (!fInhibitServer)
		this->SendServerMessage(str);

	std::cerr << str << endl;
}

void ErrorManager::Win32MessageBox(const std::string &str) const {
#ifdef WIN32
	MessageBox(0, str.c_str(), "Ephenation error", MB_OK|MB_TOPMOST|MB_ICONERROR);
#endif
}

void ErrorManager::SendServerMessage(const std::string &err) const {
	std::stringstream ss;
	ss << err << endl;
	ss << "Vendor: " << glGetString (GL_VENDOR) << endl;
	ss << "Renderer: " << glGetString (GL_RENDERER) << endl;
	ss << "Version: " << glGetString (GL_VERSION) << endl;
	ss << "GLSL: " << glGetString (GL_SHADING_LANGUAGE_VERSION) << endl;
	int major, minor, revision;
	glfwGetGLVersion(&major, &minor, &revision);
	ss << "OpenGL context version parsed by GLFW: " << major << "." << minor << "." << revision << endl;
	// This works for NVIDIA
	GLint par = -1;
	glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &par);
	if (glGetError() == gl33::GL_NO_ERROR)
		ss << "GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX " << par << endl;
	glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &par);
	if (glGetError() == GL_NO_ERROR)
		ss << "GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX " << par << endl;

	// This works for ATI
	GLint parATI[4];
	glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, parATI);
	if (glGetError() == GL_NO_ERROR)
		ss << "GL_VBO_FREE_MEMORY_ATI total " << parATI[0] << "largest block " << parATI[1] << "total aux " <<
			parATI[2] << "largest aux block " << parATI[3] << endl;

	std::string result = ss.str();
	unsigned int len = result.length() + 3;
	unsigned char buff[len];
	buff[0] = len & 0xff;
	buff[1] = len / 0x100;
	buff[2] = CMD_ERROR_REPORT;
	memcpy(buff+3, result.c_str(), len-3);
	SendMsg(buff, len);
}
