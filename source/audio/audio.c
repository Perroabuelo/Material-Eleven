#include <psp2/kernel/threadmgr.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"
#include "ui_theme.h"
#include "vitaaudiolib.h"
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
	SceUInt64 (* seek)(SceUInt64 index);
	void (* term)(void);
} Audio_Decoder;

static enum Audio_FileType file_type = FILE_TYPE_NONE;
Audio_Metadata metadata = {0};
static Audio_Metadata empty_metadata = {0};
static Audio_Decoder decoder = {0}, empty_decoder = {0};
static SceBool track_loaded = SCE_FALSE;
SceBool playing = SCE_TRUE, paused = SCE_FALSE;

static void Audio_Decode(void *buf, unsigned int length, void *userdata) {
	if (playing == SCE_FALSE || paused == SCE_TRUE) {
		// vitaAudioInit reserva grain * canales shorts, asi que el silencio se
		// escribe con esa misma cuenta. Esto era un "length * 2" fijo: en un
		// track mono escribia el doble del buffer y pisaba el heap en cada
		// callback, unas cincuenta veces por segundo. Se entra aqui al pausar,
		// y tambien al acabarse el archivo, que es cuando el decoder pone
		// playing en SCE_FALSE, de modo que bastaba con dejar sonando un mono
		// hasta el final para corromper el heap.
		memset(buf, 0, length * vitaAudioGetChannelCount() * sizeof(short));
	}
	else
		(* decoder.decode)(buf, length, userdata);
}

int Audio_Init(const char *path) {
	playing = SCE_TRUE;
	paused = SCE_FALSE;
	
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
		decoder = empty_decoder;
		metadata = empty_metadata;
		file_type = FILE_TYPE_NONE;
		track_loaded = SCE_FALSE;
		return -1;
	}

	// decoder.init is where FLAC_Init / MP3_Init / OPUS_Init decode the embedded
	// cover art into metadata.cover_image, so this is the single point per track
	// load where the accent can follow it. Formats with no cover art at all (WAV,
	// OGG, the trackers) leave cover_image NULL and fall back to the fixed accent.
	UI_Theme_SetAccentFromCoverArt((metadata.has_meta && metadata.cover_image) ? metadata.cover_image : NULL);

	vitaAudioInit((* decoder.rate)(), (* decoder.channels)() == 2? SCE_AUDIO_OUT_MODE_STEREO : SCE_AUDIO_OUT_MODE_MONO);
	vitaAudioSetChannelCallback(0, Audio_Decode, NULL);
	track_loaded = SCE_TRUE;
	return 0;
}

SceBool Audio_HasTrack(void) {
	return track_loaded;
}

SceBool Audio_IsPaused(void) {
	return paused;
}

void Audio_Pause(void) {
	paused = !paused;
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

SceUInt64 Audio_Seek(SceUInt64 index) {
	return (* decoder.seek)(index);
}

void Audio_Term(void) {
	playing = SCE_TRUE;
	paused = SCE_FALSE;
	track_loaded = SCE_FALSE;

	vitaAudioSetChannelCallback(0, NULL, NULL); // Clear channel callback
	vitaAudioEndPre();
	vitaAudioEnd(); // waits for the output thread itself
	(* decoder.term)();

	// Clear metadata struct
	metadata = empty_metadata;
	decoder = empty_decoder;

	// No track loaded any more, so the accent goes back to the fixed color.
	UI_Theme_ResetAccent();
}
