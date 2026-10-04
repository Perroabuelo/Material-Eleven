/*
 * The configurable output grain, the system volume tracking and the volume
 * limiter for EQ presets are derived from ElevenMPV-A
 * (https://github.com/GrapheneCt/ElevenMPV-A), (C) 2020-2022 GrapheneCt and
 * contributors, GPL-3.0-or-later, and were adapted from C++ to C. See NOTICE.
 */

#include <psp2/apputil.h>
#include <psp2/kernel/error.h>
#include <psp2/kernel/threadmgr.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "vitaaudiolib.h"

// Undocumented SceAppUtil system param id for the current system volume level (0-30).
#define VITA_SYSTEM_PARAM_ID_VOLUME 9

// How long vitaAudioEnd waits for an output thread to leave its loop. In the
// normal case it leaves after the grain it is on, about 20 ms.
#define VITA_AUDIO_END_TIMEOUT_US (1000 * 1000)

// Output threads that did not end in time, kept with their port and buffers
// until they do. Freeing those under a thread that is still running is exactly
// what the wait is there to avoid.
#define VITA_AUDIO_PENDING_MAX 4

static int audio_ready = 0;
static short *vitaAudioSoundBuffer[VITA_NUM_AUDIO_CHANNELS][2];
static VITA_audio_channelinfo vitaAudioStatus[VITA_NUM_AUDIO_CHANNELS];
static volatile int audio_terminate = 0;
static unsigned int audio_grain = VITA_DEFAULT_AUDIO_SAMPLES;
static unsigned int audio_channel_count = 2;

// Bumped by every vitaAudioInit. A thread whose generation is no longer the
// current one leaves its loop even though vitaAudioInit has cleared
// audio_terminate for the next track.
static volatile unsigned int audio_generation = 0;

// Copied onto the thread's own stack by sceKernelStartThread, so a thread from
// an earlier track only ever touches its own port and buffers.
typedef struct {
	int channel;
	unsigned int generation;
	int port;
	short *buffer[2];
	unsigned int grain;
	unsigned int channel_count;
} VITA_audio_threadargs;

typedef struct {
	int threadhandle;
	int port;
	short *buffer[2];
} VITA_audio_pending;

static VITA_audio_pending audio_pending[VITA_AUDIO_PENDING_MAX];
static int audio_pending_count = 0;
static unsigned int audio_deferred_reaps = 0;

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

static SceBool vitaAudioThreadIsCurrent(unsigned int generation) {
	return (audio_terminate == 0 && generation == audio_generation) ? SCE_TRUE : SCE_FALSE;
}

static int vitaAudioChannelThread(unsigned int args, void *argp) {
	volatile int bufidx = 0;

	VITA_audio_threadargs a = *(VITA_audio_threadargs *) argp;

	while (vitaAudioThreadIsCurrent(a.generation)) {
		void *bufptr = a.buffer[bufidx];
		vitaAudioCallback_t callback;
		callback = vitaAudioStatus[a.channel].callback;

		// The callback slot is shared with the next track's thread, so it is
		// only used while this thread is still the current one.
		if (callback && vitaAudioThreadIsCurrent(a.generation))
			callback(bufptr, a.grain, vitaAudioStatus[a.channel].userdata);
		else {
			unsigned int *ptr = bufptr;
			unsigned int i, count = (a.grain * a.channel_count * sizeof(short)) / sizeof(unsigned int);
			for (i = 0; i < count; ++i)
				*(ptr++) = 0;
		}

		if (!vitaAudioThreadIsCurrent(a.generation))
			break;

		// Follow the system volume slider, applying a compensating cut when an
		// EQ preset is boosting gain and the user opted in to limiting for it.
		int vol = SCE_AUDIO_OUT_MAX_VOL;
		sceAppUtilSystemParamGetInt(VITA_SYSTEM_PARAM_ID_VOLUME, &vol);

		if (config.eq_mode != 0 && config.eq_volume)
			vol /= 2;

		if (vol > SCE_AUDIO_OUT_MAX_VOL)
			vol = SCE_AUDIO_OUT_MAX_VOL;

		int vols[2] = {vol, vol};
		sceAudioOutSetVolume(a.port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vols);
		sceAudioOutOutput(a.port, bufptr);
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

unsigned int vitaAudioGetDeferredReaps(void) {
	return audio_deferred_reaps;
}

static void vitaAudioRelease(int port, short *buffer0, short *buffer1) {
	if (port >= 0)
		sceAudioOutReleasePort(port);

	free(buffer0);
	free(buffer1);
}

// Waits up to *timeout microseconds for the thread to end, or forever when
// timeout is NULL. Returns SCE_FALSE only if it is still running.
static SceBool vitaAudioWaitThread(int threadhandle, SceUInt *timeout) {
	int ret = sceKernelWaitThreadEnd(threadhandle, NULL, timeout);

	if (ret == (int)SCE_KERNEL_ERROR_WAIT_TIMEOUT)
		return SCE_FALSE;

	// Ended, or the handle is no good any more: either way nothing can be
	// waited on, and the thread is deleted below.
	sceKernelDeleteThread(threadhandle);
	return SCE_TRUE;
}

// Frees every pending thread that has ended by now, without waiting.
static void vitaAudioReapPending(void) {
	int i = 0;

	while (i < audio_pending_count) {
		VITA_audio_pending *p = &audio_pending[i];
		SceUInt timeout = 0;

		if (vitaAudioWaitThread(p->threadhandle, &timeout)) {
			vitaAudioRelease(p->port, p->buffer[0], p->buffer[1]);
			// Shifted down rather than swapped, so the oldest stays first.
			memmove(&audio_pending[i], &audio_pending[i + 1], (audio_pending_count - i - 1) * sizeof(audio_pending[0]));
			audio_pending_count--;
		}
		else
			i++;
	}
}

static void vitaAudioDefer(int threadhandle, int port, short *buffer0, short *buffer1) {
	if (audio_pending_count == VITA_AUDIO_PENDING_MAX) {
		// Full: wait for the oldest, however long it takes, to make room.
		VITA_audio_pending *oldest = &audio_pending[0];

		vitaAudioWaitThread(oldest->threadhandle, NULL);
		vitaAudioRelease(oldest->port, oldest->buffer[0], oldest->buffer[1]);
		memmove(&audio_pending[0], &audio_pending[1], (VITA_AUDIO_PENDING_MAX - 1) * sizeof(audio_pending[0]));
		audio_pending_count--;
	}

	audio_pending[audio_pending_count].threadhandle = threadhandle;
	audio_pending[audio_pending_count].port = port;
	audio_pending[audio_pending_count].buffer[0] = buffer0;
	audio_pending[audio_pending_count].buffer[1] = buffer1;
	audio_pending_count++;
	audio_deferred_reaps++;
}

int vitaAudioInit(int frequency, SceAudioOutMode mode) {
	int i, ret;
	int failed = 0;
	char str[32];

	vitaAudioReapPending();

	audio_generation++;
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

		VITA_audio_threadargs args = {
			.channel = i,
			.generation = audio_generation,
			.port = vitaAudioStatus[i].handle,
			.buffer = { vitaAudioSoundBuffer[i][0], vitaAudioSoundBuffer[i][1] },
			.grain = audio_grain,
			.channel_count = audio_channel_count,
		};

		ret = sceKernelStartThread(vitaAudioStatus[i].threadhandle, sizeof(args), &args);

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

	vitaAudioReapPending();

	for (i = 0; i < VITA_NUM_AUDIO_CHANNELS; i++) {
		int port = vitaAudioStatus[i].handle;
		short *buffer0 = vitaAudioSoundBuffer[i][0], *buffer1 = vitaAudioSoundBuffer[i][1];
		SceBool ended = SCE_TRUE;

		if (vitaAudioStatus[i].threadhandle != -1) {
			SceUInt timeout = VITA_AUDIO_END_TIMEOUT_US;

			ended = vitaAudioWaitThread(vitaAudioStatus[i].threadhandle, &timeout);
			if (!ended)
				vitaAudioDefer(vitaAudioStatus[i].threadhandle, port, buffer0, buffer1);
		}

		if (ended)
			vitaAudioRelease(port, buffer0, buffer1);

		vitaAudioStatus[i].threadhandle = -1;
		vitaAudioStatus[i].handle = -1;
		vitaAudioSoundBuffer[i][0] = NULL;
		vitaAudioSoundBuffer[i][1] = NULL;
	}
}
