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

#pragma once

#include <sstream>

namespace View {

/// @brief Manage errors during execution
/// The purpose is to collect error reports detected during execution,
/// present them to the user and forward them to the server.
/// As error messages are saved and presented later, creating errors can be done
/// also from callbacks from other libraries where it would be sensitive to pop up a window.
class ErrorManager {
public:
	/// @brief Return a stringstream that is used for composing the error message
	/// This function will initiate a graceful shutdown.
	/// @param inhibitServer Send no copy of the message sent to the server.
	/// @param inhibitPopoup Show no popup window with the error message.
	std::stringstream &GetStream(bool inhibitServer, bool inhibitPopoup);
	
	/// @brief Report errors, if any.
	void Report() const;
	
	ErrorManager();
private:
	/// @brief Produce a win32 message box with the error message
	/// @arg err The error message to be nicluded.
	void Win32MessageBox(const std::string &arg) const;
	
	/// @brief Send a message to the server about the error
	/// @arg err The error message to be nicluded.
	void SendServerMessage(const std::string &err) const;

	std::stringstream fStream;
	
	bool fInhibitServer, fInhibitPopup;
};

/// @brief A global instance of the error manager
extern ErrorManager gErrorManager;

}
