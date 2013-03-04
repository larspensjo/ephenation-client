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
#include "chunk.h"
#include "Inventory.h"

using namespace View;

struct SoundEventReceiver : public entityx::Receiver<SoundEventReceiver> {
	/// Register self for events
	void Init(SoundControl *pSoundControl, entityx::EventManager &events) {
		fSoundControl = pSoundControl;
		events.subscribe<OtherPlayerUpdateEvt>(*this);
		events.subscribe<MonsterUpdateEvt>(*this);
		events.subscribe<Inventory::AddObjectToPlayerEvt>(*this);
		events.subscribe<NoticeEvt>(*this);
		events.subscribe<PlayerHitByMonsterEvt>(*this);
		events.subscribe<MonsterHitByPlayerEvt>(*this);
		events.subscribe<PlayerStatsChangedEvt>(*this);
		events.subscribe<BlockUpdateEvt>(*this);
	}

	SoundControl *fSoundControl; // Save pointer to the system that will use the event

	SoundEventReceiver() : fSoundControl(0) {}

	void receive(const OtherPlayerUpdateEvt &evt) {
		fSoundControl->SetCreatureSound(SoundControl::SOtherPlayer, evt.id, evt.x-Model::gPlayer.x, evt.y-Model::gPlayer.y, evt.z-Model::gPlayer.z, evt.hp==0, 0.0f);
	}

	void receive(const MonsterUpdateEvt &evt) {
		fSoundControl->SetCreatureSound(SoundControl::SMonster, evt.id, evt.x-Model::gPlayer.x, evt.y-Model::gPlayer.y, evt.z-Model::gPlayer.z, evt.hp==0, evt.size);
	}

	void receive(const Inventory::AddObjectToPlayerEvt &evt) {
		SoundControl::Sound sound = SoundControl::SNone;
		switch(evt.map->category) {
			case Inventory::ICWeapon: sound = SoundControl::SDropWeapon; break;
			case Inventory::ICArmor:  sound = SoundControl::SDropArmor; break;
			case Inventory::ICHead:   sound = SoundControl::SDropArmor; break;
			case Inventory::ICPotion: sound = SoundControl::SDropPotion; break;
			case Inventory::ICScroll: sound = SoundControl::SDropScroll; break;
		}
		fSoundControl->RequestSound(sound);
	}

	void receive(const BlockUpdateEvt &evt) {
		if (evt.type == BT_Air)
			fSoundControl->RequestSound(SoundControl::SRemoveBlock);
		else
			fSoundControl->RequestSound(SoundControl::SBuildBlock);
	}

	void receive(const NoticeEvt &evt) { fSoundControl->RequestSound(SoundControl::SInterfacePing); }
	void receive(const PlayerHitByMonsterEvt &evt) { fSoundControl->RequestSound(SoundControl::SMonsterHits); }
	void receive(const MonsterHitByPlayerEvt &evt) { fSoundControl->RequestSound(SoundControl::SPlayerHits); }

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
