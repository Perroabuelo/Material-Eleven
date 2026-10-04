## Context

El porqué está en proposal.md. Acá va el estado del código que condiciona el cómo.

**Ciclo de vida de una pista hoy.**

```
  Audio_Init(path)
    decoder.init(path)        abre archivo/decoder, lee tags y caratula
    vitaAudioInit(...)        calloc de buffers, abre puerto, crea y arranca hilo "audiot0"
  ...
  Audio_Term()
    callback = NULL
    vitaAudioEndPre()         audio_terminate = 1
    sceKernelDelayThread(100 ms)          <- supone que el hilo ya salio
    vitaAudioEnd()            sceKernelDeleteThread (retorno ignorado), libera puerto y buffers
    decoder.term()
```

`sceKernelDeleteThread` solo funciona sobre un hilo que ya terminó. Si el hilo sigue dentro de un callback de decode, o bloqueado en `sceAudioOutOutput` después de los 100 ms, el borrado falla en silencio. Entonces quedan tomados su objeto y sus 64 KB de pila, y `vitaAudioEnd` libera los buffers que ese hilo todavía usa. El siguiente `vitaAudioInit` pone `audio_terminate = 0`, así que el hilo viejo, si seguía vivo, retoma su lazo con los buffers y el puerto de la pista nueva.

**Caminos de error de los decoders.**

| Archivo | Situación | Qué queda tomado |
|---|---|---|
| `xm.c` `XM_Init` | siempre | la copia `strdup(path)`, que nunca se libera |
| `xm.c` `XM_Init` | falla `xmp_load_module` | además, el `xmp_context` |
| `mp3.c` `MP3_Init` | falla `mpg123_param` u `mpg123_open` | el `mpg123_handle` |
| `ogg.c` `OGG_Init` | `ov_info` devuelve NULL | el `OggVorbis_File` y el descriptor |
| `opus.c` `OPUS_Init` | falla `op_current_link` | el `OggOpusFile` |
| `ogg.c` `OGG_Term` | siempre | nada; pero cierra dos veces el descriptor (`ov_clear` ya lo cierra con `close_func`) |

Los mismos lectores en `tags.c` y `cover.c` ya liberan bien en todos sus caminos y sirven de modelo.

**Lo que el overlay no ve.** `sceKernelGetFreeMemorySize` cuenta memblocks del kernel. El heap de newlib es un memblock que se reserva al arrancar, así que un `malloc` perdido no cambia la memoria libre que muestra el overlay. Una pila de hilo que no se borra sí la cambia.

## Goals / Non-Goals

**Goals:**
- Que al terminar `Audio_Term` no quede nada de la pista anterior, o, si el hilo no alcanzó a terminar, que no pueda volver a correr y se recoja después.
- Que cada `*_Init` que falla deje todo como estaba antes de llamarlo.
- Poder medir el heap en la consola.

**Non-Goals:**
- Cambiar los decoders o la forma de leer tags y carátulas.
- Reducir la carátula a resolución completa.

## Decisions

### 1. Esperar al hilo en vez de dormir, con generación y recogida diferida

En `vitaaudiolib.c`:

- **Generación.** Un contador `audio_generation` sube en cada `vitaAudioInit`. Cada hilo recibe su generación como argumento y sale de su lazo cuando `audio_terminate` está puesto **o** cuando su generación ya no es la vigente. Así un hilo viejo no puede revivir aunque `vitaAudioInit` vuelva a poner `audio_terminate = 0`.
- **Espera real.** `vitaAudioEnd` reemplaza el `DelayThread` fijo de `Audio_Term` por `sceKernelWaitThreadEnd` con un timeout de 1 s. Si el hilo terminó, lo borra y libera puerto y buffers, como hoy. En el caso normal, el hilo sale al terminar su grano actual (unos 20 ms), así que el cambio de pista no se alarga y, en general, se acorta.
- **Recogida diferida.** Si el timeout vence, el hilo, su puerto y sus buffers pasan a una lista corta de pendientes (4 entradas) en vez de liberarse. Al principio de cada `vitaAudioEnd` y `vitaAudioInit` se intenta recoger cada pendiente con timeout 0. Liberar los buffers de un hilo que sigue vivo es justamente el error que se evita. Si la lista está llena, se espera sin timeout al más antiguo antes de agregar otro. El overlay de debug cuenta las veces que hubo que diferir una recogida.
- `Audio_Term` deja de llamar a `sceKernelDelayThread(100 * 1000)`.

- **Alternativa descartada: esperar sin timeout.** Es lo más simple, pero si el decode quedara bloqueado en una lectura que no vuelve (tarjeta retirada), la interfaz se congelaría para siempre.
- **Alternativa descartada: subir la espera fija a 500 ms.** Hace el problema más raro, pero no lo elimina, y alarga cada cambio de pista.

### 2. Cada `*_Init` deshace lo suyo al fallar

Cada camino de error libera, en orden inverso, lo que se tomó antes, como ya hacen `FLAC_Init` y los lectores de `tags.c`:

- `XM_Init`: se elimina el `strdup`. `xmp_load_module` acepta `char *`, y se le pasa una copia en un arreglo local de `LIBRARY_PATH_MAX`, como hace `Tags_ReadModule`. Si la carga falla, se llama a `xmp_free_context`.
- `MP3_Init`: si falla cualquier paso después de `mpg123_new`, se llama a `mpg123_delete`, y además a `mpg123_close` si la apertura había funcionado.
- `OGG_Init`: si `ov_info` devuelve NULL, se llama a `ov_clear`, que también cierra el descriptor.
- `OPUS_Init`: si falla `op_current_link`, se llama a `op_free`.
- Ningún camino de error carga carátula antes de fallar, así que `metadata.cover_image` no queda huérfana. Se deja anotado en un comentario para quien agregue un paso después.

### 3. `OGG_Term` cierra el descriptor una sola vez

Se quita el `sceIoClose(ogg_file)` que sigue a `ov_clear`. `ov_clear` ya lo cierra a través de `ogg_callback_close`, igual que en `tags.c`.

### 4. El nombre del archivo se copia, no se formatea

En `Menu_InitMusic`, `snprintf(filename, 128, Utils_Basename(path))` pasa a `snprintf(filename, 128, "%s", Utils_Basename(path))`. Se comprueba además que el `malloc` de las tres cadenas haya funcionado. Si alguno falla, se liberan las otras y `Menu_InitMusic` devuelve `SCE_FALSE`, que es el camino de "no se pudo abrir" que ya existe.

### 5. Casos sin memoria que hoy pierden datos o se caen

- `Library_Scan` (`library.c`): si falla el `malloc` de `stack`, hoy retorna sin liberar `old_tracks` ni `carry` y deja la biblioteca vacía en memoria. Pasa a liberar `carry` y a restaurar `old_tracks` como índice vigente, con `library_count = old_count` y una capacidad igual a esa cuenta. El camino de abandono recarga el índice desde disco; acá basta con devolver el que ya está en memoria, sin leer la tarjeta justo cuando no hay memoria.
- `Dirbrowse_PopulateFiles` y `Queue_FillFromFolder`: comprueban el `calloc` de las entradas y, si falla, cierran el directorio y devuelven error en vez de escribir sobre NULL. `Dirbrowse_PopulateFiles` también comprueba cada `malloc` de nodo.

### 6. El overlay de debug muestra el heap en uso

`UI_Debug_SampleMemory`, que ya corre cada 120 fotogramas, lee también `mallinfo().uordblks` (bytes en uso del heap de newlib) y lo muestra en KB en una línea "HEAP", junto con el contador de recogidas diferidas de la decisión 1.

## Risks / Trade-offs

- **[`mallinfo` no disponible o sin datos en el newlib de VitaSDK]** → Se comprueba en la primera tarea, compilando y mirando el overlay en la consola. Si no sirve, la alternativa es contar `malloc`/`free` con `-Wl,--wrap=malloc,--wrap=free` solo en el overlay. En ese caso la decisión 6 se actualiza antes de seguir.
- **[El hilo recibe la generación por puntero a una variable que cambia]** → Hoy `sceKernelStartThread` recibe `&i` y copia `sizeof(i)` bytes en la pila del hilo, así que el valor queda fijo. La generación se pasa igual, por copia, en la misma estructura de argumentos.
- **[`sceKernelWaitThreadEnd` con un timeout en microsegundos]** → La firma toma un `SceUInt *timeout` que el kernel actualiza. Se usa una variable local por llamada.
- **[Una pista de tracker que tarda en cargar]** → Quitar el `strdup` no cambia cómo carga libxmp; solo de dónde sale la cadena.

## Migration Plan

No hay datos que migrar. Para volver atrás basta revertir los commits.

## Estrategia de pruebas

- **Compilación**: `scripts/build.sh` sin errores ni avisos con `-Wall -Werror`.
- **Pruebas en PC**: no aplican. Todo lo que cambia son llamadas a SCE o a los decoders, sin lógica pura nueva que valga aislar. `make -C tests` sigue pasando sin cambios.
- **Pruebas en consola**: los scenarios de `specs/playback/track-lifecycle`, con el overlay de debug. La línea HEAP se agrega primero, antes de las correcciones, para ver en la consola que las fugas existen: el heap sube al recorrer módulos de tracker y archivos dañados. Después se repite con las correcciones. Hace falta preparar en la tarjeta una carpeta con pistas de todos los formatos, una con módulos de tracker, y otra con una pista buena y archivos dañados de cada extensión.

## CI

Sin cambios en el pipeline.

## Código de terceros

No se incorpora código ni assets de terceros.
