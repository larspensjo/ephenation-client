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

// TODO: This file should be generated automatically from the go file.
// As of now, it is a hand made translation from client_prot.go. For the exact definition of this
// protocol, see also client_prot.go

#define CMD_LOGIN             1  // The argument is a string with the login name
#define CMD_SAVE              2  // No argument
#define CMD_QUIT              3  // No argument
#define CMD_MESSAGE           4  // A text message to the client
#define CMD_GET_COORDINATE    5  // Used to request a coordinate (no arguments)
#define CMD_REPORT_COORDINATE 6  // The answer sent from the server. Format is 3 x 8 bytes, with x, y, z, LSB first.
#define CMD_READ_CHUNK        7  // Read chunk at offset x,y,z from current chunk. Coded as 4 bytes, will be multiplied by chunk_size
#define CMD_CHUNK_ANSWER      8  // The requested chunk sent back to the client
#define CMD_LOGIN_ACK         9  // Acknowledge login to the client
#define CMD_START_FWD         10 // Player start moving forward
#define CMD_STOP_FWD          11 // Player stop moving forward
#define CMD_START_BWD         12 // Player start moving backward
#define CMD_STOP_BWD          13 // Player stop moving backward'
#define CMD_START_LFT         14 // Player start moving left
#define CMD_STOP_LFT          15 // Player stop moving left
#define CMD_START_RGT         16 // Player start moving right
#define CMD_STOP_RGT          17 // Player stop moving right
#define CMD_JUMP              18 // Player jump
#define CMD_SET_DIR           19 // For the client to set the looking direction.
#define CMD_OBJECT_LIST		  20 // A list of player positions
#define CMD_HIT_BLOCK		  21 // The player hit at a block
#define CMD_BLOCK_UPDATE      22 // One or more blocks have been updated in a chunk
#define CMD_DEBUG             23 // A string sent to the server, interpreted as a debug command
#define CMD_REQ_PASSWORD      24 // Request the password from the client, encrypt it with RC4 using argument.
#define CMD_RESP_PASSWORD     25 // An encrypted password from the client to the server
#define CMD_PROT_VERSION	  26 // The version of the communication protocol
// #define CMD_REQ_CHUNK_CS      27 // Request the checksum for a chunk
// #define CMD_RESP_CHUNK_CS     28 // The reported checksum
#define CMD_VRFY_SUPERCHUNCK_CS 29 // Request server to verify one or more super chunk checksums. If wrong, an update will be sent.
#define CMD_SUPERCHUNK_ANSWER 29 // (Same number) The full requested super chunk sent back to the client
#define CMD_PLAYER_STATS      30 // Information about the player that doesn't change very often.
#define CMD_ATTACK_MONSTER    31 // Initiate an attack on a monster
#define CMD_PLAYER_ACTION     32 // A generic action, see UserAction* below
#define CMD_RESP_PLAYER_HIT_BY_MONSTER 33 // The player was hit by one or more monsters
#define CMD_RESP_PLAYER_HIT_MONSTER 34 // The player hit one or more monsters
#define CMD_RESP_AGGRO_FROM_MONSTER 35 // The player got aggro from one or more monsters
#define CMD_VRFY_CHUNK_CS     36 // Request server to verify checksum for one or more chunks. If wrong, the updated chunk will be sent.
#define CMD_USE_ITEM          37 // The player uses an item from the inventory.
#define CMD_UPD_INV           38 // Server updates the client about the amount of items.
#define CMD_EQUIPMENT         39
#define CMD_JELLY_BLOCKS      40 // Temporarily turn blocks invisible and permeable.
#define CMD_PING              41
#define CMD_DROP_ITEM         42 // Drop an item from the inventory
#define CMD_LOGINFAILED       43
#define CMD_REQ_PLAYER_INFO   44 // Request player information
#define CMD_RESP_PLAYER_NAME  45 // A name of a player
#define CMD_TELEPORT          46 // Teleport the player to the specified chunk

#define PROT_VER_MAJOR 5 // What major version of the protocol is supported
#define PROT_VER_MINOR 2 // What minor version of the protocol is supported

// These are the types used by CMD_OBJECT_LIST
#define ObjTypePlayer 			0
#define ObjTypeMonster 			1

// Flags used by the CMD_PLAYER_STATS
#define UserFlagInFight         (1 << 0)
#define UserFlagPlayerHit       (1 << 1) // The player hit a monster, transient flag
#define UserFlagMonsterHit      (1 << 2) // A monster hit the player, transient flag
#define UserFlagHealed          (1 << 3) // The player was healed (not the gradual healing)
#define UserFlagJump            (1 << 4) // The sound when the user hits the ground after jumping

#define UserActionHeal          0 // Request a healing
#define UserActionCombAttack    1 // Request a special attack


#define PLAYER_HEIGHT 1.8f // TODO: Should be defined by the server.
