#include <psp2/kernel/threadmgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "debug_log.h"
#include "fade.h"
#include "lyrics.h"
#include "ui_theme.h"
#include "vitaaudiolib.h"
#include "viz_tap.h"
#include "fs.h"

#include "flac.h"
#include "mp3.h"
#include "ogg.h"
#include "opus.h"
#include "wav.h"
#include "xm.h"

enum Audio_FileType {
	FILE_TYPE_NONE,
	FILE_TYPE_FLAC,
	FILE_TYPE_MP3,
	FILE_TYPE_OGG,
	FILE_TYPE_OPUS,
	FILE_TYPE_WAV,
	FILE_TYPE_XM
};

typedef struct {
	int (* init)(const char *path);
	SceUInt32 (* rate)(void);
	SceUInt8 (* channels)(void);
	void (* decode)(void *buf, unsigned int length, void *userdata);
	SceUInt64 (* position)(void);
	SceUInt64 (* length)(void);
	SceUInt64 (* seek)(double fraction);
	void (* term)(void);
} Audio_Decoder;

static enum Audio_FileType file_type = FILE_TYPE_NONE;
Audio_Metadata metadata = {0};
static Audio_Metadata empty_metadata = {0};
static Audio_Decoder decoder = {0}, empty_decoder = {0};
static SceBool track_loaded = SCE_FALSE;
SceBool playing = SCE_TRUE, paused = SCE_FALSE;

// The rate the visualizer's copy is tagged with, set before the track's audio
// thread starts so that thread never asks the decoder.
static unsigned int viz_rate = 0;

// The user's pause is an intention the audio thread carries out: the UI flips
// pause_request and the thread fades out and only then sets `paused`, which
// from then on only the audio thread writes (and Audio_Init / Audio_Term,
// while no thread runs). Each shared value has one writer and is 32 bits.
static volatile int pause_request = 0;    // written by the UI
static volatile int hold = 0;             // written by the UI, see Audio_Hold
static volatile float master_gain = 1.0f; // written by the UI, see Audio_SetMasterGain
static SceBool start_paused = SCE_FALSE;  // UI only, consumed by Audio_Init

// Audio thread only. The master is per process, not per track, so a sleep
// timer fade carries on into the next track.
static Fade_Ramp ramp = { 1.0f, 1.0f, 0.0f, 0 };
static float master_applied = 1.0f;

static void Audio_Decode(void *buf, unsigned int length, void *userdata) {
	unsigned int channels = vitaAudioGetChannelCount();
	int want_paused = pause_request;
	float master = master_gain;

	// While held the ramp stays where it was, and picks up afterwards.
	if (!hold) {
		float target = want_paused ? 0.0f : 1.0f;

		// A new request, or one reversed halfway: from the gain reached.
		if (ramp.target != target)
			Fade_Start(&ramp, target, viz_rate / 10);

		if (!want_paused && paused == SCE_TRUE)
			paused = SCE_FALSE;
	}

	if (playing == SCE_FALSE || paused == SCE_TRUE || hold) {
		// vitaAudioInit reserva grain * canales shorts, asi que el silencio se
		// escribe con esa misma cuenta. Esto era un "length * 2" fijo: en un
		// track mono escribia el doble del buffer y pisaba el heap en cada
		// callback, unas cincuenta veces por segundo. Se entra aqui al pausar,
		// y tambien al acabarse el archivo, que es cuando el decoder pone
		// playing en SCE_FALSE, de modo que bastaba con dejar sonando un mono
		// hasta el final para corromper el heap.
		memset(buf, 0, length * channels * sizeof(short));

		// Nothing is heard, so the master can jump.
		master_applied = master;
	}
	else {
		(* decoder.decode)(buf, length, userdata);
		Fade_Apply(&ramp, buf, length, channels, master_applied, master);
		master_applied = master;

		// Faded all the way out: silence from the next callback on.
		if (want_paused && Fade_Done(&ramp))
			paused = SCE_TRUE;
	}

	// What the console is about to play, silence included, so the
	// visualizer falls to zero on pause. Same frame width as above: a mono
	// buffer holds `length` shorts, not twice that.
	VizTap_Write(VizTap_Shared(), buf, length, channels, viz_rate);
}

static const char *Audio_FormatName(enum Audio_FileType type) {
	switch (type) {
		case FILE_TYPE_FLAC:
			return "flac";
		case FILE_TYPE_MP3:
			return "mp3";
		case FILE_TYPE_OGG:
			return "ogg";
		case FILE_TYPE_OPUS:
			return "opus";
		case FILE_TYPE_WAV:
			return "wav";
		case FILE_TYPE_XM:
			return "xm";
		default:
			return "none";
	}
}

// The track line of the debug log for an attempt to open `path`. The format,
// rate and channels only when it opened, since a failed decoder has none.
static void Audio_LogOpen(const char *path, SceBool ok) {
	if (!DebugLog_On())
		return;

	LogLine line;
	DebugLog_Begin(&line, "track");
	LogLine_Str(&line, "event", "open");
	LogLine_Str(&line, "file", path);
	LogLine_Str(&line, "result", ok ? "ok" : "fail");
	if (ok) {
		LogLine_Str(&line, "format", Audio_FormatName(file_type));
		LogLine_Int(&line, "rate", (* decoder.rate)());
		LogLine_Int(&line, "channels", (* decoder.channels)());
	}
	DebugLog_Commit(&line);
}

int Audio_Init(const char *path) {
	playing = SCE_TRUE;
	paused = SCE_FALSE;
	pause_request = 0;
	Fade_Reset(&ramp, 1.0f);
	
	if (!strncasecmp(FS_GetFileExt(path), "flac", 4))
		file_type = FILE_TYPE_FLAC;
	else if (!strncasecmp(FS_GetFileExt(path), "mp3", 3))
		file_type = FILE_TYPE_MP3;
	else if (!strncasecmp(FS_GetFileExt(path), "ogg", 3))
		file_type = FILE_TYPE_OGG;
	else if (!strncasecmp(FS_GetFileExt(path), "opus", 4))
		file_type = FILE_TYPE_OPUS;
	else if (!strncasecmp(FS_GetFileExt(path), "wav", 3))
		file_type = FILE_TYPE_WAV;
	else if ((!strncasecmp(FS_GetFileExt(path), "it", 2)) || (!strncasecmp(FS_GetFileExt(path), "mod", 3))
		|| (!strncasecmp(FS_GetFileExt(path), "s3m", 3)) || (!strncasecmp(FS_GetFileExt(path), "xm", 2)))
		file_type = FILE_TYPE_XM;

	switch(file_type) {
		case FILE_TYPE_FLAC:
			decoder.init = FLAC_Init;
			decoder.rate = FLAC_GetSampleRate;
			decoder.channels = FLAC_GetChannels;
			decoder.decode = FLAC_Decode;
			decoder.position = FLAC_GetPosition;
			decoder.length = FLAC_GetLength;
			decoder.seek = FLAC_Seek;
			decoder.term = FLAC_Term;
			break;

		case FILE_TYPE_MP3:
			decoder.init = MP3_Init;
			decoder.rate = MP3_GetSampleRate;
			decoder.channels = MP3_GetChannels;
			decoder.decode = MP3_Decode;
			decoder.position = MP3_GetPosition;
			decoder.length = MP3_GetLength;
			decoder.seek = MP3_Seek;
			decoder.term = MP3_Term;
			break;

		case FILE_TYPE_OGG:
			decoder.init = OGG_Init;
			decoder.rate = OGG_GetSampleRate;
			decoder.channels = OGG_GetChannels;
			decoder.decode = OGG_Decode;
			decoder.position = OGG_GetPosition;
			decoder.length = OGG_GetLength;
			decoder.seek = OGG_Seek;
			decoder.term = OGG_Term;
			break;

		case FILE_TYPE_OPUS:
			decoder.init = OPUS_Init;
			decoder.rate = OPUS_GetSampleRate;
			decoder.channels = OPUS_GetChannels;
			decoder.decode = OPUS_Decode;
			decoder.position = OPUS_GetPosition;
			decoder.length = OPUS_GetLength;
			decoder.seek = OPUS_Seek;
			decoder.term = OPUS_Term;
			break;

		case FILE_TYPE_WAV:
			decoder.init = WAV_Init;
			decoder.rate = WAV_GetSampleRate;
			decoder.channels = WAV_GetChannels;
			decoder.decode = WAV_Decode;
			decoder.position = WAV_GetPosition;
			decoder.length = WAV_GetLength;
			decoder.seek = WAV_Seek;
			decoder.term = WAV_Term;
			break;

		case FILE_TYPE_XM:
			decoder.init = XM_Init;
			decoder.rate = XM_GetSampleRate;
			decoder.channels = XM_GetChannels;
			decoder.decode = XM_Decode;
			decoder.position = XM_GetPosition;
			decoder.length = XM_GetLength;
			decoder.seek = XM_Seek;
			decoder.term = XM_Term;
			break;

		default:
			break;
	}

	// El valor de retorno se miraba y se tiraba. Un decoder que no abrio deja
	// su handle en NULL, y los getters no lo comprueban - FLAC_GetSampleRate
	// es "return flac->sampleRate" a secas -, asi que seguir de aqui a
	// vitaAudioInit era leer de un handle que nunca existio. Cualquier archivo
	// con extension reconocida y contenido inservible tumbaba la aplicacion.
	if ((* decoder.init)(path) != 0) {
		// A decoder may have copied the lyrics before failing, and nobody else
		// will take them.
		free(metadata.lyrics);
		decoder = empty_decoder;
		metadata = empty_metadata;
		file_type = FILE_TYPE_NONE;
		track_loaded = SCE_FALSE;
		Audio_LogOpen(path, SCE_FALSE);
		return -1;
	}

	// decoder.init is where FLAC_Init / MP3_Init / OPUS_Init decode the embedded
	// cover art into metadata.cover_image, so this is the single point per track
	// load where the accent can follow it. Formats with no cover art at all (WAV,
	// OGG, the trackers) leave cover_image NULL and fall back to the fixed accent.
	UI_Theme_SetAccentFromCoverArt((metadata.has_meta && metadata.cover_image) ? metadata.cover_image : NULL);

	// No thread of this track writes yet, and the last one's ended in
	// Audio_Term, so nothing races the reset.
	viz_rate = (* decoder.rate)();
	VizTap_Reset(VizTap_Shared());

	// Opened paused, before the callback is set: the first buffer of the
	// track is already silence, and resuming fades it in from nothing.
	if (start_paused) {
		start_paused = SCE_FALSE;
		pause_request = 1;
		paused = SCE_TRUE;
		Fade_Reset(&ramp, 0.0f);
	}

	vitaAudioInit((* decoder.rate)(), (* decoder.channels)() == 2? SCE_AUDIO_OUT_MODE_STEREO : SCE_AUDIO_OUT_MODE_MONO);
	vitaAudioSetChannelCallback(0, Audio_Decode, NULL);
	track_loaded = SCE_TRUE;
	Audio_LogOpen(path, SCE_TRUE);
	return 0;
}

SceBool Audio_HasTrack(void) {
	return track_loaded;
}

SceBool Audio_IsPaused(void) {
	return pause_request ? SCE_TRUE : SCE_FALSE;
}

SceBool Audio_IsSilent(void) {
	return (!track_loaded || paused == SCE_TRUE) ? SCE_TRUE : SCE_FALSE;
}

void Audio_TogglePause(void) {
	pause_request = !pause_request;
}

void Audio_Hold(SceBool on) {
	hold = on ? 1 : 0;
}

void Audio_SetMasterGain(float gain) {
	if (!(gain > 0.0f)) // also catches NaN
		gain = 0.0f;
	else if (gain > 1.0f)
		gain = 1.0f;

	master_gain = gain;
}

void Audio_SetStartPaused(SceBool on) {
	start_paused = on;
}

void Audio_Stop(void) {
	playing = !playing;
}

SceUInt64 Audio_GetPosition(void) {
	return (* decoder.position)();
}

SceUInt64 Audio_GetLength(void) {
	return (* decoder.length)();
}

SceUInt64 Audio_GetPositionSeconds(void) {
	return (Audio_GetPosition() / (* decoder.rate)());
}

SceUInt64 Audio_GetLengthSeconds(void) {
	return (Audio_GetLength() / (* decoder.rate)());
}

SceUInt32 Audio_GetPositionMs(void) {
	SceUInt32 rate = (* decoder.rate)();

	if (rate == 0)
		return 0;

	SceUInt64 ms = Audio_GetPosition() * 1000ULL / rate;
	return (ms > 0xFFFFFFFFULL) ? 0xFFFFFFFFU : (SceUInt32)ms;
}

SceUInt64 Audio_Seek(double fraction) {
	if (!(fraction > 0.0)) // also catches NaN
		fraction = 0.0;
	else if (fraction > 1.0)
		fraction = 1.0;

	return (* decoder.seek)(fraction);
}

void Audio_Term(void) {
	playing = SCE_TRUE;
	paused = SCE_FALSE;
	track_loaded = SCE_FALSE;

	vitaAudioSetChannelCallback(0, NULL, NULL); // Clear channel callback
	vitaAudioEndPre();
	vitaAudioEnd(); // waits for the output thread itself
	(* decoder.term)();

	// After the thread ended, so it cannot act on a request left over.
	pause_request = 0;
	hold = 0;
	Fade_Reset(&ramp, 1.0f);

	// Clear metadata struct. The lyrics are normally gone already, taken by
	// Menu_InitMusic; this covers any path that loads a track without it.
	free(metadata.lyrics);
	metadata = empty_metadata;
	decoder = empty_decoder;

	// No track loaded any more, so the accent goes back to the fixed color.
	UI_Theme_ResetAccent();
}

void Audio_SetLyrics(const char *text, size_t len) {
	if (metadata.lyrics != NULL || text == NULL || len == 0 || len > LYRICS_MAX_BYTES)
		return;

	char *copy = malloc(len + 1);

	if (copy == NULL)
		return;

	memcpy(copy, text, len);
	copy[len] = '\0';
	metadata.lyrics = copy;
	metadata.lyrics_len = len;
}
