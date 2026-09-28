# Pruebas en PC de la logica pura, con el gcc del host (no el del VitaSDK).
#
#     make -C tests          compila y corre todas las pruebas
#     make -C tests clean

CC      ?= gcc
CFLAGS  ?= -std=gnu11 -O0 -g -Wall -Werror
INCLUDE := -I../include

BIN := test_lang

.PHONY: all clean

all: $(BIN)
	@for t in $(BIN); do ./$$t || exit 1; done

test_lang: test_lang.c ../source/lang.c ../include/lang.h ../include/lang_strings.h
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ test_lang.c ../source/lang.c

clean:
	rm -f $(BIN)
