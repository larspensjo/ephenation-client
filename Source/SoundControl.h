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

#pragma once

#include <pthread.h>
#include <semaphore.h>

#ifdef unix
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#else
#include <al.h>
#include <alc.h>
#endif
#include <AL/alut.h>

#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>

namespace View {

/// @brief Manage the sound effects and music
class SoundControl {
public:
	SoundControl(void);
	~SoundControl(void);

	typedef unsigned long Sound; // The type to be used for a bit mapped sound flag
	// These are the sounds that are available.
	enum { 	SNone = 0, STerminate = 1<<0, STSExec = 1<<1, SGrowl = 1<<2, SPlayerHits = 1<<3,
	        SMonsterHits = 1<<4, SHealingSelf = 1<<5, SPlayerJump = 1<<6, SPlayerLand = 1<<7,
	        SLevelUp = 1<<8, SInterfacePing = 1<<9, SBuildBlock = 1<<10, SRemoveBlock = 1<<11,
	        SDropPotion = 1<<12, SDropWeapon = 1<<13, SDropArmor = 1<<14, SDropScroll = 1<<15
	     };

	// This type is used for the main process to signal changes to the environment
	enum SoundFxStatus { SPlayerRunning, SPlayerWalking, SPlayerFeetInWater, SPlayerSwimming, SPlayerInAir, SEnvironmentEcho, SEnvironmentUnderWater };

	// This is the different music modes available
	enum MusicMode { SMusicModeNone, SMusicModeSilence, SMusicModeTourist, SMusicModeCombat, SMusicModeMenu, SNumMusicModes };

	// Objects making noise are identified by this
	enum SoundObject { SNoCreature, SToBeRemoved, SOtherPlayer, SMonster, STeleport };

	typedef int SoundControlObject;

	void Init(void);

	// Play a sound. The request may fail, unless 'force' is true.
	void RequestSound(Sound, bool force = false);
	void RequestTrigSound(const char *);
	void SetSoundFxStatus(SoundFxStatus,bool);
	void SetCreatureSound(SoundObject creatureType,unsigned long id, float dx, float dy, float dz, bool dead, float size);
	void SetEnvironmentSound(SoundObject soundType,unsigned long id, float dx, float dy, float dz);
	void RemoveEnvironmentSound(SoundObject,unsigned long);
	void RemoveCreatureSound(SoundObject,unsigned long);
	void RequestMusicMode(MusicMode);
	void SwitchMusicStatus(void);

	// This needs to be public in order to be accessible from parse.cpp when building the activator dialog
	unsigned int fNumTrigSounds;
	struct TrigSoundItem {
		char id[5];
		const char *fileName;
		const char *description;
	};
	static TrigSoundItem fTrigSoundList[];

private:
	Sound fRequestedSound;		// The sound scheduled for being played
	char fRequestedTrigSound[4];

	bool fPlayerRunning; 		// This is true as long as the player is running
	bool fFeetInWater;			// This is true while the player has his feet in water (to make splashing sound)
	bool fFeetInAir;			// This is true when the players feet do not touch the ground
	bool fUnderWater;			// This is true while the players head is under water
	bool fEcho;

	bool fMusicOn;
	bool fMusicIsPlaying;
	bool fMusicFadeOut;

	// Some settings for the OpenAL context
	bool fAudioEnabled;			// TODO: Preparation for playing without sound if audio not present
	bool fEAXPresent;
	int fALMaxSources;

	ALfloat fMusicVolume, fMusicVolumePref;
	FILE *oggFile;
	OggVorbis_File oggStream;
	vorbis_info* vorbisInfo;
	ALenum fMusicFormat;
	MusicMode fNextMusicMode, fCurrentMusicMode;
	MusicMode fRequestedMusicMode;

	pthread_cond_t fCondLock;	// The conditional lock
	pthread_mutex_t fMutex;		// The mutex used for locking
	pthread_attr_t fAttr;		// Used to setup the child thread
	pthread_t fThread;			// Identifies the child thread
	static void *Thread(void *p);	// This is the child thread

	// Helper functions
	void SrcBaseConfig(ALuint);

	// Functions for music playback
	bool MusicStream(ALuint);
	bool MusicPlaying(void);
	bool MusicPlayback(void);
	bool MusicUpdate(void);
	void MusicConfigure(void);
	void MusicStop(void);
	bool OpenOggStream(MusicMode);
	void CloseOggStream(void);
	void HandleSpecialFx(void);
	void HandleMusic(void);
	void HandleTrigSound(const char *);

	// Functions for creature sound management
	void CreatureSetPosition(int,float,float,float);
	void CreatureSetGruntSourcePosition(int,float,float,float);
	void CreatureSetMoveSourcePosition(int,float,float,float);
	void HandleCreatures(void);

	// Functions for environment sound management
	void EnvironmentSetPosition(int,float,float,float);
	void HandleEnvironment(void);
	void EnvironmentSetSourcePosition(int,float,float,float);

	// Functions for player sound management
	void HandlePlayerOrientation(void);

	int MusicChangeTimeOut;
	bool WasRunning, WasInWater, WasUnderWater, WasInAir;

	// Define sources
	enum { SSrcPlayerHit, SSrcMonsterHit, SSrcFootsteps, SSrcLevelUp, SSrcHealSelf, SSrcInterfacePing, SSrcPlayerVoice, SMusicSource, SSrcBuildAction, SSrcTrigEvent, SSrcTest, SNumSources };

	bool fALinitialized;                // True when OpenAL has been initialized

	// Define buffers
	enum { SBufPlayerHit, SBufMonsterHit, SBufFootsteps, SBufFootstepsInWater, SBufLevelUp, SBufHealSelf, SBufRunningWater, SBufWind, SBufTeleport,
	       SBufInterfacePing, SBufMonsterGrunt, SBufMonsterGrowl, SBufOtherFootSteps, SBufBlockBuild, SBufBlockRemove, SBufTrigEvent, SBufPlayerJump,
	       SNumFxBuffers=SBufPlayerJump, SMusicBuffer1, SMusicBuffer2, SNumBuffers, SNumMusicBuffers = SMusicBuffer2-SMusicBuffer1
	     };

	ALuint fBuffers[SNumBuffers];		// Sound buffers
	ALuint fSources[SNumSources];		// Sound sources

	enum { SNumEnvSrc = 2 };
	ALuint fEnvironmentSources[SNumEnvSrc];
	bool fEnvironmentSourceFree[SNumEnvSrc];

	enum { SNumCreatureSrc = 6 };
	ALuint fCreatureSources[SNumCreatureSrc];
	bool fCreatureSourceFree[SNumCreatureSrc];

	struct SoundBuffer {
		int bufferId;
		int sourceId;
		ALboolean looping;
		char *fileName;
	};

	static SoundBuffer fSoundBuffers[];

	struct MusicList {
		char id[5];
		MusicMode musicType;
		char *fileName;
	};

	static MusicList fSoundtrack[];
	int fSongCount[SNumMusicModes];

	// Keeping track of moving sound sources in the game
	// Creatures
	struct SoundCreature {
		unsigned long id;
		SoundObject type;
		float size;
		float positionX, positionY, positionZ;
		float lastX, lastY, lastZ;
		float distance;
		int attachedGruntSource; // This should be an ALuint if attached to a source, -1 else
		int attachedMoveSource; // This should be an ALuint if attached to a source, -1 else
		bool mute;
	};

	enum {MAX_CREATURES=100};

	SoundCreature KnownCreatures[MAX_CREATURES];

	// Keeping track of moving sound sources in the game
	// Environment
	struct SoundEnvironment {
		unsigned long id;
		SoundObject type;
		float size;
		float positionX, positionY, positionZ;
		float lastX, lastY, lastZ;
		float distance;
		int attachedEnvironmentSource; // This should be an ALuint if attached to a source, -1 else
		int timeOut;
		bool mute;
	};

	enum {MAX_ENVIRONMENTITEMS=50};

	SoundEnvironment EnvironmentSounds[MAX_ENVIRONMENTITEMS];
};

// One sound server, to be used to play the sounds.
extern SoundControl gSoundControl;

}
