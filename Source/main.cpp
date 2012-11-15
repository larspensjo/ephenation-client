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

#if defined(_WIN32)
#include <windows.h>
#include <Shlobj.h>
#include <direct.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <thread>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glfw.h>
#include <iostream>
#include <fstream>
#include <string>
#include <getopt.h>

#include <glm/glm.hpp>
#include "connection.h"
#include "modes.h"
#include "textures.h"
#include "shapes/Tree.h"
#include "shapes/Cube.h"
#include "shapes/quadstage1.h"
#include "gamedialog.h"
#include "DrawText.h"
#include "shaders/ChunkShader.h"
#include "primitives.h"
#include "client_prot.h"
#include "player.h"
#include "Options.h"
#include "render.h"
#include "ui/Error.h"
#include "SoundControl.h"
#include "TSExec.h"
#include "chunkcache.h"
#include "ChunkProcess.h"
#include "BlenderModel.h"
#include "monsters.h"
#include "uniformbuffer.h"
#include "billboard.h"
#include "worsttime.h"
#include "contrib/glsw.h"

#ifndef GL_VERSION_3_2
#define GL_CONTEXT_CORE_PROFILE_BIT       0x00000001
#define GL_CONTEXT_COMPATIBILITY_PROFILE_BIT 0x00000002
#define GL_CONTEXT_PROFILE_MASK           0x9126
#define GL_NUM_EXTENSIONS                 0x821D
#define GL_CONTEXT_FLAGS                  0x821E
#define GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT 0x0001
#endif

#ifndef GL_VERSION_2_0
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C
#endif

using namespace std;

static const char* get_profile_name(GLint mask) {
	if (mask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
		return "compatibility";
	if (mask & GL_CONTEXT_CORE_PROFILE_BIT)
		return "core";

	return "unknown";
}

// Used for NVIDIA
#define GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#define GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#define GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
#define GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B

// Used for ATI
#define VBO_FREE_MEMORY_ATI                           0x87FB
#define TEXTURE_FREE_MEMORY_ATI                       0x87FC
#define RENDERBUFFER_FREE_MEMORY_ATI                  0x87FD

static void dumpInfo(int major, int minor, int revision) {
	int glfwMajor=-1, glfwMinor=-1, glfwRev=-1;
	glfwGetVersion(&glfwMajor, &glfwMinor, &glfwRev);
	printf ("GLFW Version: %d.%d.%d\n", glfwMajor, glfwMinor, glfwRev);
	printf ("Vendor: %s\n", glGetString (GL_VENDOR));
	printf ("Renderer: %s\n", glGetString (GL_RENDERER));
	printf ("Version: %s\n", glGetString (GL_VERSION));
	printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));
	printf("OpenGL context version parsed by GLFW: %u.%u.%u\n", major, minor, revision);
	GLint flags, mask;

	if (major >= 3) {
		glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		printf("OpenGL context flags:");

		if (flags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT)
			puts(" forward-compatible");
		else
			puts(" none");
	}

	if (major > 3 || (major == 3 && minor >= 2)) {
		glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &mask);
		printf("OpenGL profile mask: 0x%08x (%s)\n", mask, get_profile_name(mask));
	}

	// This works for NVIDIA
	GLint par = -1;
	glGetIntegerv(GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &par);
	if (glGetError() == GL_NO_ERROR)
		printf("GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX %d\n", par);
	glGetIntegerv(GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &par);
	if (glGetError() == GL_NO_ERROR)
		printf("GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX %d\n", par);

	// This works for ATI
	GLint parATI[4];
	glGetIntegerv(VBO_FREE_MEMORY_ATI, parATI);
	if (glGetError() == GL_NO_ERROR)
		printf("VBO_FREE_MEMORY_ATI total %d, largest block %d, total aux %d, largest aux block %d\n", parATI[0], parATI[1], parATI[2], parATI[3]);
}

void dumpGraphicsMemoryStats(void) {
	if (strncmp((const char *)glGetString (GL_RENDERER), "NVS", 3) == 0) {
		// This works for NVIDIA
		GLint par = -1;
		glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &par);
		if (glGetError() == GL_NO_ERROR)
			printf("GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX %d\n", par);
		glGetIntegerv(GPU_MEMORY_INFO_EVICTION_COUNT_NVX, &par);
		if (glGetError() == GL_NO_ERROR)
			printf("GPU_MEMORY_INFO_EVICTION_COUNT_NVX %d\n", par);
		glGetIntegerv(GPU_MEMORY_INFO_EVICTED_MEMORY_NVX, &par);
		if (glGetError() == GL_NO_ERROR)
			printf("GPU_MEMORY_INFO_EVICTED_MEMORY_NVX %d\n", par);
	}

	if (strncmp((const char *)glGetString (GL_RENDERER), "AMD", 3) == 0) {
		GLint parATI[4];
		glGetIntegerv(VBO_FREE_MEMORY_ATI, parATI);
		if (glGetError() == GL_NO_ERROR)
			printf("VBO_FREE_MEMORY_ATI total %d, largest block %d, total aux %d, largest aux block %d\n", parATI[0], parATI[1], parATI[2], parATI[3]);
	}
}

void APIENTRY DebugFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam) {
	std::string srcName;
	switch(source) {
	case GL_DEBUG_SOURCE_API_ARB: srcName = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: srcName = "Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: srcName = "Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: srcName = "Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION_ARB: srcName = "Application"; break;
	case GL_DEBUG_SOURCE_OTHER_ARB: srcName = "Other"; break;
	}

	std::string errorType;
	switch(type) {
	case GL_DEBUG_TYPE_ERROR_ARB: errorType = "Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: errorType = "Deprecated Functionality"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: errorType = "Undefined Behavior"; break;
	case GL_DEBUG_TYPE_PORTABILITY_ARB: errorType = "Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE_ARB: errorType = "Performance"; break;
	case GL_DEBUG_TYPE_OTHER_ARB: errorType = "Other"; break;
	}

	std::string typeSeverity;
	switch(severity) {
	case GL_DEBUG_SEVERITY_HIGH_ARB: typeSeverity = "High"; break;
	case GL_DEBUG_SEVERITY_MEDIUM_ARB: typeSeverity = "Medium"; break;
	case GL_DEBUG_SEVERITY_LOW_ARB: typeSeverity = "Low"; break;
	}

	printf("%s from %s,\t%s priority\nMessage: %s\n",
	       errorType.c_str(), srcName.c_str(), typeSeverity.c_str(), message);
	if (severity == GL_DEBUG_SEVERITY_HIGH_ARB && !gIgnoreOpenGLErrors)
		printf("%s from %s,\t%s priority\nMessage: %s\n", errorType.c_str(), srcName.c_str(), typeSeverity.c_str(), message); // Can't use ErrorDialog() here.
}

void APIENTRY DebugFuncAMD(GLuint id, GLenum category, GLenum severity, GLsizei length, const GLchar* message, GLvoid* userParam) {
	std::string typeSeverity;
	switch(severity) {
	case GL_DEBUG_SEVERITY_HIGH_ARB: typeSeverity = "High"; break;
	case GL_DEBUG_SEVERITY_MEDIUM_ARB: typeSeverity = "Medium"; break;
	case GL_DEBUG_SEVERITY_LOW_ARB: typeSeverity = "Low"; break;
	}

	std::string typeCategory;
	switch(category) {
	case GL_DEBUG_CATEGORY_API_ERROR_AMD: typeCategory = "API"; break;
	case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD: typeCategory = "Window system"; break;
	case GL_DEBUG_CATEGORY_DEPRECATION_AMD: typeCategory = "Deprecation"; break;
	case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD: typeCategory = "Undefined behaviour"; break;
	case GL_DEBUG_CATEGORY_PERFORMANCE_AMD: typeCategory = "Performance"; break;
	case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD: typeCategory = "Shader compiler"; break;
	case GL_DEBUG_CATEGORY_APPLICATION_AMD: typeCategory = "Application"; break;
	case GL_DEBUG_CATEGORY_OTHER_AMD: typeCategory = "Other"; break;
	}

	printf("%s,\t%s priority\nMessage: %s\n",
	       typeCategory.c_str(), typeSeverity.c_str(), message);
	if (severity == GL_DEBUG_SEVERITY_HIGH_ARB && !gIgnoreOpenGLErrors)
		printf("%s,\t%s priority\nMessage: %s\n", typeCategory.c_str(), typeSeverity.c_str(), message); // Can't use ErrorDialog() here.
}

// Used for debugging, where it is difficult when there are many threads trigging a break point.
static int sSingleThread = 0;

// Log in automatically as a test user
static int sTestUser = 0;

static struct option long_options[] = {
	/* These options set a flag. */
	{"verbose", no_argument,       &gVerbose, 1},
	{"debug",   no_argument,       &gDebugOpenGL, 1},
	{"singlethread", no_argument,  &sSingleThread, 1},
	{"testuser", no_argument,      &sTestUser, 1},
	{0, 0, 0, 0}
};

int main(int argc, char** argv) {
	if (!glfwInit()) {
		ErrorDialog("Failed to initialize GLFW\n");
		exit(EXIT_FAILURE);
	}

	gCurrentFrameTime = 0.0;

	string optionsFilename = "ephenation.ini";
	string dataDir; // The directory where the client can save data
#ifdef unix
	const char *home = getenv("HOME");
	// Save Linux Path
	dataDir = string(home) + "/.ephenation";
	const char *ephenationPath = dataDir.c_str();
	struct stat st;
	if (stat(ephenationPath,&st) != 0) {
		mkdir(ephenationPath, 0777);
	}
	if (home)
		optionsFilename = dataDir + "/" + optionsFilename;
	else
		optionsFilename = ".ephenation/ephenation.ini"; // Fallback
#endif
#ifdef WIN32
	TCHAR home[MAX_PATH];
	HRESULT res = SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, 0, 0, home);
	if (res == S_OK) {
		dataDir = string(home) + "\\ephenation";
		const char *ephenationPath = dataDir.c_str();
		struct _stat st;
		if (_stat(ephenationPath,&st) != 0) {
			res = _mkdir(ephenationPath);
		}
		optionsFilename = dataDir + "\\" + optionsFilename; // Fallback
	}
#endif

	const char *host = "server1.ephenation.net";
	int port = 57862;
	/* getopt_long stores the option index here. */
	int option_index = 0;

	while(1) {
		int c = getopt_long (argc, argv, "", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;
	}
	if (optind < argc)
		host = argv[optind];
#ifdef WIN32
	if (!gDebugOpenGL && !gVerbose)
		FreeConsole();
	const char *cacheName = "\\cache";
#else
	const char *cacheName = "/cache";
#endif

	char *cachePath = new char[strlen(dataDir.c_str()) + strlen(cacheName) + strlen(host) + 2];
	strcpy(cachePath, dataDir.c_str());
#ifdef WIN32
	strcat(cachePath, "\\cache");
	strcat(cachePath, host);
	strcat(cachePath, "\\");
#else
	strcat(cachePath, "/cache");
	strcat(cachePath, host);
	strcat(cachePath, "/");
#endif

	ChunkCache::fgChunkCache.SetCacheDir(cachePath);

	//printf("Game Path: %s\n", dataDir);

	gOptions.Init(optionsFilename); // This one should come early, as it is used to initalize things.
	unsigned maxThreads = gOptions.fNumThreads;
	if (sSingleThread) {
		maxThreads = 1; // Override this number
		std::cout << "Limit to minimum number of threads" << std::endl;
	}
	glswInit();
	glswSetPath("shaders/", ".glsl");
	ConnectToServer(host, port);
	gSoundControl.Init();
	TSExec::gTSExec.Init(); // This must be called after initiating gSoundControl.

	if (gDebugOpenGL)
		printf("Number of threads: %d\n", maxThreads);
	int numChunkProc = maxThreads - 1;
	if (numChunkProc <= 0)
		numChunkProc = 1;
	gChunkProcess.Init(numChunkProc);

	//glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	//glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, gDebugOpenGL);
	// glfwOpenWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Need to specify at least version 3.3 for this to work.

	if (!glfwOpenWindow(gOptions.fWindowWidth, gOptions.fWindowHeight, 0, 0, 0, 0, 16, 1, gOptions.fFullScreen ? GLFW_FULLSCREEN : GLFW_WINDOW)) {
		glfwTerminate();

		ErrorDialog("Failed to open GLFW window");
	}
	glfwSetWindowTitle("Ephenation");
	checkError("glfwSetWindowTitle");
	GLFWvidmode desktopMode;
	glfwGetDesktopMode(&desktopMode);
	if (gDebugOpenGL)
		printf("Desktop mode %d blue bits, %d green bits, %d red bits, %dx%d\n", desktopMode.BlueBits, desktopMode.GreenBits, desktopMode.RedBits, desktopMode.Width, desktopMode.Height);
	gDesktopAspectRatio = float(desktopMode.Width)/float(desktopMode.Height);

	// Initialize glew
	GLenum err=glewInit();
	checkError("glewInit");
	if(err!=GLEW_OK) {
		//problem: glewInit failed, something is seriously wrong
		printf("Fail to init glew: Error: %s\n", glewGetErrorString(err));
		checkError("glewInit");
		return -1;
	}

	// Only continue, if OpenGL of the expected version is supported.
	int major, minor, revision;
	glfwGetGLVersion(&major, &minor, &revision);
	if ((major == 3 && minor < 3) || major < 3) {
		ErrorDialog("OpenGL context version parsed by GLFW: %u.%u.%u. Version 3.3 required\n", major, minor, revision);
	}
	if (major > 3 || (major == 3 && minor > 2))
		vGL_3_3 = true;

	if (gDebugOpenGL) {
		dumpInfo(major, minor, revision); // Enable this to show some version information about the OpenGL and the graphics card.
		// dumpGraphicsMemoryStats();
		// const GLubyte* sExtensions = glGetString(GL_EXTENSIONS);
		// printf("GL extensions: %s\n", sExtensions);
		// glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		if (glDebugMessageCallbackARB != 0)
			glDebugMessageCallbackARB(DebugFunc, (void*)15);
		else if (glDebugMessageCallbackAMD != 0)
			glDebugMessageCallbackAMD(DebugFuncAMD, (void*)15);
	}

	glfwSwapInterval(gOptions.fVSYNC); // 0 means do not wait for VSYNC, which would delay the FPS sometimes.

	ComputeRelativeChunksSortedDistances();

	BlenderModel::InitModels();

	gPlayer.loginOk = true;
	gUniformBuffer.Init();
	gDrawFont.Init("textures/georgia12"); // Must be done before gGameDialog.
	GameTexture::Init();
	gGameDialog.init();
	ChunkShader *shader = ChunkShader::Make();
	Tree::InitStatic();
	gLantern.Init(shader);
	gQuadStage1.Init();
	gBillboard.Init();

	gSoundControl.RequestMusicMode(SoundControl::SMusicModeMenu);
	glEnable(GL_DEPTH_TEST); // Always enabled by default

	// Immediately clear screen, to minimize chance that something undefined is shown.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
	glfwSwapBuffers();

	// Last thing before starting the game, update the save copy of the options.
	Options::sfSave = gOptions;
	// double prevTime = 0.0;
	while(glfwGetWindowParam(GLFW_OPENED)) {
		while (ListenForServerMessages())
			continue;
		if (sTestUser && gMode.Get() == GameMode::LOGIN) {
			PerformLoginProcedure("", "", "", true);
			sTestUser = false; // Try this only once
		}
		static WorstTime tm(" Mainloop");
		tm.Start();
		gCurrentFrameTime = glfwGetTime();
		if (gShowPing && gCurrentFrameTime - gLastPing > 5.0) {
			// It was a request, so we need to send a response
			unsigned char msg[4];
			msg[0] = sizeof msg;
			msg[1] = 0;
			msg[2] = CMD_PING;
			msg[3] = 0; // 0 means request, 1 means response
			SendMsg(msg, sizeof msg);
			gLastPing = gCurrentFrameTime;
			gCurrentPing = 0.0;
			// Use a busy wait for the response, no time must be lost
			while (gCurrentPing == 0.0 && glfwGetTime() - gLastPing < 0.5)
				ListenForServerMessages();
		}

		gGameDialog.Update();
		gGameDialog.render();

		glfwSwapBuffers();

		if (gMode.Get() == GameMode::ESC)
			glfwCloseWindow();
		tm.Stop();
	}

	// The window is closed, request quit (which will save the player on the server side)
	unsigned char b[] = { 0x03, 0x00, CMD_QUIT };
	SendMsg(b, sizeof b);

	// The logout acknowledge from the server may take sime time. Take the opportunity to
	// halt the process pool.ss
	gChunkProcess.RequestTerminate();
	glswShutdown();

	double timer = glfwGetTime();
	// Wait for ack from server, but not indefinitely
	while (gMode.Get() == GameMode::GAME && glfwGetTime() - timer < 2.0) {
		glfwSleep(0.1); // Avoid a busy wait
		ListenForServerMessages(); // Wait for acknowledge
	}

	if (gMode.Get() == GameMode::GAME) {
		printf("Failed to disconnect from server\n");
	}

	// The options will be saved by the destructor.
	Options::sfSave.fViewingDistance = maxRenderDistance;
	gMode.Set(GameMode::EXIT);
	Options::sfSave.Save();
	return 0;
}
