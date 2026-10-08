#include <ctype.h>
#include <psp2/io/fcntl.h>
#include <FLAC/metadata.h>

#include "audio.h"
#include "config.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

// Large read-ahead buffer around the raw file descriptor. dr_flac's default
// file callbacks otherwise issue many small sceIoRead syscalls while
// scanning/seeking, which is slow on the memory card / SD2VITA.
#define FLAC_IO_BUFFER_SIZE (128 * 1024)

typedef struct {
	SceUID fd;
	unsigned char *buffer;
	SceOff buffer_file_offset;
	SceSize buffer_fill;
	SceOff pos;
} Flac_BufferedIo;

static Flac_BufferedIo flac_io;

static size_t FLAC_ReadCB(void *user_data, void *buffer_out, size_t bytes_to_read) {
	Flac_BufferedIo *io = (Flac_BufferedIo *)user_data;
	unsigned char *out = (unsigned char *)buffer_out;
	size_t total_read = 0;

	while (bytes_to_read > 0) {
		if (io->pos >= io->buffer_file_offset && io->pos < io->buffer_file_offset + (SceOff)io->buffer_fill) {
			SceSize offset_in_buffer = (SceSize)(io->pos - io->buffer_file_offset);
			SceSize available = io->buffer_fill - offset_in_buffer;
			SceSize to_copy = (bytes_to_read < available) ? (SceSize)bytes_to_read : available;

			memcpy(out, io->buffer + offset_in_buffer, to_copy);

			out += to_copy;
			io->pos += to_copy;
			total_read += to_copy;
			bytes_to_read -= to_copy;
		}
		else {
			SceSSize ret;

			sceIoLseek(io->fd, io->pos, SCE_SEEK_SET);
			ret = sceIoRead(io->fd, io->buffer, FLAC_IO_BUFFER_SIZE);

			if (ret <= 0) {
				io->buffer_fill = 0;
				break;
			}

			io->buffer_file_offset = io->pos;
			io->buffer_fill = (SceSize)ret;
		}
	}

	return total_read;
}

static drflac_bool32 FLAC_SeekCB(void *user_data, int offset, drflac_seek_origin origin) {
	Flac_BufferedIo *io = (Flac_BufferedIo *)user_data;

	if (origin == drflac_seek_origin_start)
		io->pos = offset;
	else
		io->pos += offset;

	return DRFLAC_TRUE;
}

static drflac *flac;
static drflac_uint64 frames_read = 0;

int FLAC_Init(const char *path) {
	flac_io.fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (flac_io.fd < 0)
		return -1;

	flac_io.buffer = malloc(FLAC_IO_BUFFER_SIZE);
	flac_io.buffer_file_offset = 0;
	flac_io.buffer_fill = 0;
	flac_io.pos = 0;

	if (flac_io.buffer == NULL) {
		sceIoClose(flac_io.fd);
		return -1;
	}

	flac = drflac_open(FLAC_ReadCB, FLAC_SeekCB, &flac_io, NULL);
	if (flac == NULL) {
		free(flac_io.buffer);
		sceIoClose(flac_io.fd);
		return -1;
	}

	FLAC__StreamMetadata *tags;
	if (FLAC__metadata_get_tags(path, &tags)) {
		for (int i = 0; i < tags->data.vorbis_comment.num_comments; i++)  {
			char *tag = (char *)tags->data.vorbis_comment.comments[i].entry;

			if (!strncasecmp("TITLE=", tag, 6)) {
				metadata.has_meta = SCE_TRUE;
				snprintf(metadata.title, 31, "%s\n", tag + 6);
			}

			if (!strncasecmp("ALBUM=", tag, 6)) {
				metadata.has_meta = SCE_TRUE;
				snprintf(metadata.album, 31, "%s\n", tag + 6);
			}

			if (!strncasecmp("ARTIST=", tag, 7)) {
				metadata.has_meta = SCE_TRUE;
				snprintf(metadata.artist, 31, "%s\n", tag + 7);
			}

			if (!strncasecmp("DATE=", tag, 5)) {
				metadata.has_meta = SCE_TRUE;
				snprintf(metadata.year, 31, "%d\n", atoi(tag + 5));
			}

			if (!strncasecmp("COMMENT=", tag, 8)) {
				metadata.has_meta = SCE_TRUE;
				snprintf(metadata.comment, 31, "%s\n", tag + 8);
			}

			if (!strncasecmp("GENRE=", tag, 6)) {
				metadata.has_meta = SCE_TRUE;
				snprintf(metadata.genre, 31, "%s\n", tag + 6);
			}

			// The first of the two that shows up; read whatever the cover
			// settings say.
			FLAC__uint32 tag_len = tags->data.vorbis_comment.comments[i].length;

			if (tag_len >= 7 && !strncasecmp("LYRICS=", tag, 7))
				Audio_SetLyrics(tag + 7, tag_len - 7);
			else if (tag_len >= 15 && !strncasecmp("UNSYNCEDLYRICS=", tag, 15))
				Audio_SetLyrics(tag + 15, tag_len - 15);
		}
	}

	if (tags)
		FLAC__metadata_object_delete(tags);

	// data_length y no length: length es el tamaño del bloque de metadatos
	// entero - tipo MIME, descripcion y dimensiones incluidos -, no el de la
	// imagen. libjpeg no se atragantaba porque para en el marcador EOI, pero le
	// llegaban de mas los bytes de cabecera que preceden a los datos.
	FLAC__StreamMetadata *picture;
	if (config.meta_flac && FLAC__metadata_get_picture(path, &picture, FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER, "image/jpg", NULL, (unsigned)(-1), (unsigned)(-1),
		(unsigned)(-1), (unsigned)(-1))) {
		metadata.has_meta = SCE_TRUE;
		metadata.cover_image = vita2d_load_JPEG_buffer(picture->data.picture.data, picture->data.picture.data_length);
		FLAC__metadata_object_delete(picture);
	}
	else if (config.meta_flac && FLAC__metadata_get_picture(path, &picture, FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER, "image/jpeg", NULL, (unsigned)(-1), (unsigned)(-1),
		(unsigned)(-1), (unsigned)(-1))) {
		metadata.has_meta = SCE_TRUE;
		metadata.cover_image = vita2d_load_JPEG_buffer(picture->data.picture.data, picture->data.picture.data_length);
		FLAC__metadata_object_delete(picture);
	}
	else if (config.meta_flac && FLAC__metadata_get_picture(path, &picture, FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER, "image/png", NULL, (unsigned)(-1), (unsigned)(-1),
		(unsigned)(-1), (unsigned)(-1))) {
		metadata.has_meta = SCE_TRUE;
		metadata.cover_image = vita2d_load_PNG_buffer(picture->data.picture.data);
		FLAC__metadata_object_delete(picture);
	}

	return 0;
}

SceUInt32 FLAC_GetSampleRate(void) {
	return flac->sampleRate;
}

SceUInt8 FLAC_GetChannels(void) {
	return flac->channels;
}

void FLAC_Decode(void *buf, unsigned int length, void *userdata) {
	frames_read += drflac_read_pcm_frames_s16(flac, (drflac_uint64)length, (drflac_int16 *)buf);
	
	if (frames_read >= flac->totalPCMFrameCount)
		playing = SCE_FALSE;
}

SceUInt64 FLAC_GetPosition(void) {
	return frames_read;
}

SceUInt64 FLAC_GetLength(void) {
	return flac->totalPCMFrameCount;
}

SceUInt64 FLAC_Seek(SceUInt64 index) {
	drflac_uint64 seek_frame = (flac->totalPCMFrameCount * (index / 450.0));
	
	if (drflac_seek_to_pcm_frame(flac, seek_frame) == DRFLAC_TRUE) {
		frames_read = seek_frame;
		return frames_read;
	}

	return -1;
}

void FLAC_Term(void) {
	frames_read = 0;

	if (metadata.has_meta)
		metadata.has_meta = SCE_FALSE;

	drflac_close(flac);

	free(flac_io.buffer);
	sceIoClose(flac_io.fd);
	flac_io.buffer = NULL;
}

// Functions needed for libFLAC

int utime(const char *filename, const void *buf) {
	return 0;
}

int utimensat(int dirfd, const char *pathname, const void *times, int flags) {
	return 0;
}
