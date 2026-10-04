# Pruebas en PC de la logica pura, con el gcc del host (no el del VitaSDK).
#
#     make -C tests          compila y corre todas las pruebas
#     make -C tests clean

CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Werror
INCLUDE := -I../include

BIN := test_lang test_screen_off test_accent test_track_meta test_library_row test_nav_request

.PHONY: all clean

all: $(BIN)
	@for t in $(BIN); do ./$$t || exit 1; done

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

clean:
	rm -f $(BIN)
