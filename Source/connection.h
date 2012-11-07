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

#pragma once

#include <string>

using std::string;

// The major and minor version of the available client
extern int gClientAvailMajor, gClientAvailMinor;

extern void ConnectToServer(const char *host, int port);

extern bool ListenForServerMessages(void);

extern void SendMsg(const unsigned char *b, int n);

extern void Password(const unsigned char *key, int len);

// Send login info to server and wait for acknowledge. Return true if success.
bool PerformLoginProcedure(const string &email, const string &licencekey, const string &password, bool testOverride);
