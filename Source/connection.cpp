// Copyright 2012-2013 The Ephenation Authors
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

#include <glbinding/gl/functions33.h>
#include <glbinding/gl/enum33.h>
#include <stdio.h>
#include <sys/types.h>
#include <vector>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <iostream>

#ifdef WIN32
#include <winsock2.h>
#include <memory.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <glm/glm.hpp>
#include "connection.h"
#include "parse.h"
#include "render.h"
#include "chunk.h"
#include "player.h"
#include "client_prot.h"
#include "modes.h"
#include "worsttime.h"
#include "Options.h"
#include "crypt.h"
#include "SoundControl.h"
#include "errormanager.h"

#ifdef WIN32
static SOCKET sock_fd;
#else
static int sock_fd;
#endif

int gClientAvailMajor, gClientAvailMinor;

void ConnectToServer(const char *host, int port) {
	struct hostent *server;

#ifdef WIN32 // Initialize winsockets
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	/* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		/* Tell the user that we could not find a usable */
		/* Winsock DLL.                                  */
		printf("WSAStartup failed with error: %d\n", err);
		exit(1);
	}
#endif // WIN32

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef WIN32
	bool fail = sock_fd == INVALID_SOCKET;
#else
	bool fail = sock_fd == -1;
#endif
	if (fail) {
		perror("Socket error");
		// int ret = WSAGetLastError();
		exit(0);
	}
	server = gethostbyname(host);
	if (server == NULL) {
		perror("ERROR, no such host\n");
		exit(0);
	}
	struct sockaddr_in serv_addr;
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	serv_addr.sin_port = htons(port);
	int ret = connect(sock_fd, (sockaddr*)&serv_addr, sizeof serv_addr);
	if (ret < 0) {
		auto &ss = View::gErrorManager.GetStream(true, false);
#ifdef WIN32
		auto err = WSAGetLastError();
		switch(err) {
		case WSAETIMEDOUT:
			ss << "No connection to server";
			break;
		default:
			ss << "Failed to connect to server " << err;
		}
#else
		auto err = errno;
		ss << "Failed to connect to server " << err;
#endif
	}
}

static std::vector<unsigned char> buff;

static int ReadBytes(int offset, int n) {
	static WorstTime tm("LstnSrvrMsgs");
	tm.Start();
#ifdef WIN32
	int count = recv(sock_fd, (char *)&buff[offset], n, 0);
#else
	int count = read(sock_fd, &buff[offset], n);
#endif
	tm.Stop();

	if (count == 0 || (count<0 && errno == EEXIST)) { // EEXIST can happen on windows if connection is broken
		gMode.Set(GameMode::ESC);
		count = 0;
	}
	if (count < 0) {
		if (errno != EAGAIN) {
			auto &ss = View::gErrorManager.GetStream(true, false);
#ifdef WIN32
			ss << "ListenForServerMessages1: Error " << errno << " length " << count;
#else
			ss << "ListenForServerMessages1: Error " << errno << "(" << strerror(errno) << "), length" << count;
#endif
		}
		count = 0; // This is not a fatal error, need to try again
	}
	return count;
}

// We know there is data available to read. Return true if there is a complete message available.
// Return true, and an empty message, when it is eof.
// Never do more than one read action, to make sure the read operation never blocks.
static bool ReadOneMessage(void) {
	int offset = buff.size();
	int msgLength = 3;                                // The maximum amount we try to read. No message is smaller than this.
	if (offset > 2)
		msgLength = Parseuint16(&buff[0]);            // Info about complete message length is available, try to get it all.
	buff.resize(msgLength);                           // Prepare as if we succeed to read all we want.
	int count = ReadBytes(offset, msgLength-offset);
	buff.resize(offset+count);                        // Adjust to what was actually read
	if (offset > 2 && offset+count == Parseuint16(&buff[0]))
		return true;
	if (count == 0) {
		// A zero cound indicates end of file, which means connection is down.
		buff.clear();
		return true; // An empty message is also complete, sort of
	}
	return false;
}

// Listen for a message, and parse it. Return true if there could be more to read.
bool ListenForServerMessages(void) {
	// Query the socket to find out if there is anything available to read. Never block!
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	int nfds = sock_fd+1;
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(sock_fd, &readfds);
	int cnt = select(nfds, &readfds, NULL, NULL, &timeout);
	if (cnt < 0) {
		if (errno == EINTR)
			return true;                // "Interrupted system call". Not fatal.
		auto &ss = View::gErrorManager.GetStream(true, false);
		ss << "ListenForServerMessages select() failed: errno " << errno;
	}
	if (cnt <= 0)
		return false;                   // Nothing more available for now
	if (ReadOneMessage()) {
		if (buff.size() == 0)
			return false;               // Zero size, the connection is broken.
		buff.push_back(0);              // Sometimes there is a string. Help with adding a terminating null byte. TODO: Ugly
		Parse(&buff[2], buff.size()-3); // Exclude the two initial bytes message length and the added 0-byte from the size
		buff.clear();
	}
	return true;
}

void SendMsg(unsigned char const *b, int n) {
	static WorstTime tm("SendMsg");
	tm.Start();
#ifdef WIN32
	int res = send(sock_fd, (char*)b, n, 0);
#else
	int res = write(sock_fd, b, n);
#endif
	tm.Stop();
	if (res == -1) {
		perror("write socket");
	}
}

static void LoginMessage(const char *id) {
	int len = strlen(id);
	int totLength = len+3;
	unsigned char b[totLength];
	b[0] = totLength&0xff; // LSB of message length
	b[1] = totLength>>8;   // MSB of message length
	b[2] = CMD_LOGIN;
	memcpy(b+3, id, len);
	SendMsg(b, totLength);
}

std::vector<unsigned char> gLoginChallenge;

static void Password(const char *pass, const char *key) {
	size_t keylen = strlen(key);
	size_t passlen = strlen(pass);
	// Create a new key, which is the XOR of the "key" and the challenge that
	// was received from the server. If the vectors are of unequal length,
	// the end is padded with values from the longest.
	size_t newLength = keylen;
	if (gLoginChallenge.size() > newLength)
		newLength = gLoginChallenge.size();
	unsigned char newkey[newLength];
	for (size_t i=0; i<newLength; i++) newkey[i] = 0;
	for (size_t i=0; i<keylen; i++) newkey[i] = key[i];
	for (size_t i=0; i<gLoginChallenge.size(); i++) newkey[i] ^= gLoginChallenge[i];
#if 0
	printf("Password: Key (len %d): ", newLength);
	for (int i=0; i<newLength; i++) printf("%d ", newkey[i]);
	printf("\n");
#endif
	unsigned char b[passlen+3];
	// Build the message into 'b'.
	b[0] = passlen+3; // LSB of message length
	b[1] = 0;   // MSB of message length
	b[2] = CMD_RESP_PASSWORD;
	memcpy(b+3, pass, passlen); // Initialize with the clear text password
	rc4_init(newkey, newLength);
	rc4_xor(b+3, passlen); // Encrypt the password

#if 0
	printf("encr: ");
	for (int i=0; i<passlen; i++) printf(" %d", b[i+3]);
	printf("\n");
#endif

	SendMsg(b, passlen+3);
}

bool PerformLoginProcedure(const string &email, const string &licencekey, const string &password, bool testOverride) {
	// Wait for acknowledge from server (in the form of a protocol command)
	auto beginTime = glfwGetTime();
	if (gMode.Get() != GameMode::LOGIN) {
		auto &ss = View::gErrorManager.GetStream(true, false);
		ss << "Login in wrong state " << gMode.Get();
		return false;
	}

	LoginMessage(testOverride ? "test0" : email.c_str());
	gMode.Set(GameMode::PASSWORD);
	while (gMode.Get() != GameMode::GAME) {
		glfwSleep(0.01); // Avoid a busy wait
		ListenForServerMessages(); // Wait for automatic login without password
		switch (gMode.Get()) {
		case GameMode::PASSWORD:
		case GameMode::WAIT_ACK:
			continue; // Nothing to do yet.
		case GameMode::REQ_PASSWD:
			// printf("Parse: mode GameMode::REQ_PASSWD, %d chall bytes\n", gLoginChallengeLength);
			Password(password.c_str(), licencekey.c_str());
			gMode.Set(GameMode::WAIT_ACK);
			// printf("loginDialog::UpdateDialog: Mode GameMode::WAIT_ACK\n");
			continue;
		case GameMode::LOGIN_FAILED: {
			auto &ss = View::gErrorManager.GetStream(true, false);
			ss << "Login failed";
			return false;
		}
		case GameMode::ESC:
			exit(1);
		case GameMode::GAME:
			break; // Now we are done!
		case GameMode::TELEPORT:
		case GameMode::CONSTRUCT:
		case GameMode::INIT:
		case GameMode::LOGIN:
		case GameMode::EXIT:
			std::cerr << "PerformLoginProcedure Unexpected mode" << gMode.Get();
			exit(1);
		}
	}
	double connectionDelay = glfwGetTime() - beginTime;
	if (connectionDelay > 0.5 && gDebugOpenGL)
		printf("PerformLoginProcedure connection delay %f\n", connectionDelay);
	unsigned char b[] = { 0x03, 0x00, CMD_GET_COORDINATE }; // GET_COORDINATE
	SendMsg(b, sizeof b);

	View::gSoundControl.RequestMusicMode(View::SoundControl::SMusicModeTourist);

	return true;
}
