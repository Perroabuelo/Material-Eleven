/*
 * The configurable output grain, the system volume tracking and the volume
 * limiter for EQ presets are derived from ElevenMPV-A
 * (https://github.com/GrapheneCt/ElevenMPV-A), (C) 2020-2022 GrapheneCt and
 * contributors, GPL-3.0-or-later, and were adapted from C++ to C. See NOTICE.
 */

#include <psp2/apputil.h>
#include <psp2/kernel/threadmgr.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "vitaaudiolib.h"

// Undocumented SceAppUtil system param id for the current system volume level (0-30).
#define VITA_SYSTEM_PARAM_ID_VOLUME 9

static int audio_ready = 0;
static short *vitaAudioSoundBuffer[VITA_NUM_AUDIO_CHANNELS][2];
static VITA_audio_channelinfo vitaAudioStatus[VITA_NUM_AUDIO_CHANNELS];
static volatile int audio_terminate = 0;
static unsigned int audio_grain = VITA_DEFAULT_AUDIO_SAMPLES;
static unsigned int audio_channel_count = 2;

void vitaAudioSetVolume(int channel, int left, int right) {
	vitaAudioStatus[channel].volumeleft = left;
	vitaAudioStatus[channel].volumeright = right;
}

void vitaAudioSetChannelCallback(int channel, vitaAudioCallback_t callback, void *userdata) {
	volatile VITA_audio_channelinfo *pci = &vitaAudioStatus[channel];

	if (callback == 0)
		pci->callback = 0;
	else
		pci->callback = callback;
}

int vitaAudioOutBlocking(unsigned int channel, unsigned int vol1, unsigned int vol2, const void *buf) {
	if (!audio_ready)
		return(-1);

	if (channel >= VITA_NUM_AUDIO_CHANNELS)
		return(-1);

	if (vol1 > SCE_AUDIO_OUT_MAX_VOL)
		vol1 = SCE_AUDIO_OUT_MAX_VOL;

	if (vol2 > SCE_AUDIO_OUT_MAX_VOL)
		vol2 = SCE_AUDIO_OUT_MAX_VOL;

	int vol[2] = {vol1, vol2};
	sceAudioOutSetVolume(vitaAudioStatus[channel].handle, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vol);
	return sceAudioOutOutput(vitaAudioStatus[channel].handle, buf);
}

static int vitaAudioChannelThread(unsigned int args, void *argp) {
	volatile int bufidx = 0;

	int channel = *(int *) argp;

	while (audio_terminate == 0) {
		void *bufptr = vitaAudioSoundBuffer[channel][bufidx];
		vitaAudioCallback_t callback;
		callback = vitaAudioStatus[channel].callback;

		if (callback)
			callback(bufptr, audio_grain, vitaAudioStatus[channel].userdata);
		else {
			unsigned int *ptr = bufptr;
			unsigned int i, count = (audio_grain * audio_channel_count * sizeof(short)) / sizeof(unsigned int);
			for (i = 0; i < count; ++i)
				*(ptr++) = 0;
		}

		// Follow the system volume slider, applying a compensating cut when an
		// EQ preset is boosting gain and the user opted in to limiting for it.
		int vol = SCE_AUDIO_OUT_MAX_VOL;
		sceAppUtilSystemParamGetInt(VITA_SYSTEM_PARAM_ID_VOLUME, &vol);

		if (config.eq_mode != 0 && config.eq_volume)
			vol /= 2;

		vitaAudioOutBlocking(channel, vol, vol, bufptr);
		bufidx = (bufidx ? 0:1);
	}

	sceKernelExitThread(0);
	return(0);
}

void vitaAudioPreSetGrain(unsigned int grain) {
	audio_grain = grain;
}

unsigned int vitaAudioGetGrain(void) {
	return audio_grain;
}

unsigned int vitaAudioGetChannelCount(void) {
	return audio_channel_count;
}

unsigned int vitaAudioGetDefaultGrain(void) {
	return VITA_DEFAULT_AUDIO_SAMPLES;
}

int vitaAudioInit(int frequency, SceAudioOutMode mode) {
	int i, ret;
	int failed = 0;
	char str[32];

	audio_terminate = 0;
	audio_ready = 0;
	audio_channel_count = (mode == SCE_AUDIO_OUT_MODE_STEREO) ? 2 : 1;

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		vitaAudioStatus[i].handle = -1;
		vitaAudioStatus[i].threadhandle = -1;
		vitaAudioStatus[i].volumeright = SCE_AUDIO_OUT_MAX_VOL;
		vitaAudioStatus[i].volumeleft  = SCE_AUDIO_OUT_MAX_VOL;
		vitaAudioStatus[i].callback = 0;
		vitaAudioStatus[i].userdata = 0;

		vitaAudioSoundBuffer[i][0] = calloc(audio_grain * audio_channel_count, sizeof(short));
		vitaAudioSoundBuffer[i][1] = calloc(audio_grain * audio_channel_count, sizeof(short));

		if (vitaAudioSoundBuffer[i][0] == NULL || vitaAudioSoundBuffer[i][1] == NULL)
			failed = 1;
	}

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		if ((vitaAudioStatus[i].handle = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, audio_grain, frequency, mode)) < 0)
			failed = 1;
	}

	if (failed) {
		for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
			if (vitaAudioStatus[i].handle != -1)
				sceAudioOutReleasePort(vitaAudioStatus[i].handle);

			vitaAudioStatus[i].handle = -1;

			free(vitaAudioSoundBuffer[i][0]);
			free(vitaAudioSoundBuffer[i][1]);
			vitaAudioSoundBuffer[i][0] = NULL;
			vitaAudioSoundBuffer[i][1] = NULL;
		}

		return 0;
	}

	audio_ready = 1;
	strcpy(str, "audiot0");

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		str[6]= '0' + i;
		vitaAudioStatus[i].threadhandle = sceKernelCreateThread(str, (SceKernelThreadEntry)&vitaAudioChannelThread, 0x40, 0x10000, 0, 0, NULL);

		if (vitaAudioStatus[i].threadhandle < 0) {
			vitaAudioStatus[i].threadhandle = -1;
			failed = 1;
			break;
		}

		ret = sceKernelStartThread(vitaAudioStatus[i].threadhandle, sizeof(i), &i);

		if (ret != 0) {
			failed = 1;
			break;
		}
	}

	if (failed) {
		audio_terminate = 1;

		for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
			if (vitaAudioStatus[i].threadhandle != -1)
				sceKernelDeleteThread(vitaAudioStatus[i].threadhandle);

			vitaAudioStatus[i].threadhandle = -1;
		}

		audio_ready = 0;
		return 0;
	}

	return 1;
}

void vitaAudioEndPre(void) {
	audio_ready = 0;
	audio_terminate = 1;
}

void vitaAudioEnd(void) {
	int i = 0;
	audio_ready = 0;
	audio_terminate = 1;

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		if (vitaAudioStatus[i].threadhandle != -1)
			sceKernelDeleteThread(vitaAudioStatus[i].threadhandle);

		vitaAudioStatus[i].threadhandle = -1;
	}

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		if (vitaAudioStatus[i].handle != -1) {
			sceAudioOutReleasePort(vitaAudioStatus[i].handle);
			vitaAudioStatus[i].handle = -1;
		}

		free(vitaAudioSoundBuffer[i][0]);
		free(vitaAudioSoundBuffer[i][1]);
		vitaAudioSoundBuffer[i][0] = NULL;
		vitaAudioSoundBuffer[i][1] = NULL;
	}
}
