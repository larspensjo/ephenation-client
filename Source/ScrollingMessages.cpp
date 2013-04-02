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

#include <GL/glew.h>
#include <GL/glfw.h>
#include <stdio.h>
#include <entityx/Components.h>

#include "ScrollingMessages.h"
#include "primitives.h"
#include "object.h"
#include "DrawText.h"
#include "player.h"
#include "parse.h"
#include "monsters.h"
#include "Inventory.h"
#include "msgwindow.h"

using std::shared_ptr;

using namespace View;

/// The message component added to entities used for scrolling texts.
struct MessageCmp : entityx::Component<MessageCmp> {
	MessageCmp(std::shared_ptr<const Model::Object> o, glm::vec3 colorOffset, shared_ptr<DrawFont> font, const std::string &str) :
		o(o), startTime(glfwGetTime()), colorOffset(colorOffset), font(font), id(font->vsfl.genSentence())
	{
		font->vsfl.prepareSentence(id, str);
	}
	std::shared_ptr<const Model::Object> o; // A reference that is allocated and deallocated elsewhere
	double startTime;
	glm::vec3 colorOffset;
	shared_ptr<DrawFont> font;
	GLuint id; // Sentence Id
	~MessageCmp() {
		font->vsfl.deleteSentence(id);
	}
};

// @todo This ScrollMsgReceiver should be combined into the ScrollingMessages system?
struct ScrollMsgReceiver : public entityx::Receiver<ScrollMsgReceiver> {
	void receive(const PlayerHitByMonsterEvt &evt);
	void receive(const MonsterHitByPlayerEvt &evt);
	void receive(const Inventory::AddObjectToPlayerEvt &evt);
	void receive(const MsgWindow::MessageEvt &evt);

	/// Register self for events
	void Init(ScrollingMessages *pScrollingMessages, entityx::EventManager &events) {
		fScrollingMessages = pScrollingMessages;
		events.subscribe<PlayerHitByMonsterEvt>(*this);
		events.subscribe<MonsterHitByPlayerEvt>(*this);
		events.subscribe<Inventory::AddObjectToPlayerEvt>(*this);
		events.subscribe<MsgWindow::MessageEvt>(*this);
	}

	ScrollingMessages *fScrollingMessages; // Save pointer to the system that will use the event

	ScrollMsgReceiver() : fScrollingMessages(0) {}
};

static ScrollMsgReceiver sEventReceiver;

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

ScrollingMessages::ScrollingMessages(entityx::EntityManager &es) : fEntities(es) {
	fFont = std::make_shared<DrawFont>();
	fFont->Init("textures/gabriola18");
}

void ScrollingMessages::AddMessage(std::shared_ptr<const Model::Object> o, const std::string &str, glm::vec3 colorOffset) {
	auto entity = fEntities.create();
	entity.assign<MessageCmp>(o, colorOffset, fFont, str);
}

void ScrollingMessages::AddMessagePlayer(const std::string &str, glm::vec3 colorOffset) {
	// Some tricks are used here. A shared_ptr to an object is required, but gPlayer is a global variable.
	auto DummyDelete = [](Model::Player*) {}; // This is a deleter function that will do nothing.
	auto pl = std::shared_ptr<Model::Player>(&Model::gPlayer, DummyDelete); // Create a shared_ptr to gPlayer, which must not be deleted
	this->AddMessage(pl, str, colorOffset);
}

void ScrollingMessages::AddMessage(float x, float y, const std::string &str, glm::vec3 colorOffset) {
	auto so = std::make_shared<ScreenObject>();
	so->fScreen.x = x;
	so->fScreen.y = y;
	auto entity = fEntities.create();
	entity.assign<MessageCmp>(so, colorOffset, fFont, str);
}

void ScrollingMessages::update(entityx::EntityManager &, entityx::EventManager &events, double dt) {
	fFont->Enable();
	fFont->UpdateProjection();

	std::shared_ptr<MessageCmp> message;
	for (auto entity : fEntities.entities_with_components(message)) {
		// Do things with entity ID, position and direction.
		double delta = gCurrentFrameTime - message->startTime;
		if (delta > 6.0) {
			entity.destroy();
			continue;
		}
		glm::vec3 pos = message->o->GetPosition();
		if (message->o->GetType() != -1) {
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
		fFont->vsfl.renderSentence(message->id, message->colorOffset);
	}
	fFont->Disable();
}

void ScrollingMessages::configure(entityx::EventManager &events) {
	sEventReceiver.Init(this, events);
}

void ScrollMsgReceiver::receive(const PlayerHitByMonsterEvt &evt) {
	std::stringstream ss;
	ss << int(evt.damage*100+0.5f);
	fScrollingMessages->AddMessagePlayer(ss.str(), glm::vec3(0, -1, -1));
	// @todo Add an Entity instead.
}

void ScrollMsgReceiver::receive(const MonsterHitByPlayerEvt &evt) {
	auto m = Model::gMonsters->Find(evt.id);
	if (m.valid()) {
		std::stringstream ss;
		ss << int(evt.damage*100+0.5f);
		// fScrollingMessages->AddMessage(m, ss.str(), glm::vec3(0, 0, -1)); // Use yellow color for monster
	}
	// @todo Add an Entity instead.
}

void ScrollMsgReceiver::receive(const Inventory::AddObjectToPlayerEvt &evt) {
	fScrollingMessages->AddMessagePlayer(evt.map->descr);
}

void ScrollMsgReceiver::receive(const MsgWindow::MessageEvt &evt) {
	fScrollingMessages->AddMessage(evt.dropx, evt.dropy, evt.buff);
}
