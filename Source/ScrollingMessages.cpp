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

#include "ScrollingMessages.h"
#include "primitives.h"
#include "object.h"
#include "DrawText.h"
#include "player.h"

using std::list;
using std::shared_ptr;

using namespace View;

boost::shared_ptr<ScrollingMessages> View::gScrollingMessages = boost::make_shared<ScrollingMessages>();

// This is used as a special object to denote a screen coordinate instead of a world position
class ScreenObject : public Model::Object {
public:
	virtual unsigned long GetId() const { return 0; }
	virtual int GetType() const { return -1; } // -1 is used to identify this unique object
	virtual int GetLevel() const { return 0; }
	virtual glm::vec3 GetPosition() const { return fScreen; }
	virtual glm::vec3 GetSelectionColor() const { return glm::vec3(0,0,0); }
	virtual bool IsDead(void) const { return false; }
	virtual void RenderHealthBar(View::HealthBar *, float angle) const {}
	virtual bool InGame(void) const { return true; }
	glm::vec3 fScreen; // Only x and y will be used
};

struct ScrollingMessages::Message {
	shared_ptr<const Model::Object> o; // A reference that is allocated and deallocated elsewhere
	GLuint id;
	double startTime;
	glm::vec3 colorOffset;
	shared_ptr<DrawFont> fFont;
	~Message() {
		fFont->vsfl.deleteSentence(id);
	}
};

ScrollingMessages::ScrollingMessages() {
	// TODO Auto-generated constructor stub

}

ScrollingMessages::~ScrollingMessages() {
	// TODO Auto-generated destructor stub
}

void ScrollingMessages::AddMessage(shared_ptr<const Model::Object> o, const std::string &str, glm::vec3 colorOffset) {
	unique_ptr<Message> m(new Message);
	m->o = o;
	m->id = fFont->vsfl.genSentence();
	m->startTime = gCurrentFrameTime;
	m->colorOffset = colorOffset;
	m->fFont = fFont;
	fFont->vsfl.prepareSentence(m->id, str);
	fMessageList.push_back(std::move(m));
}

void ScrollingMessages::AddMessagePlayer(const std::string &str, glm::vec3 colorOffset) {
	// Some tricks are used here. A shared_ptr to an object is required, but gPlayer is a global variable.
	auto DummyDelete = [](Model::Player*) {}; // This is a deleter function that will do nothing.
	auto pl = shared_ptr<Model::Player>(&Model::gPlayer, DummyDelete); // Create a shared_ptr to gPlayer, which must not be deleted
	this->AddMessage(pl, str, colorOffset);
}

void ScrollingMessages::AddMessage(float x, float y, const std::string &str, glm::vec3 colorOffset) {
	auto so = std::make_shared<ScreenObject>();
	so->fScreen.x = x;
	so->fScreen.y = y;
	unique_ptr<Message> m(new Message);
	m->o = so;
	m->id = fFont->vsfl.genSentence();
	m->startTime = gCurrentFrameTime;
	m->colorOffset = colorOffset;
	m->fFont = fFont;
	fFont->vsfl.prepareSentence(m->id, str);
	fMessageList.push_back(std::move(m));
}

void ScrollingMessages::Update(void) {
	fFont->Enable();
	fFont->UpdateProjection();

	// Iterate for each message
	for (auto it=fMessageList.begin() ; it != fMessageList.end(); ) {
		auto current = it;
		it++; // Move iterator already now, as current item may be deleted
		auto m = (*current).get(); // Get a temporary pointer to the message.
		double delta = gCurrentFrameTime - m->startTime;
		if (delta > 6.0) {
			fMessageList.erase(current);
			continue;
		}
		glm::vec3 pos = m->o->GetPosition();
		if (m->o->GetType() != -1) {
			glm::vec4 v = glm::vec4(pos.x, pos.y+2, pos.z, 1.0f);
			glm::vec4 screen = gProjectionMatrix * gViewMatrix * v;
			screen = screen / screen.w;
			float screenHeight = gViewport[3] * (1-screen.y)/2 - delta*50;
			fFont->SetOffset(gViewport[2] * (screen.x+1)/2, screenHeight);
			// printf("ScrollingMessages::Update %f, %f\n", gViewport[2] * (screen.x+1)/2, screenHeight);
		} else {
			fFont->SetOffset(pos.x, pos.y - delta*50);
			// printf("ScrollingMessages::Update %f, %f\n", pos.x, pos.y - delta*50);
		}
		fFont->vsfl.renderSentence(m->id, m->colorOffset);
	}
	fFont->Disable();
}

void ScrollingMessages::Init(shared_ptr<DrawFont> font) {
	fFont = font;
}

void ScrollingMessages::update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) {
}
