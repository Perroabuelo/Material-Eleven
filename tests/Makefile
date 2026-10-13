# Pruebas en PC de la logica pura, con el gcc del host (no el del VitaSDK).
#
#     make -C tests              compila y corre todas las pruebas
#     make -C tests SANITIZE=0   sin sanitizers, para depurar con gdb
#     make -C tests clean
#
# Por defecto compila con AddressSanitizer (incluye LeakSanitizer) y UBSan:
# una fuga, una escritura fuera de buffer o un comportamiento indefinido hace
# fallar la prueba. Con -fno-sanitize-recover, UBSan corta en vez de avisar.

CC       ?= gcc
SANITIZE ?= 1

ifeq ($(SANITIZE),1)
SANFLAGS := -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined
else
SANFLAGS :=
endif

CFLAGS  ?= -std=gnu11 -O1 -g -Wall -Werror
CFLAGS  += $(SANFLAGS)
INCLUDE := -I../include

BIN := test_lang test_screen_off test_accent test_track_meta test_library_row test_nav_request test_capture_bmp test_text_encoding test_lyrics test_lyrics_layout test_track_advance test_seek_bar test_key_repeat test_text_fold test_list_filter test_gesture test_list_scroll test_playlist

.PHONY: all clean FORCE

all: $(BIN)
	@for t in $(BIN); do ./$$t || exit 1; done

# Rewritten only when the compiler or the flags change (SANITIZE=0, say), so
# every test depends on it and gets rebuilt with the new flags.
.cflags: FORCE
	@echo '$(CC) $(CFLAGS)' | cmp -s - $@ || echo '$(CC) $(CFLAGS)' > $@

$(BIN): .cflags

test_lang: test_lang.c ../source/lang.c ../include/lang.h ../include/lang_strings.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_lang.c ../source/lang.c

test_screen_off: test_screen_off.c ../source/screen_off.c ../include/screen_off.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_screen_off.c ../source/screen_off.c

test_accent: test_accent.c ../source/accent.c ../include/accent.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_accent.c ../source/accent.c -lm

test_track_meta: test_track_meta.c ../source/track_meta.c ../include/track_meta.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_track_meta.c ../source/track_meta.c

test_library_row: test_library_row.c ../source/library_row.c ../include/library_row.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_library_row.c ../source/library_row.c

test_nav_request: test_nav_request.c ../source/nav_request.c ../include/nav_request.h ../include/ui_screen.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_nav_request.c ../source/nav_request.c

test_capture_bmp: test_capture_bmp.c ../source/capture_bmp.c ../include/capture_bmp.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_capture_bmp.c ../source/capture_bmp.c

test_text_encoding: test_text_encoding.c ../source/text_encoding.c ../include/text_encoding.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_text_encoding.c ../source/text_encoding.c

test_lyrics: test_lyrics.c ../source/lyrics.c ../include/lyrics.h ../source/text_encoding.c ../include/text_encoding.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_lyrics.c ../source/lyrics.c ../source/text_encoding.c

test_lyrics_layout: test_lyrics_layout.c ../source/lyrics_layout.c ../include/lyrics_layout.h ../source/lyrics.c ../include/lyrics.h ../source/text_encoding.c ../include/text_encoding.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_lyrics_layout.c ../source/lyrics_layout.c ../source/lyrics.c ../source/text_encoding.c

test_track_advance: test_track_advance.c ../source/track_advance.c ../include/track_advance.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_track_advance.c ../source/track_advance.c

test_seek_bar: test_seek_bar.c ../source/seek_bar.c ../include/seek_bar.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_seek_bar.c ../source/seek_bar.c

test_key_repeat: test_key_repeat.c ../source/key_repeat.c ../include/key_repeat.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_key_repeat.c ../source/key_repeat.c

test_text_fold: test_text_fold.c ../source/text_fold.c ../include/text_fold.h ../source/text_encoding.c ../include/text_encoding.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_text_fold.c ../source/text_fold.c ../source/text_encoding.c

test_list_filter: test_list_filter.c ../source/list_filter.c ../include/list_filter.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_list_filter.c ../source/list_filter.c

test_gesture: test_gesture.c ../source/gesture.c ../include/gesture.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_gesture.c ../source/gesture.c

test_list_scroll: test_list_scroll.c ../source/list_scroll.c ../include/list_scroll.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_list_scroll.c ../source/list_scroll.c -lm

test_playlist: test_playlist.c ../source/playlist.c ../include/playlist.h ../source/text_fold.c ../include/text_fold.h ../source/text_encoding.c ../include/text_encoding.h
	$(CC) $(CFLAGS) $(INCLUDE) -DPLAYLIST_TEST_ALLOC -o $@ test_playlist.c ../source/playlist.c ../source/text_fold.c ../source/text_encoding.c

clean:
	rm -f $(BIN) .cflags
