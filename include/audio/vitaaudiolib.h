#ifndef _ELEVENMPV_AUDIO_LIB_H_
#define _ELEVENMPV_AUDIO_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <psp2/audioout.h>

#define VITA_NUM_AUDIO_CHANNELS 1
#define VITA_DEFAULT_AUDIO_SAMPLES 960

typedef void (* vitaAudioCallback_t)(void *stream, unsigned int length, void *userdata);

typedef struct {
	int threadhandle;
	int handle;
	int volumeleft;
	int volumeright;
	vitaAudioCallback_t callback;
	void *userdata;
} VITA_audio_channelinfo;

typedef int (* vitaAudioThreadfunc_t)(int args, void *argp);

// Not declared by vitasdk's psp2/audioout.h, but present in the stub library.
// Applies a hardware EQ preset to BGM audio output: 0 = Off, 1 = Heavy, 2 = Pop, 3 = Jazz, 4 = Unique.
extern int sceAudioOutSetEffectType(int type);

void vitaAudioSetVolume(int channel, int left, int right);
void vitaAudioSetChannelCallback(int channel, vitaAudioCallback_t callback, void *userdata);
int vitaAudioOutBlocking(unsigned int channel, unsigned int vol1, unsigned int vol2, const void *buf);
int vitaAudioInit(int frequency, SceAudioOutMode mode);
void vitaAudioEndPre(void);
void vitaAudioEnd(void);
void vitaAudioPreSetGrain(unsigned int grain);
unsigned int vitaAudioGetGrain(void);
// Canales con los que vitaAudioInit dimensiono el buffer de salida. Quien
// escriba en ese buffer tiene que contar con esto y no suponer estereo.
unsigned int vitaAudioGetChannelCount(void);
unsigned int vitaAudioGetDefaultGrain(void);
// Times vitaAudioEnd gave up waiting for an output thread and left it, with its
// port and buffers, to be freed once it ends. Shown in the debug overlay.
unsigned int vitaAudioGetDeferredReaps(void);

#ifdef __cplusplus
}
#endif

#endif
