#pragma once

#include "bbsys.h"

struct Sound;

Sound *bbLoadSound(BBStr *path);
Sound *bbLoad3DSound(BBStr *path);
void bbFreeSound(Sound *sound);
void bbLoopSound(Sound *sound);
void bbSoundVolume(Sound *sound, float volume);
void bbSoundPan(Sound *sound, float pan);
void bbSoundPitch(Sound *sound, int pitch);
uint32_t bbPlaySound(Sound *sound);
uint32_t bbPlayMusic(BBStr* path);
void bbStopChannel(uint32_t channel);
void bbPauseChannel(uint32_t channel);
void bbResumeChannel(uint32_t channel);
void bbChannelVolume(uint32_t channel, float volume);
void bbChannelPan(uint32_t channel, float pan);
void bbChannelPitch(uint32_t channel, int pitch);
int bbChannelPlaying(uint32_t channel);

// ***** INTERNAL *****
uint32_t bbPlay3dSound(Sound* sound,float x, float y, float z, float vx, float vy, float vz);
void bbSet3dChannel(uint32_t channel, float x, float y, float z, float vx, float vy, float vz);
void bbSet3dListenerConfig(float roll, float dopp, float dist);
void bbSet3dListener(float x,float y, float z, float kx, float ky, float kz, float jx, float jy, float jz, float vx, float vy, float vz);
