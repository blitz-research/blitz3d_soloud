#include "bbaudio_soloud.h"

#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_wavstream.h"

SoLoud::Soloud *soloud;

namespace {
	std::set<Sound *> sounds;

	inline void debugSound(Sound *sound) {
		if (::debug && sounds.find(sound) == sounds.end()) RTEX("Sound does not exist"); // NOLINT
	}

	inline void debugChannel(uint32_t channel) {
		if (::debug && !soloud) RTEX("Soloud has not been initialized"); // NOLINT
	}

	float rolloffFactor = 1.0f;
	float dopplerFactor = 1.0f;
	float distanceFactor = 1.0f;
}

struct Sound {
	SoLoud::Wav *const wav;

	float volume{1};
	float pan{0};
	int pitch{0};

	explicit Sound(SoLoud::Wav *wav) : wav(wav) {
		if (debug) sounds.insert(this);
	}

	~Sound() {
		if (debug) sounds.erase(this);
		delete wav;
	}

	uint32_t play() const {
		uint32_t chan;
		if (pitch) {
			chan = soloud->play(*wav, volume, pan, true, 0);
			soloud->setSamplerate(chan, (float) pitch);
			soloud->setPause(chan, false);
		} else {
			chan = soloud->play(*wav, volume, pan, false, 0);
		}
		return chan;
	}

	uint32_t play3d(float x, float y, float z, float vx, float vy, float vz) const {
		uint32_t chan;
		wav->set3dAttenuation(SoLoud::AudioSource::INVERSE_DISTANCE, rolloffFactor);
		wav->set3dDopplerFactor(dopplerFactor);
		if (pitch) {
			chan = soloud->play3d(*wav, x, y, z, vx, vy, vz, volume, true, 0);
			soloud->setSamplerate(chan, (float) pitch);
			soloud->setPause(chan, false);
		} else {
			chan = soloud->play3d(*wav, x, y, z, vx, vy, vz, volume, false, 0);
		}
		return chan;
	}
};

Sound *bbLoadSound(BBStr *path) {
	if (!soloud) {
		delete path;
		return nullptr;
	}
	auto wav = new SoLoud::Wav();
	auto r = wav->load(path->c_str());
	delete path;
	return r == SoLoud::SO_NO_ERROR ? new Sound(wav) : nullptr;
}

Sound *bbLoad3DSound(BBStr *path) {
	auto sound = bbLoadSound(path);
	return sound;
}

void bbFreeSound(Sound *sound) {
	if (!sound) return;
	debugSound(sound);
	delete sound;
}

void bbLoopSound(Sound *sound) {
	if (!sound) return;
	debugSound(sound);
	sound->wav->setLooping(true);
}

void bbSoundVolume(Sound *sound, float volume) {
	if (!sound) return;
	debugSound(sound);
	sound->volume = volume;
}

void bbSoundPan(Sound *sound, float pan) {
	if (!sound) return;
	debugSound(sound);
	sound->pan = pan;
}

void bbSoundPitch(Sound *sound, int pitch) {
	if (!sound) return;
	debugSound(sound);
	sound->pitch = pitch;
}

uint32_t bbPlaySound(Sound *sound) {
	if (!sound) return 0;
	debugSound(sound);
	return sound->play();
}

uint32_t bbPlayMusic(BBStr *path) {
	if(!soloud) {
		delete path;
		return 0;
	}
	SoLoud::WavStream wavStream;
	auto r = wavStream.load(path->c_str());
	delete path;
	return r == SoLoud::SO_NO_ERROR ? soloud->play(wavStream) : 0;
}

void bbStopChannel(uint32_t channel) {
	if (!channel) return;
	debugChannel(channel);
	soloud->stop(channel);
}

void bbPauseChannel(uint32_t channel) {
	if (!channel) return;
	debugChannel(channel);
	soloud->setPause(channel, true);
}

void bbResumeChannel(uint32_t channel) {
	if (!channel) return;
	debugChannel(channel);
	soloud->setPause(channel, false);
}

void bbChannelVolume(uint32_t channel, float volume) {
	if (!channel) return;
	debugChannel(channel);
	soloud->setVolume(channel, volume);
}

void bbChannelPan(uint32_t channel, float pan) {
	if (!channel) return;
	debugChannel(channel);
	soloud->setPan(channel, pan);
}

void bbChannelPitch(uint32_t channel, int pitch) {
	if (!channel) return;
	debugChannel(channel);
	soloud->setSamplerate(channel, (float) pitch);
}

int bbChannelPlaying(uint32_t channel) {
	if (!channel) return 0;
	debugChannel(channel);
	return soloud->isValidVoiceHandle(channel);
}

uint32_t bbPlay3dSound(Sound *sound, float x, float y, float z, float vx, float vy, float vz) {
	if (!sound) return 0;
	debugSound(sound);
	auto sc = distanceFactor;
	return sound->play3d(x * sc, y * sc, z * sc, vx * sc, vy * sc, vz * sc);
}

void bbSet3dChannel(uint32_t channel, float x, float y, float z, float vx, float vy, float vz) {
	if (!channel) return;
	debugChannel(channel);
	auto sc = distanceFactor;
	soloud->set3dSourceParameters(channel, x * sc, y * sc, z * sc, vx * sc, vy * sc, vz * sc);
}

void bbSet3dListenerConfig(float roll, float dopp, float dist) {
	if (!soloud) return;
	rolloffFactor = roll;
	dopplerFactor = dopp;
	distanceFactor = dist;
}

void bbSet3dListener(float x, float y, float z, float kx, float ky, float kz, float jx, float jy, float jz, float vx,
					 float vy, float vz) {
	if (!soloud) return;
	auto sc = distanceFactor;
	soloud->set3dListenerParameters(x * sc, y * sc, z * sc, kx, ky, kz, jy, jy, jz, vx * sc, vy * sc, vz * sc);
	soloud->update3dAudio();
}

bool audio_create() {
	soloud = new SoLoud::Soloud();
	if (soloud->init() != SoLoud::SO_NO_ERROR) {
		delete soloud;
		soloud = nullptr;
	}
	return true;
}

bool audio_destroy() {
	if (!soloud) return true;
	soloud->deinit();
	delete soloud;
	soloud = nullptr;
	return true;
}

void audio_link(void(*rtSym)(const char *, void *)) {
	rtSym("%LoadSound$filename", bbLoadSound);
	rtSym("%Load3DSound$filename", bbLoad3DSound);
	rtSym("FreeSound%sound", bbFreeSound);
	rtSym("LoopSound%sound", bbLoopSound);
	rtSym("SoundPitch%sound%pitch", bbSoundPitch);
	rtSym("SoundVolume%sound#volume", bbSoundVolume);
	rtSym("SoundPan%sound#pan", bbSoundPan);
	rtSym("%PlaySound%sound", bbPlaySound);
	rtSym("%PlayMusic$midifile", bbPlayMusic);
//	rtSym( "%PlayCDTrack%track%mode=1",bbPlayCDTrack );
	rtSym("StopChannel%channel", bbStopChannel);
	rtSym("PauseChannel%channel", bbPauseChannel);
	rtSym("ResumeChannel%channel", bbResumeChannel);
	rtSym("ChannelPitch%channel%pitch", bbChannelPitch);
	rtSym("ChannelVolume%channel#volume", bbChannelVolume);
	rtSym("ChannelPan%channel#pan", bbChannelPan);
	rtSym("%ChannelPlaying%channel", bbChannelPlaying);
}
