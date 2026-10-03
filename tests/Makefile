# Pruebas en PC de la logica pura, con el gcc del host (no el del VitaSDK).
#
#     make -C tests          compila y corre todas las pruebas
#     make -C tests clean

CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Werror
INCLUDE := -I../include

BIN := test_lang test_screen_off

.PHONY: all clean

all: $(BIN)
	@for t in $(BIN); do ./$$t || exit 1; done

test_lang: test_lang.c ../source/lang.c ../include/lang.h ../include/lang_strings.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_lang.c ../source/lang.c

test_screen_off: test_screen_off.c ../source/screen_off.c ../include/screen_off.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_screen_off.c ../source/screen_off.c

clean:
	rm -f $(BIN)
