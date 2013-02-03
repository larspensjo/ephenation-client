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

//
// Sound is managed by a child process, using a conditional lock and pthreads.
//

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <GL/glfw.h>

#include "SoundControl.h"
#include "errormanager.h"
#include "Options.h"
#include "modes.h"
#include "player.h"

//#define MUSIC_DEBUG

#define MUSIC_TIMEOUT_VAL 75

// Some lazy defines
// A modifier to divide the block coordinate with in order to get a fair "distance"
#define SOUND_DISTANCE_MOD 1000.0f
// A maximum distance for grunts, monsters beyond this point (x*x+y*y+z*z) will not be heard
// NOTE: The current value corresponds approximately to the "compass circle" in the game
#define SOUND_DISTANCE_MAX 9500000.0f

using namespace View;

SoundControl View::gSoundControl;

// Macro that computes number of elements of a static vector
#define NELEM(v) (sizeof v / sizeof v[0])

/*
** SUPPORT FUNCTIONS
*/
static ALenum err; // Declaring this global for the check error macro to work everywhere
#define CheckForError(x) err = alGetError(); if( err != AL_NO_ERROR) { printf("SoundControl: error %s at %s!\n",GetALErrorString(err),x);}

// Helper function for errors
const char *GetALErrorString(ALenum err) {
	switch(err) {
	case AL_NO_ERROR:
		return "AL_NO_ERROR";
	case AL_INVALID_NAME:
		return "AL_INVALID_NAME";
	case AL_INVALID_ENUM:
		return "AL_INVALID_ENUM";
	case AL_INVALID_VALUE:
		return "AL_INVALID_VALUE";
	case AL_INVALID_OPERATION:
		return "AL_INVALID_OPERATION";
	case AL_OUT_OF_MEMORY:
		return "AL_OUT_OF_MEMORY";
	default:
		return "Unknown error";
	}
}

/*
** CONFIGURATION DATA
*/
// This is the configuration data for the sound buffers and some of the sources
SoundControl::SoundBuffer SoundControl::fSoundBuffers[] = {
	/* Buffer				Source (-1 undef)	Looping,	File */
	// Sounds for player hitting a monster (or the player making an attack)
	{ SBufPlayerHit,		SSrcPlayerHit, 		AL_FALSE,	(char *)"sounds/Knife_SL-Derka_De-8768.wav" },
	// Monster attack sounds
	{ SBufMonsterHit,		SSrcMonsterHit, 	AL_FALSE,	(char *)"sounds/attack.wav" },
	// Sounds for footsteps on different materials
	// Source is attached to normal footsteps at startup
	{ SBufFootsteps,		SSrcFootsteps, 		AL_TRUE,	(char *)"sounds/Footsteps.wav" },
	{ SBufFootstepsInWater,	-1,					AL_TRUE,	(char *)"sounds/FootstepsInWater.wav" },
	// Sounds for player oral sounds
	{ SBufPlayerJump,		SSrcPlayerVoice, 	AL_FALSE,	(char *)"sounds/jump.wav" },
	// Sounds for player actions
	{ SBufBlockBuild,		-1,					AL_FALSE,	(char *)"sounds/sound58.wav" },
	{ SBufBlockRemove,		-1,					AL_FALSE,	(char *)"sounds/sound84.wav" },
	// TODO:Sounds from monsters in the surrounding
	{ SBufMonsterGrunt,		-1,					AL_FALSE,	(char *)"sounds/creature-growl-01.wav" },
	{ SBufMonsterGrowl,		-1,					AL_FALSE,	(char *)"sounds/creature-growl-02.wav" },
	// TODO:Sounds from players in the surrounding
	{ SBufOtherFootSteps,	-1, 				AL_TRUE,	(char *)"sounds/Footsteps.wav" },
	// Sound effects for environmental sounds
	{ SBufRunningWater,		-1,					AL_TRUE,	(char *)"sounds/RunningWater.wav" },
	{ SBufWind,				-1,					AL_TRUE,	(char *)"sounds/wind.wav" },
	{ SBufTeleport,			-1,					AL_TRUE,	(char *)"sounds/TeleportStation.wav" },
	// Sound source for triggered events.
	// TODO: This should not be assigned to a source here, nor should the buffer be loaded.
	{ SBufTrigEvent,		SSrcTrigEvent,		AL_FALSE,	(char *)"sounds/wind.wav" },
	// Other sound effects
	{ SBufLevelUp,			SSrcLevelUp, 		AL_FALSE,	(char *)"sounds/Sound Effects - Tada (fanfare) 01.wav" },
	{ SBufHealSelf,			SSrcHealSelf, 		AL_FALSE,	(char *)"sounds/heartbea-dr-2063.wav" },
	{ SBufInterfacePing,	SSrcInterfacePing,	AL_FALSE,	(char *)"sounds/chime.wav" },
};

/* This is the configuration data for the soundtrack */
SoundControl::MusicList SoundControl::fSoundtrack[] = {
	/* id,		Music Type				File */
	{ "Menu",	SMusicModeMenu, 		(char *)"sounds/music/One Man's Courage.ogg" },
	{ "C001",	SMusicModeCombat,		(char *)"sounds/music/Escape.ogg" },
	{ "T001",	SMusicModeTourist,		(char *)"sounds/music/05 - Box of Chocolates.ogg" },
	{ "T002",	SMusicModeTourist,	 	(char *)"sounds/music/Arcadia.ogg" },
	{ "T003",	SMusicModeTourist,	 	(char *)"sounds/music/Basement Floor.ogg" },
	{ "T004",	SMusicModeTourist,	 	(char *)"sounds/music/Gnarled Situation.ogg" },
	{ "T005",	SMusicModeTourist,	 	(char *)"sounds/music/Interloper.ogg" },
	{ "T006",	SMusicModeTourist,	 	(char *)"sounds/music/Private Reflection.ogg" },
	{ "T007",	SMusicModeTourist,	 	(char *)"sounds/music/Supernatural.ogg" },
	{ "T008",	SMusicModeTourist,	 	(char *)"sounds/music/The Chamber.ogg" },
	{ "T009",	SMusicModeTourist,	 	(char *)"sounds/music/Virtutes Instrumenti.ogg" },
	{ "T010", 	SMusicModeTourist,	 	(char *)"sounds/music/Darkness is Coming.ogg" },
	{ "C002", 	SMusicModeCombat,		(char *)"sounds/music/08 - Bringer Of War.ogg" },
};

SoundControl::TrigSoundItem SoundControl::fTrigSoundList[] = {
	// Identifier, file name, description for activator configuration window
	{ "GRW1", "sounds/creature-growl-01.wav", "Growl" },
	{ "GRW2", "sounds/lion-01.wav", "Growl, lion" },
	{ "CAT1", "sounds/cat-01.wav", "Cat meow" },
	{ "SCR1", "sounds/people086.wav", "Scream" },
	{ "THR1", "sounds/thunder001.wav", "Thunder" },
	{ "APL1", "sounds/applause-light-02.wav", "Light applause" },
	{ "CHR1", "sounds/cheer-01.wav", "Cheer" },
	{ "WHI1", "sounds/factorywhistle-01.wav", "Factory whistle" },
	{ "DCK1", "sounds/duck-01.wav", "Duck" },
	{ "PCK1", "sounds/peacock.wav", "Peacock" },
	{ "EAQ1", "sounds/earthquake-02.wav", "Earthquake" },
	{ "LAV1", "sounds/lava-03.wav", "Lava" },
	{ "EXP1", "sounds/explosion-01.wav", "Explosion" },
	{ "HIT1", "sounds/hit-02.wav", "Hit" },
	{ "DRC1", "sounds/door-creak-01.wav", "Door creaking" },
	{ "DRS1", "sounds/door-shut-03.wav", "Door shut" },
	{ "ICE1", "sounds/ice-cracking-01.wav", "Ice cracking" },
	{ "SNK1", "sounds/sink-empty-01.wav", "Sink empty" },
	{ "BCH1", "sounds/beach-04.wav", "Beach" },
	{ "JNG1", "sounds/jungle-03.wav", "Jungle" },
	{ "SWM1", "sounds/swamp-04.wav", "Swamp" },
	{ "WND1", "sounds/wind-05.wav", "Wind" },
	{ "FAIL", "sounds/Small_Bo-Public_D-323.wav", "Failures" },
	// A null pointer indicates that the rest, from now on, are "private" sounds that may not be used in construction mode
	{ "BOOM", "sounds/Drum_Ech-E77-7332.wav", 0 },
	{ "POT1", "sounds/PotionDrop.wav", 0 },
	{ "WPN1", "sounds/Donk-Public_d-336.wav", 0 },
	{ "ARM1", "sounds/ArmorDrop.wav", 0 },
	{ "S001", "sounds/ScrollDrop.wav", 0 },
	{ "PLP1", "sounds/plopp1.wav", 0 },
};

/*
** INITIALIZATION
*/
SoundControl::SoundControl() : fFeetInWater(false), fFeetInAir(false), fUnderWater(false), fEcho(false), fMusicOn(false), fMusicIsPlaying(false),
	fMusicFadeOut(false), fAudioEnabled(false), fEAXPresent(false), fALMaxSources(0), oggFile(0)
{
	unsigned int i;

	fALinitialized=false; MusicChangeTimeOut=0; fPlayerRunning=false;
	WasRunning=false; WasInWater=false; WasUnderWater=false; WasInAir=false;

	// Clear buffers and sources
	for (i=0; i<NELEM(fBuffers); i++)
		fBuffers[i] = 0;
	for (i=0; i<NELEM(fSources); i++)
		fSources[i] = 0;

	for( unsigned idx=0; idx<4; idx++ ) {
		fRequestedTrigSound[idx] = 0;
	}

	fNumTrigSounds = NELEM(fTrigSoundList);
	fRequestedSound = SNone;
	pthread_cond_init(&fCondLock, 0);
	pthread_mutex_init(&fMutex, 0);
	pthread_attr_init(&fAttr);
}

// SoundControl initialization
void SoundControl::Init(void) {
	int idx;

	//TODO: Error handling - if OpenAL can not be initialized, allow the game to be played anyway?

	// Initialize Settings
	fMusicVolumePref = ((float)gOptions.fMusicVolume)/100.0f;
	fMusicVolume = fMusicVolumePref;
	fMusicOn = gOptions.fMusicOn == 1;

	// Initialize data structures
	for( idx=0; idx < MAX_CREATURES; idx++ ) {
		KnownCreatures[idx].type = SNoCreature;
		KnownCreatures[idx].attachedGruntSource = -1;
		KnownCreatures[idx].attachedMoveSource = -1;
		KnownCreatures[idx].id = 0;
		KnownCreatures[idx].type = SNoCreature;
	}

	for( idx=0; idx < MAX_ENVIRONMENTITEMS; idx++ ) {
		EnvironmentSounds[idx].type = SNoCreature;
		EnvironmentSounds[idx].id = 0;
		EnvironmentSounds[idx].attachedEnvironmentSource = -1;
		EnvironmentSounds[idx].type = SNoCreature;
	}

	for( idx=0; idx < SNumCreatureSrc; idx++ )
		fCreatureSourceFree[idx] = true;

	for( idx=0; idx < SNumEnvSrc; idx++ )
		fEnvironmentSourceFree[idx] = true;

	// Initialize OpenAL
	alutInit(NULL, 0);
	fAudioEnabled = alGetError() != AL_NO_ERROR;
	if(fAudioEnabled) {
		auto &ss = View::gErrorManager.GetStream(false, false);
		ss << "SoundControl: alutInit failed!";
		return;
	}

	// Check for ALC_EXT_EFX and store state
	fEAXPresent = alcIsExtensionPresent(alcGetContextsDevice(alcGetCurrentContext()), "ALC_EXT_EFX") == AL_TRUE;

	// Detect maximum number of sources. For hardware rendering, this is the number of available sources on the
	// sound card (i.e. for Windows). For software rendering under Linux this seems to return 255.
	ALCint size;
	alcGetIntegerv( alcGetContextsDevice(alcGetCurrentContext()), ALC_ATTRIBUTES_SIZE, 1, &size);
	std::vector<ALCint> attrs(size);
	alcGetIntegerv( alcGetContextsDevice(alcGetCurrentContext()), ALC_ALL_ATTRIBUTES, size, &attrs[0] );
	for(int i=0; i<(int)attrs.size(); ++i) {
		if( attrs[i] == ALC_MONO_SOURCES ) {
			fALMaxSources = attrs.at(i+1);
		}
	}

	// Generate sources. Buffers are generated when the file is loaded.
	alGenSources(SNumSources, fSources);
	CheckForError("alGenSources failed!");

	// Fill sound buffers and connect sources as configured
	// Initialize settings for the sources
	for( idx=0; idx < (int)NELEM(fSoundBuffers); idx++ ) {
#ifdef MUSIC_DEBUG
		printf( "Loading %i - %s\n", idx, fSoundBuffers[idx].fileName );
#endif
		fBuffers[fSoundBuffers[idx].bufferId] = alutCreateBufferFromFile(fSoundBuffers[idx].fileName);

		CheckForError("alutCreateBufferFromFile failed");
		//if(alGetError() != AL_NO_ERROR) {
		//	ErrorDialog("SoundControl: alutCreateBufferFromFile for buffer %i failed!\n", fSoundBuffers[idx].bufferId);
		//}

		if( fSoundBuffers[idx].sourceId >= 0 ) {
#ifdef MUSIC_DEBUG
			printf( "Attaching buf: %i to src: %i\n", fSoundBuffers[idx].bufferId, fSoundBuffers[idx].sourceId );
#endif

			alSourcei(fSources[fSoundBuffers[idx].sourceId], AL_BUFFER, fBuffers[fSoundBuffers[idx].bufferId]);
			CheckForError("SC: Buffering");
			SrcBaseConfig(fSources[fSoundBuffers[idx].sourceId]);
			CheckForError("SC: Init sound buffers");
			alSourcei(fSources[fSoundBuffers[idx].sourceId], AL_LOOPING, fSoundBuffers[idx].looping);
			CheckForError("SC: Set looping for sound buffer");
		}
	} // Loop over buffers to load audio and set up sources as configured

	// Genereate generic sources
	alGenSources(SNumEnvSrc, fEnvironmentSources);
	alGenSources(SNumCreatureSrc, fCreatureSources);

	// Setup Listener location
	ALfloat Ori[] = {0.0f,0.0f,-1.0f,0.0f,1.0f,0.0f};
	alListener3f(AL_POSITION, 0.0,0.0,0.0);
	alListener3f(AL_VELOCITY, 0.0,0.0,0.0);
	alListenerfv(AL_ORIENTATION, Ori);
	CheckForError("Init listener position");

	// Generate buffers for the background music
	alGenBuffers(1,&fBuffers[SMusicBuffer1]);
	alGenBuffers(1,&fBuffers[SMusicBuffer2]);
	CheckForError("Generate Music buffers");

	// Music sources are already created above, configure music source
	alSource3f(fSources[SMusicSource], AL_POSITION, 0.0,0.0,0.0 );
	alSource3f(fSources[SMusicSource], AL_VELOCITY, 0.0,0.0,0.0 );
	alSource3f(fSources[SMusicSource], AL_DIRECTION, 0.0,0.0,0.0 );
	alSourcef(fSources[SMusicSource], AL_GAIN, fMusicVolume);
	alSourcef(fSources[SMusicSource], AL_ROLLOFF_FACTOR, 0.0 );
	alSourcei(fSources[SMusicSource], AL_SOURCE_RELATIVE, AL_TRUE );
	CheckForError("Music Source Configuration");

	// Set music modes
	fNextMusicMode = SMusicModeNone;
	fCurrentMusicMode = SMusicModeNone;
	fRequestedMusicMode = SMusicModeNone;
	fMusicIsPlaying = false;

	// Count songs of mode types
	for( idx=0; idx<SNumMusicModes; idx++ ) {
		fSongCount[idx] = 0;
	}

	for( idx=0; idx<(int)NELEM(fSoundtrack); idx++ ) {
		fSongCount[fSoundtrack[idx].musicType]++;
	}

	/* For portability, explicitly create threads in a joinable state */
	pthread_attr_setdetachstate(&fAttr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&fThread, &fAttr, SoundControl::Thread, (void *)this); // This starts the child thread
	fALinitialized = true;
}

/*
** SHUTDOWN FUNCTIONS
*/
// Destructor, cleans up pthread and then OpenAL
SoundControl::~SoundControl(void) {
	if (!fALinitialized)
		return; // Early termination before Init() has been called.

	this->RequestSound(SoundControl::STerminate, true);	// This tells the child process to stop
	fALinitialized = false;                         // This class is no longer in a functional state
	pthread_join(fThread, NULL);					// Wait for child process to terminate
	pthread_cond_destroy(&fCondLock);
	pthread_mutex_destroy(&fMutex);
	pthread_attr_destroy(&fAttr);

	if( this->fMusicOn )
		this->MusicStop();

	// TODO: If music was forcefully muted, it needs to be stopped properly just about here

	alDeleteSources(NELEM(fSources), fSources);
	CheckForError("Exit - DeleteSources");
	alDeleteSources(SNumEnvSrc, fEnvironmentSources);
	CheckForError("Exit - DeleteSources - Env");
	alDeleteSources(SNumCreatureSrc, fCreatureSources);
	CheckForError("Exit - DeleteSources - Creatures");

	// NOTE: If this call fails it is probably related to the fact that there is an item
	// in the enum that has not been assigned to a buffer because a sound has not been loaded.
	alDeleteBuffers(SNumBuffers, fBuffers);
	CheckForError("Exit - DeleteBuffers");

	/* Kill Audio */
#ifdef MUSIC_DEBUG
	printf("Killing ALUT\n");
#endif
	alutExit();

	fALinitialized = false;
}

/*
** HELPER FUNCTIONS TO SHORTEN CODE
*/
void SoundControl::SrcBaseConfig(ALuint srcId) {
	alSourcef(srcId, AL_PITCH, 1.0f);
	alSourcef(srcId, AL_GAIN, 1.0f);
	alSource3f(srcId, AL_POSITION, 0.0,0.0,0.0);
	alSource3f(srcId, AL_VELOCITY, 0.0,0.0,0.0);
}

/*
** PUBLIC FUNCTIONS TO REQUEST SOUNDS AND SOUND EFFECTS
*/
// This is executed in the main thread
// Request one or more sounds
void SoundControl::RequestSound(Sound s, bool force) {
	if (!fALinitialized) // TEST TODO: Does not seem to help
		return; // Early termination before Init() has been called or after the destructor has run.

	if (force)
		pthread_mutex_lock(&fMutex);
	else if (pthread_mutex_trylock(&fMutex) != 0)
		return;

	fRequestedSound |= s;						// Update the message to the child thread
	pthread_cond_signal(&fCondLock);			// Signal the child thread that there is something
	pthread_mutex_unlock(&fMutex);				// Unlock the mutex; this will wakeup the child thread
}

// Request a sound to be triggered by an activator or similar
void SoundControl::RequestTrigSound(const char *soundId) {
	// Try doing this without mutex etc.
	// The only thing that can happen is that no sound, or the wrong sound, is played
	for( int x=0; x<4; x++ )
		fRequestedTrigSound[x] = soundId[x];
}

// Executed in the main thread
// Set a sound effect status, sound modifiers that can be on or off
void SoundControl::SetSoundFxStatus(SoundFxStatus soundfx, bool status) {
	pthread_mutex_lock(&fMutex);						// Lock the mutex, to get access to the message variable
	switch(soundfx) {
	case SPlayerRunning:
		fPlayerRunning = status;				// Update the message to the child thread
		break;
	case SPlayerFeetInWater:
		fFeetInWater = status;
		break;
	case SEnvironmentUnderWater:
		fUnderWater = status;
		break;
	case SEnvironmentEcho:
		fEcho = status;
		break;
	case SPlayerInAir:
		fFeetInAir = status;
		break;
	default:									// Unknown message
		break;
	};
	pthread_cond_signal(&fCondLock);			// Signal the child thread that there is something
	pthread_mutex_unlock(&fMutex);				// Unlock the mutex; this will wakeup the child thread
}

/*
** MUSIC FUNCTIONS - PUBLIC
*/
void SoundControl::RequestMusicMode(MusicMode music) {
	pthread_mutex_lock(&fMutex);				// Lock the mutex, to get access to the message variable
	fRequestedMusicMode = music;
	pthread_cond_signal(&fCondLock);			// Signal the child thread that there is something
	pthread_mutex_unlock(&fMutex);				// Unlock the mutex; this will wakeup the child thread
}

// SwitchMusicStatus - used to turn music on/off
void SoundControl::SwitchMusicStatus(void) {
	fMusicOn = !fMusicOn;						// Do this without locking the mutex

	// Print debugdata to stdout, mimics function printing OpenGL context
	if (fEAXPresent) {
		printf("SoundControl: ALC EXT EFX present\n");
	} else {
		printf("SoundControl: ALC EXT EFX not present\n");
	}

	printf("SoundControl: max mono sources: %i\n", fALMaxSources);
	printf("SoundControl: used sources:     %i [std] %i [env] %i [creature]\n", SNumSources, SNumEnvSrc, SNumCreatureSrc);
}

/*
** MUSIC FUNCTIONS - PRIVATE
*/
// Choose a song based on the distance from the start.
static int MusicChoice(int count) {
	// Idea is to switch song every 90 blocks. Also divide by 100 to compensate for resolution
	int y = int(Model::gPlayer.y / (100 * 100));
	if (y<0)
		y = 1-y;
	int z = int(Model::gPlayer.z / (100 * 100));
	if (z<0)
		z = 1-z;
	int dist = y+z;
	int song = (dist-1) % (count-1)+1; // Gives a value from 1 to 'count'-1.
	if (y == 0 && z == 0)
		song = 0; // Use the first song for the starting area, and never again
	// printf("SoundControl MusicChoice distance %d, song %d\n", dist, song);
	return song;
}

// MusicStream - Read the next section of music data
bool SoundControl::MusicStream(ALuint buffer) {
#define BUFFER_SIZE (8192*16) // 128kb
	char data[BUFFER_SIZE]; // This is the temporary buffer
	int size=0;
	int section, result;
	ALenum error;

	while( size < BUFFER_SIZE ) {
		result=ov_read(&oggStream,data+size,BUFFER_SIZE-size,0,2,1,&section);
		if(result>0)
			size += result;
		else if(result<0) {
			printf("SoundControl: oggRead failure section %i, size %i\n", section, size);
		} else
			break;
	}

	if(size==0)
		return false;

	CheckForError("Music before alBufferData!");

	alBufferData(buffer,fMusicFormat,data,size,vorbisInfo->rate);
	// This gives more specific error that the CheckForError test, leave it as it is
	error = alGetError();
	if(error != AL_NO_ERROR) {
		printf("SoundControl: Stream (%i) alBufferData failed, rate %li, size %i! Error: %i\n", buffer, vorbisInfo->rate, size, error);
	}

	return true;
}

// MusicPlaying - Check if music is playing or not
bool SoundControl::MusicPlaying(void) {
	ALenum state;

	alGetSourcei(fSources[SMusicSource],AL_SOURCE_STATE, &state);
	return (state==AL_PLAYING);
}

// MusicPlayback - Start music playback
bool SoundControl::MusicPlayback(void) {
	if( MusicPlaying() )
		return true;

	if( !MusicStream(fBuffers[SMusicBuffer1]) )
		return false;

	if( !MusicStream(fBuffers[SMusicBuffer2]) )
		return false;

	alSourceQueueBuffers(fSources[SMusicSource],2,&fBuffers[SMusicBuffer1]);
	CheckForError("Music QueueBuffers failed");

	alSourcePlay(fSources[SMusicSource]);
	CheckForError("Music SourcePlay failed");

	return true;
}

// MusicUpdate - Update music buffers and read more data if necessary
bool SoundControl::MusicUpdate(void) {
	int processed;
	bool active = true;

	alGetSourcei(fSources[SMusicSource],AL_BUFFERS_PROCESSED,&processed);
	CheckForError("Music GetSourcei processed");

	while(processed--) {
		ALuint buffer;
		alSourceUnqueueBuffers(fSources[SMusicSource],1,&buffer); // local buffer

		CheckForError("MusicUpdate Unqueue Buffer");
		//if(alGetError() != AL_NO_ERROR)
		//printf("SoundControl:MusicUpdate Unqueue Buffer %i failed! --> %i\n", buffer, processed);

		active = MusicStream(buffer);
		alSourceQueueBuffers(fSources[SMusicSource],1,&buffer); // local buffer

		CheckForError("MusicUpdate Queue Buffer");
		//if(alGetError() != AL_NO_ERROR)
		//printf("SoundControl:MusicUpdate Queue Buffer %i failed! --> %i\n", buffer, processed);
	}

	return active;
}

// OpenOggStream - Open a music file based on a requested music mode
bool SoundControl::OpenOggStream(MusicMode mode) {
	int x, selection;

	// TODO: Intelligent selection of music
	if (mode == SMusicModeTourist)
		selection = MusicChoice(fSongCount[mode]);
	else if (fSongCount[mode] != 0)
		selection = rand()%fSongCount[mode];
	else
		return false; // Quitting is a better option than to get a division by zero

	// This will open the first soundtrack for the current mode
	for( x=0; x<(int)NELEM(fSoundtrack); x++ ) {
		if (fSoundtrack[x].musicType == mode) {
			if (selection==0) {
#ifdef MUSIC_DEBUG
				printf("Selecting song %s for mode %i\n", fSoundtrack[x].fileName, mode);
#endif
				oggFile = fopen(fSoundtrack[x].fileName,"rb");
				break;
			} else {
				selection--;
			}
		}
	}

	if( !oggFile ) {
		return false;
	}

	if( ov_open(oggFile, &oggStream, NULL, 0) < 0 ) {
		fclose(oggFile);
		return false;
	}

	vorbisInfo = ov_info(&oggStream, -1);

	if( vorbisInfo->channels==1 )
		fMusicFormat = AL_FORMAT_MONO16;
	else
		fMusicFormat = AL_FORMAT_STEREO16;

	return true;
}

// CloseOggStream - Close the file used for music streaming
void SoundControl::CloseOggStream(void) {
	ov_clear(&oggStream);
}

// MusicStop - Stop the music currently playing
void SoundControl::MusicStop(void) {
	alSourceStop(fSources[SMusicSource]);
	alSourceUnqueueBuffers(fSources[SMusicSource],1,&fBuffers[SMusicBuffer1]); // local buffer
	alSourceUnqueueBuffers(fSources[SMusicSource],1,&fBuffers[SMusicBuffer2]); // local buffer
	CloseOggStream();
}

// HandleMusic - The main loop for playing music
// TODO: Since glfw has been included we might as well use the same timer for the fight music
// TODO: Replace all unconditional errors with better failsafe
void SoundControl::HandleMusic(void) {
	static double delayMusic = 0.0;

	// Music delayed now and then, but only for tourist mode.
	if (fNextMusicMode != SMusicModeTourist)
		delayMusic = 0.0; // Reset delay
	if (glfwGetTime() < delayMusic || fNextMusicMode==SMusicModeNone)
		return;

	// If no music is playing, start playback, if Music is desired by the user
	if( !fMusicIsPlaying ) {
		OpenOggStream(fNextMusicMode);
		fCurrentMusicMode = fNextMusicMode;
		fMusicFadeOut = false;

		switch (fCurrentMusicMode) {
		case SMusicModeCombat:
			// When in combat music, we want the music to change back when we're not fighting anymore
			MusicChangeTimeOut = MUSIC_TIMEOUT_VAL;
			fNextMusicMode = SMusicModeTourist;
			break;
		default:
			break;
		}

		if( !MusicPlayback() ) {
			printf("SoundControl: Music failed to start!\n");
		} else {
			fMusicIsPlaying = true;
		}
	}

	if( MusicUpdate() ) {
		// Music is playing
		if( !MusicPlaying() ) {
			if( !MusicPlayback() ) {
				printf("SoundControl: Music abruptly stopped!\n");
			} else {
				printf("SoundControl: Stream failed!\n");
			}
		}
	} else {
		// Music has finished, clean up
		MusicStop();
		fMusicIsPlaying = false; // This will cause music to be restarted next time around...
		// TODO: This is not a good way to get silence, will cause error messages
		delayMusic = glfwGetTime() + 180.0; // Wait a couple of minutes until next song
	}

	if( fMusicFadeOut ) {
		if( fMusicVolume>0.0f ) {
			fMusicVolume -= fMusicVolumePref/7.0f; // TODO: 7 samples to fade out -> needed?
			if( fMusicVolume>0.0f )
				alSourcef(fSources[SMusicSource], AL_GAIN, fMusicVolume); // Set next volume
			else
				alSourcef(fSources[SMusicSource], AL_GAIN, 0.0f); // Set next volume

			CheckForError("Music Fade out");
		} else {
			fMusicFadeOut = false;
			fMusicIsPlaying = false;
			fMusicVolume = fMusicVolumePref;
			MusicStop();
			CheckForError("Music Stop");
			alSourcef(fSources[SMusicSource], AL_GAIN, fMusicVolume); // Set next volume
			CheckForError("Music Fade out");
		}
	}

	// Handle music time out
	if(MusicChangeTimeOut>0) {
		if(--MusicChangeTimeOut<=0) {
			fMusicFadeOut = true; // Trigger music change
		}
	}
}

// Set creature position
void SoundControl::CreatureSetPosition( int id, float x, float y, float z ) {
	KnownCreatures[id].positionX = x;
	KnownCreatures[id].positionY = y;
	KnownCreatures[id].positionZ = z;

	KnownCreatures[id].distance = x*x+y*y+z*z;
}

// Set audio position
void SoundControl::CreatureSetGruntSourcePosition(int id, float x, float y, float z) {
	// Relative listener position
#ifdef MUSIC_DEBUG
	printf("Setting grunt pos creature %i in source %i to (%3.2f,%3.2f,%3.2f)\n", id, KnownCreatures[id].attachedGruntSource, KnownCreatures[id].positionX,KnownCreatures[id].positionY, KnownCreatures[id].positionZ);
#endif
	alSource3f(fCreatureSources[KnownCreatures[id].attachedGruntSource], AL_POSITION, x/SOUND_DISTANCE_MOD, z/SOUND_DISTANCE_MOD, y/SOUND_DISTANCE_MOD);
}

// Set audio position
void SoundControl::CreatureSetMoveSourcePosition(int id, float x, float y, float z) {
	// Relative listener position
#ifdef MUSIC_DEBUG
	printf("Setting move pos creature %i in source %i to (%3.2f,%3.2f,%3.2f)\n", id, KnownCreatures[id].attachedMoveSource, KnownCreatures[id].positionX,KnownCreatures[id].positionY, KnownCreatures[id].positionZ);
#endif
	alSource3f(fCreatureSources[KnownCreatures[id].attachedMoveSource], AL_POSITION, x/SOUND_DISTANCE_MOD, z/SOUND_DISTANCE_MOD, y/SOUND_DISTANCE_MOD);
}

/*
** HANDLING SOUNDS FROM OTHER CREATURES - PUBLIC
*/
void SoundControl::SetCreatureSound(SoundObject creatureType,unsigned long id, float dx, float dy, float dz, bool dead, float size) {
	int x, firstFree;
	bool isUpdated;

	// Locking might not be necessary but if this data is updated at the same time
	// as the timer task runs, it could cause strange effects?
	pthread_mutex_lock(&fMutex);		// Lock the mutex, to get access to the message variable
	isUpdated = false;
	firstFree = -1;

	// Loop over creatures
	for( x=0; x<(int)NELEM(KnownCreatures); x++ ) {
		if ((KnownCreatures[x].id == id) && (KnownCreatures[x].type == creatureType)) {
			// Update monster
			isUpdated = true;
			CreatureSetPosition( x, dx, dy, dz );

			KnownCreatures[x].mute = dead; // If the monster is dead, it will be silent

			// In order to save some time, we will save last position only in the time task
			break;
		} else if ((firstFree==-1)&&(KnownCreatures[x].type == SNoCreature)) {
			firstFree=x;
		}
	}

	if (!isUpdated) {
		if( firstFree == -1 ) {
		} else {
			//printf("New monster %li of type %i in pos %i\n", id, creatureType, firstFree);
			KnownCreatures[firstFree].id = id;
			KnownCreatures[firstFree].type = creatureType;
			KnownCreatures[firstFree].size = size;
			KnownCreatures[firstFree].attachedGruntSource = -1;
			KnownCreatures[firstFree].attachedMoveSource = -1;
			CreatureSetPosition( firstFree, dx, dy, dz );
			KnownCreatures[firstFree].lastX = dx;
			KnownCreatures[firstFree].lastY = dy;
			KnownCreatures[firstFree].lastZ = dz;
			KnownCreatures[firstFree].distance = dx*dx+dy*dy+dz*dz;
			KnownCreatures[firstFree].mute = dead; // If the monster is already dead, it will be silent
		}
	}

	// Don't signal the thread to do anything, it will handle the creatures in the time task
	pthread_mutex_unlock(&fMutex);				// Unlock the mutex
}

void SoundControl::RemoveCreatureSound(SoundObject creatureType,unsigned long id) {
	int x;

	pthread_mutex_lock(&fMutex);		// Lock the mutex, to get access to the message variable
	// Loop over creatures
	for( x=0; x<(int)NELEM(KnownCreatures); x++ ) {
		if ((KnownCreatures[x].id == id) && (KnownCreatures[x].type == creatureType)) {
			//printf("Creature pos %i removed\n",x);
			KnownCreatures[x].type = SToBeRemoved;
			break;
		}
	}

	// Don't signal the thread to do anything, it will handle the creatures in the time task
	pthread_mutex_unlock(&fMutex);				// Unlock the mutex
}

/*
** HANDLING SOUNDS FROM OTHER CREATURES - PRIVATE
*/
void SoundControl::HandleCreatures(void) {
	int idx;
	int FreeSource;
	bool CreatureIsMoving;

	// Handle Creatures (#41 & #158)
	for( idx=0; idx<(int)NELEM(KnownCreatures); idx++ ) {
		// First see if creature should be removed
		if (KnownCreatures[idx].type == SToBeRemoved) {
			KnownCreatures[idx].type = SNoCreature;
			if( KnownCreatures[idx].attachedGruntSource != -1 ) {
				alSourceStop(fCreatureSources[KnownCreatures[idx].attachedGruntSource]);
				CheckForError("Creature removal: sound stop");
				fCreatureSourceFree[KnownCreatures[idx].attachedGruntSource] = true;
				CreatureSetGruntSourcePosition(idx, 0.0f, 0.0f, 0.0f);
				CheckForError("Creature removal: position");
				KnownCreatures[idx].attachedGruntSource = -1;
			}
#ifdef MUSIC_DEBUG
			printf("Creature pos %i removed\n",idx);
#endif
			continue; // No need to process this creature further - it's gone
		}

		// Unless position has changed considerably, do not update position
		CreatureIsMoving =	(abs(KnownCreatures[idx].positionX - KnownCreatures[idx].lastX) > 10.0) ||
		                    (abs(KnownCreatures[idx].positionY - KnownCreatures[idx].lastY) > 10.0) ||
		                    (abs(KnownCreatures[idx].positionZ - KnownCreatures[idx].lastZ) > 10.0);

		if( CreatureIsMoving ) {
			//printf("Creature %i pos updated: %3.3f o %3.3f o %3.3f\n",idx, KnownCreatures[idx].positionX, KnownCreatures[idx].positionY, KnownCreatures[idx].positionZ);
			KnownCreatures[idx].lastX = KnownCreatures[idx].positionX;
			KnownCreatures[idx].lastY = KnownCreatures[idx].positionY;
			KnownCreatures[idx].lastZ = KnownCreatures[idx].positionZ;
		}

		// Clean up errors
		CheckForError("TSExec after creature cleanup");

		// Monster muted, probably dead
		if( KnownCreatures[idx].mute && KnownCreatures[idx].attachedGruntSource != -1 ) {
#ifdef MUSIC_DEBUG
			printf("Creature %i dead - removing source %i\n", idx, KnownCreatures[idx].attachedGruntSource);
#endif
			alSourceStop(fCreatureSources[KnownCreatures[idx].attachedGruntSource]);
			CheckForError("Creature dead: sound stop");
			fCreatureSourceFree[KnownCreatures[idx].attachedGruntSource] = true;
			CreatureSetGruntSourcePosition(idx, 0.0f, 0.0f, 0.0f);
			CheckForError("Creature dead: position");
			KnownCreatures[idx].attachedGruntSource = -1;
		}

		// TODO: Add new sounds and modifiers for creatures
		if (KnownCreatures[idx].attachedGruntSource != -1) {
			// Check if sound is finished, in which case deallocate source
			ALenum state;
			alGetSourcei(fCreatureSources[KnownCreatures[idx].attachedGruntSource],AL_SOURCE_STATE, &state);

			// See if a creature is already playing something,
			// in which case the position should be updated
			if( state!=AL_PLAYING ) {
				// If sound done, remove monster
#ifdef MUSIC_DEBUG
				printf("Remove %i from source %i\n", idx, KnownCreatures[idx].attachedGruntSource);
#endif
				alSourceStop(fCreatureSources[KnownCreatures[idx].attachedGruntSource]);
				CheckForError("Creature growl stop");
				fCreatureSourceFree[KnownCreatures[idx].attachedGruntSource] = true;
				CreatureSetGruntSourcePosition(idx, 0.0f, 0.0f, 0.0f); // Prevent bad position next time
				CheckForError("Creature growl stop pos");
				KnownCreatures[idx].attachedGruntSource = -1;
			} else {
				// If sound is still running, update position of this monster
				CreatureSetGruntSourcePosition(idx, KnownCreatures[idx].positionX, KnownCreatures[idx].positionY, KnownCreatures[idx].positionZ);
				CheckForError("Set creature position");
			}
		} else {
			// See if this item wants to grunt :-)
			int GruntType = rand()%2; // TODO: Fix more sounds
			bool DoGrunt = (rand()%1000) <= 10; // TODO: Monster wants to grunt?

			for( FreeSource=0; FreeSource<SNumCreatureSrc; FreeSource++ ) {
				if( fCreatureSourceFree[FreeSource] ) {
					break;
				}
			}

			// Trigger grunt based on chance, monster, not dead and within maximum distance
			if( DoGrunt && fCreatureSourceFree[FreeSource] && KnownCreatures[idx].type == SMonster && !KnownCreatures[idx].mute &&
			        (KnownCreatures[idx].distance < SOUND_DISTANCE_MAX) ) {
#ifdef MUSIC_DEBUG
				printf("Assigned Source %i to creature %i\n", FreeSource, idx);
#endif
				fCreatureSourceFree[FreeSource] = false;
				KnownCreatures[idx].attachedGruntSource = FreeSource;
				// TODO: Prepare for dynamic loading of grunt sounds
				// If each grunt source has a buffer attached, an unlimited number of monster sounds can be used.
				// This will be useful once the monsters are defined by the server.
				if( GruntType == 0 )
					alSourcei(fCreatureSources[FreeSource], AL_BUFFER, fBuffers[SBufMonsterGrunt]);
				else
					alSourcei(fCreatureSources[FreeSource], AL_BUFFER, fBuffers[SBufMonsterGrowl]);
				CheckForError("Set creature sound properties: BUFFER");

				// TODO: Volume and pitch to get different sounds
				float PitchModifier = 0.5f/(KnownCreatures[idx].size + 0.05f);
				float VolumeModifier = 0.0f; // KnownCreatures[idx].size;

				alSourcef(fCreatureSources[FreeSource], AL_PITCH, 0.8f+PitchModifier);
				alSourcef(fCreatureSources[FreeSource], AL_GAIN, 1.0f+VolumeModifier);

				// Set grunt source
				CreatureSetGruntSourcePosition(idx, KnownCreatures[idx].positionX,KnownCreatures[idx].positionY,KnownCreatures[idx].positionZ);
				CheckForError("Set creature sound properties: POSITION");

				alSource3f(fCreatureSources[FreeSource], AL_VELOCITY, 0.0,0.0,0.0);
				CheckForError("Set creature sound properties: VELOCITY");
				alSourcei(fCreatureSources[FreeSource], AL_LOOPING, AL_FALSE);
				CheckForError("Set creature sound properties: LOOPING");
				alSourcePlay(fCreatureSources[FreeSource]);
				CheckForError("Play creature sound");
			}
		} // End of else FOR "if (KnownCreatures[idx].attachedGruntSource != -1)"


		// Test movement if a movement source has been attached
		if (KnownCreatures[idx].attachedMoveSource != -1) {
			// Check if moving has stopped, in which case deallocate source
			// TODO: This is tricky, since CreatureIsMoving is declared above as a certain movement, can movement be lower?
			if (!CreatureIsMoving) {
#ifdef MUSIC_DEBUG
				printf("Remove %i from move source %i\n", idx, KnownCreatures[idx].attachedMoveSource);
#endif
				alSourceStop(fCreatureSources[KnownCreatures[idx].attachedMoveSource]);
				CheckForError("Creature move stop");
				fCreatureSourceFree[KnownCreatures[idx].attachedMoveSource] = true;
				CreatureSetMoveSourcePosition(idx,0.0f,0.0f,0.0f);
				CheckForError("Creature move stop pos");
				KnownCreatures[idx].attachedMoveSource = -1;

			} else { // Creature is moving, move it around
				CreatureSetMoveSourcePosition(idx,KnownCreatures[idx].positionX,KnownCreatures[idx].positionY,KnownCreatures[idx].positionZ);
				CheckForError("Set creature move sound properties: POSITION");
			}

		} else if (CreatureIsMoving) {
#ifdef MUSIC_DEBUG
			//printf("Creature %i has moved. It's a type %i of size %3.1f\n", idx, KnownCreatures[idx].type, KnownCreatures[idx].size);
#endif
			// See if another player is moving, make a sound!
			if(KnownCreatures[idx].type == SOtherPlayer) {
				for( FreeSource=0; FreeSource<SNumCreatureSrc; FreeSource++ ) {
					if( fCreatureSourceFree[FreeSource] ) {
						break;
					}
				}

				if( FreeSource >= SNumCreatureSrc )
					FreeSource = 0; // This will be false anyway

				// TODO: FreeSource can be outside of the bounds for free source array
				if( fCreatureSourceFree[FreeSource] && !KnownCreatures[idx].mute ) {
#ifdef MUSIC_DEBUG
					printf("Assigned Source %i to creature %i for movement\n", FreeSource, idx);
#endif
					fCreatureSourceFree[FreeSource] = false;
					KnownCreatures[idx].attachedMoveSource = FreeSource;
					alSourcei(fCreatureSources[FreeSource], AL_BUFFER, fBuffers[SBufFootsteps]);
					CheckForError("Set creature move sound properties: BUFFER");
					SrcBaseConfig(fCreatureSources[FreeSource]);
					CheckForError("Set creature move sound properties: BASE");
					CreatureSetMoveSourcePosition(idx,KnownCreatures[idx].positionX,KnownCreatures[idx].positionY,KnownCreatures[idx].positionZ);
					CheckForError("Set creature move sound properties: POSITION");
					alSourcei(fCreatureSources[FreeSource], AL_LOOPING, AL_TRUE); // Movement = loop
					CheckForError("Set creature move sound properties: LOOPING");
					alSourcePlay(fCreatureSources[FreeSource]);
					CheckForError("Play creature move sound");
				}

				// TODO: See if a monster is moving, make a sound!
			} else if(KnownCreatures[idx].type == SMonster) {
				// This will be useful when monsters are better defined, currently SoundControl knows very little about the monster
			}
		} // End of creature move source
	}
}

// Set audio position
void SoundControl::EnvironmentSetSourcePosition(int id, float x, float y, float z) {
	// Relative listener position
#ifdef MUSIC_DEBUG
	printf("Setting environment pos  %i in source %i to (%3.2f,%3.2f,%3.2f)\n", id, EnvironmentSounds[id].attachedEnvironmentSource, EnvironmentSounds[id].positionX,EnvironmentSounds[id].positionY, EnvironmentSounds[id].positionZ);
#endif
	alSource3f(fEnvironmentSources[EnvironmentSounds[id].attachedEnvironmentSource], AL_POSITION, x/SOUND_DISTANCE_MOD, z/SOUND_DISTANCE_MOD, y/SOUND_DISTANCE_MOD);
}

/*
** SET SOUNDS FROM THE ENVIRONMENT - Public
*/
void SoundControl::SetEnvironmentSound(SoundObject soundType,unsigned long id, float dx, float dy, float dz) {
	int x, firstFree;
	bool isUpdated;

	// Locking might not be necessary but if this data is updated at the same time
	// as the timer task runs, it could cause strange effects?
	pthread_mutex_lock(&fMutex);		// Lock the mutex, to get access to the message variable
	isUpdated = false;
	firstFree = -1;

	// Loop over environment sources
	for( x=0; x<(int)NELEM(EnvironmentSounds); x++ ) {
		if ((EnvironmentSounds[x].id == id) && (EnvironmentSounds[x].type == soundType)) {
			// Update Environment
#ifdef MUSIC_DEBUG
			printf("Found existing Env source for %li at %i\n", id, x);
#endif
			isUpdated = true;
			EnvironmentSounds[x].type = soundType;
			EnvironmentSounds[x].positionX = dx;
			EnvironmentSounds[x].positionY = dy;
			EnvironmentSounds[x].positionZ = dz;
			EnvironmentSounds[x].distance = dx*dx+dy*dy+dz*dz;
			EnvironmentSounds[x].timeOut = 2;
			// In order to save some time, we will save last position only in the time task
			break;
		} else if ((firstFree==-1)&&(EnvironmentSounds[x].type == SNoCreature)) {
			firstFree=x;
		}
	}

	if (!isUpdated) {
		if( firstFree == -1 ) {
		} else {
#ifdef MUSIC_DEBUG
			printf("New environment %li in pos %i\n", id, firstFree);
#endif
			EnvironmentSounds[firstFree].id = id;
			EnvironmentSounds[firstFree].type = soundType;
			EnvironmentSounds[firstFree].positionX = dx;
			EnvironmentSounds[firstFree].positionY = dy;
			EnvironmentSounds[firstFree].positionZ = dz;
			EnvironmentSounds[firstFree].lastX = dx;
			EnvironmentSounds[firstFree].lastY = dy;
			EnvironmentSounds[firstFree].lastZ = dz;
			EnvironmentSounds[firstFree].distance = dx*dx+dy*dy+dz*dz;
			EnvironmentSounds[firstFree].timeOut = 2;
		}
	}

	// Don't signal the thread to do anything, it will handle the creatures in the time task
	pthread_mutex_unlock(&fMutex);				// Unlock the mutex
}

/*
** REMOVE ENVIRONMENT SOUND - Public
*/
void SoundControl::RemoveEnvironmentSound(SoundObject soundType,unsigned long id) {
	int x;

	pthread_mutex_lock(&fMutex);	// Lock the mutex, to get access to the message variable
	// Loop over environment objects
	for( x=0; x<(int)NELEM(EnvironmentSounds); x++ ) {
		if ((EnvironmentSounds[x].id == id) && (EnvironmentSounds[x].type == soundType)) {
			//printf("Creature pos %i removed\n",x);
			EnvironmentSounds[x].type = SToBeRemoved;
			break;
		}
	}

	// Don't signal the thread to do anything, it will handle the creatures in the time task
	pthread_mutex_unlock(&fMutex);				// Unlock the mutex
};


/*
** HANDLING SOUNDS FROM ENVIRONMENT - PRIVATE
*/
void SoundControl::HandleEnvironment(void) {
	int idx;
	int FreeSource;
	bool SourceIsMoving;

	// Handle Environment Sounds (#401)
	for( idx=0; idx<(int)NELEM(EnvironmentSounds); idx++ ) {
		// Do not treat unassigned creatures
		if (EnvironmentSounds[idx].type == SNoCreature) {
			continue;
		}

		if (EnvironmentSounds[idx].timeOut-- <= 0) {
#ifdef MUSIC_DEBUG
			printf("TimeOut!\n");
#endif
			EnvironmentSounds[idx].type = SToBeRemoved;
		}

		// First see if creature should be removed
		if (EnvironmentSounds[idx].type == SToBeRemoved) {
			EnvironmentSounds[idx].type = SNoCreature;
			if( EnvironmentSounds[idx].attachedEnvironmentSource != -1 ) {
#ifdef MUSIC_DEBUG
				printf("Removing Environmental sound source attached to id %li\n",EnvironmentSounds[idx].id);
#endif
				alSourceStop(fEnvironmentSources[EnvironmentSounds[idx].attachedEnvironmentSource]);
				CheckForError("Environment removal: sound stop");
				fEnvironmentSourceFree[EnvironmentSounds[idx].attachedEnvironmentSource] = true;
				// TODO: Reset position of this sound to 0,0,0 may be a good idea?
				CheckForError("Environment removal: position");
				EnvironmentSounds[idx].attachedEnvironmentSource = -1;
			}
#ifdef MUSIC_DEBUG
			printf("Environment pos %i removed\n",idx);
#endif
			continue; // No need to process this creature further - it's gone
		}

		// Unless position has changed considerably, do not update position
		SourceIsMoving =	(abs(EnvironmentSounds[idx].positionX - EnvironmentSounds[idx].lastX) > 0.5) ||
		                    (abs(EnvironmentSounds[idx].positionY - EnvironmentSounds[idx].lastY) > 0.5) ||
		                    (abs(EnvironmentSounds[idx].positionZ - EnvironmentSounds[idx].lastZ) > 0.5);

		if( SourceIsMoving ) {
#ifdef MUSIC_DEBUG
			printf("Environment %i pos updated: %3.3f o %3.3f o %3.3f\n",idx, EnvironmentSounds[idx].positionX, EnvironmentSounds[idx].positionY, EnvironmentSounds[idx].positionZ);
#endif
			EnvironmentSounds[idx].lastX = EnvironmentSounds[idx].positionX;
			EnvironmentSounds[idx].lastY = EnvironmentSounds[idx].positionY;
			EnvironmentSounds[idx].lastZ = EnvironmentSounds[idx].positionZ;
		}

		// Clean up errors
		CheckForError("TSExec after environment cleanup");

		// No need to check dead flag - the environment never dies

		// If a source is attached, handle sound update
		if (EnvironmentSounds[idx].attachedEnvironmentSource != -1) {
			// TODO: Check if sound is finished, in which case deallocate source

			ALenum state;
			alGetSourcei(fEnvironmentSources[EnvironmentSounds[idx].attachedEnvironmentSource],AL_SOURCE_STATE, &state);

			// If the sound is already playing?
			if( state==AL_PLAYING ) {
				// If sound is still running, update position of this source
				EnvironmentSetSourcePosition(idx, EnvironmentSounds[idx].positionX, EnvironmentSounds[idx].positionY, EnvironmentSounds[idx].positionZ);
				CheckForError("Set environment position");
			}
			// There is no source attached to this item
		} else {
			// Trigger sound if there is a source available
#ifdef MUSIC_DEBUG
			printf("No source for environment %i\n",idx);
#endif

			for( FreeSource=0; FreeSource<SNumEnvSrc; FreeSource++ ) {
				if( fEnvironmentSourceFree[FreeSource] ) {
					break;
				}
			}

			if (FreeSource<SNumEnvSrc ) {
#ifdef MUSIC_DEBUG
				printf("Found free env src %i\n", FreeSource);
#endif

				switch (EnvironmentSounds[idx].type) {

				case STeleport:
					fEnvironmentSourceFree[FreeSource] = false;
					EnvironmentSounds[idx].attachedEnvironmentSource = FreeSource;
					alSourcei(fEnvironmentSources[FreeSource], AL_BUFFER, fBuffers[SBufTeleport]);
					CheckForError("Set environment sound properties: BUFFER");
					SrcBaseConfig(fEnvironmentSources[FreeSource]);
					CheckForError("Set environment sound properties: BASE");
					EnvironmentSetSourcePosition(idx,EnvironmentSounds[idx].positionX,EnvironmentSounds[idx].positionY,EnvironmentSounds[idx].positionZ);
					CheckForError("Set environment sound properties: POSITION");
					alSourcei(fEnvironmentSources[FreeSource], AL_LOOPING, AL_TRUE); // Teleport = loop
					CheckForError("Set environment sound properties: LOOPING");
					alSourcePlay(fEnvironmentSources[FreeSource]);
					CheckForError("Play environment sound");
					break;

				default:
					printf("Error: Environment unknown sound type!\n");
					break;
				}
			}


		} // End of else FOR "if (EnvironmentSounds[idx].attachedEnvironmentSource != -1)"

	} // Loop over EnvironmentSounds

}


/*
** HANDLE SPECIAL EFFECTS - Footsteps, healing, level up etc.
*/
void SoundControl::HandleSpecialFx() {
	if (fRequestedSound & SPlayerHits) {
		alSourcePlay(fSources[SSrcPlayerHit]);
		CheckForError("PlayerHit");
		// Set music mode to combat once someone hits something
		if( fCurrentMusicMode != SMusicModeCombat ) {
			fMusicFadeOut = true;
			fNextMusicMode = SMusicModeCombat;
		} else {
			MusicChangeTimeOut = MUSIC_TIMEOUT_VAL; // Cause music change to be delayed further
		}
	}

	if (fRequestedSound & SMonsterHits) {
		alSourcePlay(fSources[SSrcMonsterHit]);
		CheckForError("MonsterHit");
		// Set music mode to combat once someone hits something
		if( fCurrentMusicMode != SMusicModeCombat ) {
			fMusicFadeOut = true;
			fNextMusicMode = SMusicModeCombat;
		} else {
			MusicChangeTimeOut = MUSIC_TIMEOUT_VAL; // Cause music change to be delayed further
		}
	}

	if (fRequestedSound & SHealingSelf) {
		alSourcePlay(fSources[SSrcHealSelf]);
		CheckForError("HealSelf");
	}

	if (fRequestedSound & SPlayerJump && !fFeetInAir) {
		alSourcePlay(fSources[SSrcPlayerVoice]);
		CheckForError("Jump");
	}

	if (fRequestedSound & SLevelUp) {
		alSourcePlay(fSources[SSrcLevelUp]);
		CheckForError("LevelUp");
	}

	if (fRequestedSound & SInterfacePing) {
		alSourcePlay(fSources[SSrcInterfacePing]);
		CheckForError("Ping");
	}

	if (fRequestedSound & SDropPotion) {
		this->HandleTrigSound("POT1");
	}

	if (fRequestedSound & SDropWeapon) {
		this->HandleTrigSound("WPN1");
	}

	if (fRequestedSound & SDropArmor) {
		this->HandleTrigSound("ARM1");
	}

	if (fRequestedSound & SDropScroll) {
		this->HandleTrigSound("S001");
	}

	if (fRequestedSound & SBuildBlock) {
		alSourceStop(fSources[SSrcBuildAction]); // Stop Sound before trying to change buffer
		CheckForError("Stop Buildsound");

		alSourcei(fSources[SSrcBuildAction], AL_BUFFER, fBuffers[SBufBlockBuild]);
		alSourcePlay(fSources[SSrcBuildAction]);
		CheckForError("Block Add");
	}

	if (fRequestedSound & SRemoveBlock) {
		alSourceStop(fSources[SSrcBuildAction]); // Stop Sound before trying to change buffer
		CheckForError("Stop Buildsound");

		alSourcei(fSources[SSrcBuildAction], AL_BUFFER, fBuffers[SBufBlockRemove]);
		alSourcePlay(fSources[SSrcBuildAction]);
		CheckForError("Block Remove");
	}

	if (fUnderWater != WasUnderWater) {
		WasUnderWater = fUnderWater;
	}

	if (fFeetInWater != WasInWater) {
		WasInWater = fFeetInWater;
		alSourceStop(fSources[SSrcFootsteps]); // Stop Sound before trying to change buffer
		CheckForError("FeetWet:Stop Footsteps");
		// printf("Water state changed to ");
		if( WasInWater ) {
			alSourcei(fSources[SSrcFootsteps], AL_BUFFER, fBuffers[SBufFootstepsInWater]);
			CheckForError("FeetWet:Set source in water");
			//printf("feet in water\n");
		} else {
			alSourcei(fSources[SSrcFootsteps], AL_BUFFER, fBuffers[SBufFootsteps]);
			CheckForError("FeetWet:Set source on land");
			//printf("feet not in water\n");
		}
		// This will reset the running status and play the sound if required
		WasRunning = !WasRunning;
	}

	if (fFeetInAir != WasInAir ) {
#ifdef MUSIC_DEBUG
		if (fFeetInAir)
			printf("Feet are in the air!\n");
		else
			printf("Feet are not in the air!\n");
#endif
		WasInAir = fFeetInAir;
		// This will reset the running status and play the sound if required
		WasRunning = !WasRunning;
	}

	if (fPlayerRunning != WasRunning ) {
		if( fPlayerRunning && !WasUnderWater && !WasInAir ) {
			alSourcePlay(fSources[SSrcFootsteps]);
			CheckForError("UnderWater:On land");
		} else {
			alSourceStop(fSources[SSrcFootsteps]);
			CheckForError("In water or in the air");
		}
		WasRunning = fPlayerRunning;
	}

	// Another entity has requested a change of music mode
	if (fRequestedMusicMode != SMusicModeNone ) {
		if( fRequestedMusicMode != fCurrentMusicMode ) {
			fNextMusicMode = fRequestedMusicMode;		// Request new music mode
			fMusicFadeOut = true;						// Smoother change of music mode
		}

		fRequestedMusicMode = SMusicModeNone; 			// Clear request
	}
}

/*
** HANDLE PLAYER ORIENTATION
*/
void SoundControl::HandlePlayerOrientation(void) {
	static float LastCompassAngle = 0.0f;
	// Handle player orientation, found in gPlayer.fAngleHor
	if( ((Model::gPlayer.fAngleHor-LastCompassAngle) >= 1.0f) || ((Model::gPlayer.fAngleHor-LastCompassAngle) <= -1.0f) ) {
		LastCompassAngle = Model::gPlayer.fAngleHor;
		ALfloat atX = -((ALfloat) sin(Model::gPlayer.fAngleHor*3.14159265f/180.0f)); // Normalized vector -->NOTE: Changed to - to fit world coordinates
		ALfloat atZ = (-(ALfloat) cos(Model::gPlayer.fAngleHor*3.14159265f/180.0f)); // Normalized vector
		ALfloat Ori[] = {atX,0.0f,atZ,0.0f,1.0f,0.0f}; // Ignore if head is turned up/down
		// Ori vector :  <------AT-----> <-----UP----->
		alListenerfv(AL_ORIENTATION, Ori);
		CheckForError("Orientation Test");

#ifdef MUSIC_DEBUG
		printf("Player orientation: %3.3f -> x=%3.3f z=%3.3f\n", Model::gPlayer.fAngleHor, (float)atX, (float)atZ );
#endif
	}

	// Clean up errors
	CheckForError("After player orientation");
}

// TODO: Play requested trig sound - this has "priority" for the time being but
// it will probably be enough to do all of this in the timer task
void SoundControl::HandleTrigSound(const char *snd) {
	ALenum state;
// 1. Check if source is playing and stop source SSrcTrigEvent
	alGetSourcei(fSources[SSrcTrigEvent],AL_SOURCE_STATE, &state);
	if( state==AL_PLAYING ) {
		alSourceStop(fSources[SSrcTrigEvent]);
		CheckForError("Trig stop sound");
	}

	alSourcei(fSources[SSrcTrigEvent], AL_BUFFER, AL_NONE);
	CheckForError("Trig Buffer disconnect");
	alDeleteBuffers(1,&fBuffers[SBufTrigEvent]);
	CheckForError("Trig Buffer delete");

	// 2. Load buffer
	unsigned int idx;

	for( idx=0 ; idx < NELEM(fTrigSoundList) ; idx++ ) {
		if( strncmp( fTrigSoundList[idx].id, snd, 4 ) == 0 ) {
			break;
		}
	}

	// 3. Attach buffer to source
	fBuffers[SBufTrigEvent] = alutCreateBufferFromFile(fTrigSoundList[idx].fileName);
	CheckForError("Trig Buffering fill");
	alSourcei(fSources[SSrcTrigEvent], AL_BUFFER, fBuffers[SBufTrigEvent]);
	CheckForError("Trig Buffering");

	// 4. Configure source (TODO: Remove)
	SrcBaseConfig(fSources[SSrcTrigEvent]);
	CheckForError("Trig Init sound buffers");
	alSourcei(fSources[SSrcTrigEvent], AL_LOOPING, AL_FALSE);
	CheckForError("Trig Set looping for sound buffer");

	// 5. Play!
	alSourcePlay(fSources[SSrcTrigEvent]);

	// Reset reqested trig sound
}

/*
** MAIN THREAD COORDINATION
*/
void *SoundControl::Thread(void *p) {
	bool MusicWasOn;
	int idx;
	char RequestedTrigSound[4] = {0,0,0,0};

	SoundControl *ss = (SoundControl *)p;
	MusicWasOn=ss->fMusicOn;
	pthread_mutex_lock(&ss->fMutex);
	// Stay in a loop forever, waiting for new messages to play
	while(1) {
		pthread_cond_wait(&ss->fCondLock, &ss->fMutex);

		// Clean up errors
		CheckForError("Before thread start");

		// Copy trig sound ID to minimize risk of accidental change during search
		for(idx=0; idx<4 && ss->fRequestedTrigSound[idx]!=0; idx++) {
			RequestedTrigSound[idx] = ss->fRequestedTrigSound[idx];
			ss->fRequestedTrigSound[idx] = 0;
		}

		if( RequestedTrigSound[0] != 0 ) {
			ss->HandleTrigSound(RequestedTrigSound);
			RequestedTrigSound[0] = 0;
			//printf("---END OF TRIG REQUEST---\n");
		}

		if (ss->fRequestedSound & SoundControl::STerminate || gMode.Get() == GameMode::EXIT) {
			// This is not a sound, it is used to request that the child thread terminates.
			pthread_mutex_unlock(&ss->fMutex);
			pthread_exit(0);
		}

		ss->HandleSpecialFx();

		if (ss->fMusicOn != MusicWasOn) {
			// Execute only if music mode has changed
			MusicWasOn = ss->fMusicOn;
			gOptions.fMusicOn = ss->fMusicOn;

			if( ss->fMusicOn ) {
				// printf("Music switch mode, now ON\n");
			} else {
				// printf("Music switch mode, now OFF\n");
				ss->fMusicFadeOut = false;
				ss->fMusicIsPlaying = false;
				ss->fMusicVolume = ss->fMusicVolumePref;
				ss->MusicStop();
			}
		}

		// Clean up errors
		CheckForError("Before TSExec");

		// This indicates that the periodic sound update has been called.
		// Most basic tasks should be performed here, as they do not need more updates than this.
		if (ss->fRequestedSound & STSExec) {
			ss->HandlePlayerOrientation();

			ss->HandleCreatures();

			ss->HandleEnvironment();

			// Since file buffering occurs here, it should be done after playing
			// all other sounds, and only when the timer task trigger SoundControl.
			if (ss->fMusicOn)
				ss->HandleMusic();
		}

		// Clear the sound request before exiting
		ss->fRequestedSound = SNone;
	} // While (1)
	return 0; // Not reached, but get rid of warnings
}

