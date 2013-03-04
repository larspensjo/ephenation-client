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

/// @file generalevents.h
/// A list of events not associated with a specific generator

#include <entityx/Event.h>
#include <string>

struct FailureMessageEvt : public entityx::Event<FailureMessageEvt> {
	/// Add a message originating at an object
	FailureMessageEvt(const std::string &msg) : msg(msg) {}

	const std::string &msg; /// Describes the object
};
