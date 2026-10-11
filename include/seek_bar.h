#ifndef _ELEVENMPV_SEEK_BAR_H_
#define _ELEVENMPV_SEEK_BAR_H_

// A progress bar the user can grab and drag. A press inside its touch area
// grabs it; while the finger stays down only its x counts, and lifting it
// asks for one seek to the finger's last position.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_seek_bar.c). The screen builds the input from touch.c and does
// the seek itself; this module knows nothing about audio.

typedef struct {
	float x, w;                  // left end and drawn width
	float hit_top, hit_bottom;   // vertical touch area
} SeekBar_Geometry;

typedef struct {
	int grabbed;
	float fraction;              // finger position while grabbed
} SeekBar_State;

typedef struct {
	int pressed, held, released;
	float x, y;
} SeekBar_Input;

typedef enum {
	SEEKBAR_IDLE,                // not grabbed
	SEEKBAR_DRAGGING,            // grabbed: draw the finger's fraction, do not seek
	SEEKBAR_COMMIT               // just let go: seek once to *out_fraction
} SeekBar_Result;

// Lets go without a seek. Used when the screen, the view or the track changes
// under the finger.
void SeekBar_Reset(SeekBar_State *state);

// Advances the bar by one frame of input. On SEEKBAR_COMMIT, *out_fraction
// (if not NULL) is the place to seek to, in [0, 1].
SeekBar_Result SeekBar_Update(SeekBar_State *state, const SeekBar_Geometry *geo,
	const SeekBar_Input *in, double *out_fraction);

// Share of the bar left of `x`, clamped to [0, 1]. 0 for a bar with no width.
float SeekBar_FractionAt(const SeekBar_Geometry *geo, float x);

// What to draw: the finger's fraction while grabbed, the playback one if not.
float SeekBar_DisplayFraction(const SeekBar_State *state, float playback_fraction);

#endif
