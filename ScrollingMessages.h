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

#include <list>
#include <string>
#include <memory>

using std::unique_ptr;

#include "glm/glm.hpp"

//
// Implement a list of scrolling messages.
// A message is added to a specific coordinate, and will scroll in real time up to the top
// of the screen where it will disappear.

struct Object;
class DrawFont;

class ScrollingMessages {
public:
	ScrollingMessages();
	virtual ~ScrollingMessages();
	void Init(std::shared_ptr<DrawFont> font);
	void Update(void);
	// Add a message originating at n object
	void AddMessage(Object *, const std::string &, glm::vec3 colorOffset = glm::vec3(0,0,0));
	// Add a message originating at a screen position. If there is another message active already,
	// it will be moved along to the new position.
	void AddMessage(float x, float y, const std::string &, glm::vec3 colorOffset = glm::vec3(0,0,0));
private:
	struct Message;
	std::list<unique_ptr<Message>> fMessageList;
	std::shared_ptr<DrawFont> fFont;
};

extern ScrollingMessages gScrollingMessages;
