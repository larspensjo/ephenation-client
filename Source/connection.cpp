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

#include <GL/glew.h>
#include <stdio.h>
#include <sys/types.h>
#include <vector>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

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
#include "ui/Error.h"
#include "worsttime.h"

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
#ifdef WIN32
		auto err = WSAGetLastError();
		switch(err) {
		case WSAETIMEDOUT:
			ErrorDialog("No connection to server");
			break;
		default:
			ErrorDialog("Failed to connect to server (%d)", err);
		}
#else
		auto err = errno;
		ErrorDialog("Failed to connect to server (%d)", err);
#endif
		exit(0);
	}
	// printf("Got fd %d\n", sock_fd);
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
	if (tm.Get() > 0.030) {
		// printf("ListenForServerMessages: Long reading time (%.1f ms). Message %2d, length %4d\n", tm.Get() * 1000, buff[0], n);
	}
	if (count == 0 || (count<0 && errno == EEXIST)) { // EEXIST can happen on windows if connection is broken
		gMode.Set(GameMode::ESC);
		count = 0;
	}
	if (count < 0) {
		if (errno != EAGAIN) {
#ifdef WIN32
			ErrorDialog("ListenForServerMessages1: Error %d, length %d\n", errno, count);
#else
			ErrorDialog("ListenForServerMessages1: Error %d (%s), length %d\n", errno, sys_errlist[errno], count);
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
		ErrorDialog("ListenForServerMessages select() failed: errno %d\n", errno);
	}
	if (cnt == 0)
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

void LoginMessage(const char *id) {
	int len = strlen(id);
	int totLength = len+3;
	unsigned char b[totLength];
	b[0] = totLength&0xff; // LSB of message length
	b[1] = totLength>>8;   // MSB of message length
	b[2] = CMD_LOGIN;
	memcpy(b+3, id, len);
	SendMsg(b, totLength);
}
