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

/**
 * @file parse.h
 * @brief Manage parsing of the communication protocol
 * This is part of the Controller.
 * @todo Old inheritance from C, should be managed with a class
 */

#include <string>
#include <vector>

/**
 * @brief Parse a message from the server
 */
void Parse(const unsigned char *, int);

/**
 * @brief Parse a 16-bit unsigned from two bytes. LSB first.
 */
unsigned short Parseuint16(const unsigned char *);

/**
 * @brief Encode a 32 bit unsigned number as 4 bytes, LSB first.
 */
void EncodeUint32(unsigned char *b, unsigned int val);

/**
 * @brief Encode a 16 bit unsigned number as 4 bytes, LSB first.
 */
void EncodeUint16(unsigned char *b, unsigned short val);

/**
 * @brief Encode a 64 bit unsigned number as 5 bytes, LSB first.
 */
void EncodeUint40(unsigned char *b, unsigned long long val);

void DumpBytes(const unsigned char *b, int n);

/**
 * @brief Used for the key in the encryption
 */
extern std::vector<unsigned char> gLoginChallenge;

/**
 * @brief The parser may save a message to be displayed at login.
 * This is typically used for information about mismatch in protocols, etc.
 */
extern std::string gParseMessageAtLogin;
