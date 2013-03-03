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

/// @file soundevents.cpp
/// This file manages events used to trig sounds.

#include <entityx/Event.h>

#include "SoundControl.h"
#include "entitycomponentsystem.h"
#include "parse.h"
#include "player.h"

using namespace View;

struct SoundEventReceiver : public entityx::Receiver<SoundEventReceiver> {
	/// Register self for events
	void Init(SoundControl *pSoundControl, entityx::EventManager &events) {
		fSoundControl = pSoundControl;
		events.subscribe<OtherPlayerUpdateEvt>(*this);
		events.subscribe<MonsterUpdateEvt>(*this);
		events.subscribe<NoticeEvt>(*this);
		events.subscribe<PlayerStatsChangedEvt>(*this);
	}

	SoundControl *fSoundControl; // Save pointer to the system that will use the event

	SoundEventReceiver() : fSoundControl(0) {}

	void receive(const OtherPlayerUpdateEvt &evt) {
		fSoundControl->SetCreatureSound(SoundControl::SOtherPlayer, evt.id, evt.x-Model::gPlayer.x, evt.y-Model::gPlayer.y, evt.z-Model::gPlayer.z, evt.hp==0, 0.0f);
	}

	void receive(const MonsterUpdateEvt &evt) {
		fSoundControl->SetCreatureSound(SoundControl::SMonster, evt.id, evt.x-Model::gPlayer.x, evt.y-Model::gPlayer.y, evt.z-Model::gPlayer.z, evt.hp==0, evt.size);
	}

	void receive(const NoticeEvt &evt) { fSoundControl->RequestSound(SoundControl::SInterfacePing); }

	void receive(const PlayerStatsChangedEvt &evt) {
		SoundControl::Sound sound = SoundControl::SNone;
		if (evt.healed)
			sound |= SoundControl::SHealingSelf;
		if (evt.levelUp)
			sound |= SoundControl::SLevelUp;
		if (evt.touchDown)
			sound |= SoundControl::SPlayerLand;
		if (sound != SoundControl::SNone)
			gSoundControl.RequestSound(sound);
	}
};

static SoundEventReceiver sEventReceiver;

void SoundControl::InitializeEvents(entityx::EventManager &em) {
	sEventReceiver.Init(this, em);
}
