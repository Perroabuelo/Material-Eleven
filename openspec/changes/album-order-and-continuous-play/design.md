## Context

El porqué está en proposal.md. Acá va el estado del código que condiciona el cómo.

**Tags.** `Tags_Read` (`tags.c`) llena solo `title`, `artist` y `album`. FLAC y OGG pasan por `Tags_TakeVorbisComment`, Opus por `opus_tags_query`, y MP3 por `mpg123_id3`, que devuelve `mpg123_id3v2` (con el arreglo `text[]` de frames de texto, entre ellos `TRCK` y `TPOS`) y `mpg123_id3v1`. Los módulos de tracker y WAV no traen números.

**Índice.** `Library_Track` guarda ruta, tres tags, tamaño, fecha, extensión y `tagged`. `Library_Save` escribe una cabecera con `LIBRARY_INDEX_VERSION 2` y una fila por pista con 8 campos separados por tabuladores, con la ruta al final. `Library_Load` descarta el archivo entero si la versión no es exactamente 2. Al reescanear, `Library_CarryTags` copia título, artista y álbum del índice anterior **solo si la pista estaba `tagged`** y el archivo no cambió (mismo tamaño y fecha).

**Vistas.** `Library_BuildFieldTracks` (dentro de un artista o un álbum) ordena con `Library_CmpTitle`, el mismo comparador que la vista Canciones. `Library_BuildFieldNames` arma los nombres distintos con `strcasecmp`, pone el cubo vacío al final y cuenta las pistas de cada nombre. Hay un solo arreglo de vista (`library_view`), compartido por todas las vistas.

**Cola.** `Queue_Add(path, title)` guarda la ruta y el título. `Queue_PeekAhead` devuelve los dos. La cola envuelve en los extremos y su techo (4000) es igual al de la biblioteca.

**Carátulas.** `Cover_Get(album, path, &budget)` busca en 16 slots en RAM (LRU) y, si el presupuesto lo permite, lee la entrada de disco (cabecera de 128 bytes más 64 KB de píxeles). La clave es el álbum, o la ruta si la pista no tiene álbum. Devuelve `NULL` tanto si la entrada no tiene imagen como si no hubo presupuesto.

**Ajustes.** `config.cfg` agrega líneas al final en cada versión. `Config_Load` lee con `sscanf` el prefijo que haya y completa lo que falta (v2 → v3 ya se hizo así).

## Goals / Non-Goals

**Goals:**
- Que todo lo que decide un orden (interpretar números, comparar pistas, leer una fila del índice) sea código puro con pruebas en PC.
- Que actualizar no deje la biblioteca vacía ni pierda títulos en ningún momento, tampoco durante la relectura de tags.
- Que la vista visible y la cola nunca se pisen, aunque compartan el arreglo de vista.

**Non-Goals:**
- Cambiar el formato de la caché de carátulas o la pasada de carátulas.
- Releer tags en un hilo de fondo. La pasada sigue siendo la pantalla que existe hoy, abandonable.

## Decisions

### 1. Módulo puro `track_meta` para números y orden

Se agrega `source/track_meta.c` / `include/track_meta.h`, sin vita2d ni SCE ni `psp2/types.h`:

```c
// "3", "03", "3/12" -> 3. Vacio, "abc", "/12", NULL -> 0 (ausente).
int TrackMeta_ParseNumber(const char *text);

// ID3v1.1: si comment[28] == 0 y comment[29] != 0, el numero de pista es comment[29].
int TrackMeta_Id3v1Track(const unsigned char comment[30]);

typedef struct { const char *group; int disc, track; const char *title, *path; } TrackMeta_Key;

// Orden del disco: disco (0 cuenta como 1) -> pista (0 al final) -> titulo -> ruta.
int TrackMeta_CompareDisc(const TrackMeta_Key *a, const TrackMeta_Key *b);

// Grupo (vacio al final, strcasecmp) y, dentro del grupo, orden del disco.
int TrackMeta_CompareGrouped(const TrackMeta_Key *a, const TrackMeta_Key *b);
```

`library.c` arma las claves a partir de `Library_Track` y llama a estas funciones desde sus comparadores de `qsort`. El título de la clave es el mismo respaldo que usa la vista (`Library_SortTitle`: el título o, si falta, el nombre de archivo).

- **Alternativa descartada:** escribir los comparadores dentro de `library.c`. Ese archivo incluye vita2d (pantallas de progreso) y no compila en PC, así que el orden quedaría sin prueba automática, y el orden es justamente lo que este cambio arregla.

### 2. `Tags` gana `track` y `disc`

`Tags` suma `int track, disc` (0 = ausente). Cada lector los llena con `TrackMeta_ParseNumber`:
- Comentarios Vorbis (FLAC y OGG): `TRACKNUMBER=` y `DISCNUMBER=`, sin distinguir mayúsculas.
- Opus: `opus_tags_query(tags, "tracknumber", 0)` y `"discnumber"`.
- ID3v2: se recorre `v2->text[0..texts)` buscando los ids `TRCK` y `TPOS`.
- ID3v1.1: si v2 no trajo número de pista, `TrackMeta_Id3v1Track(v1->comment)`.

Si el `mpg123.h` del VitaSDK no expone `text`/`texts` en `mpg123_id3v2`, la tarea 1.2 lo anota y MP3 se queda solo con ID3v1.1. Esa API existe desde hace muchas versiones de mpg123, así que no se espera que falte.

### 3. Índice v3 que acepta v2

`Library_Track` suma `track` y `disc` (enteros chicos). La fila v3 pasa a tener 10 campos, con la ruta todavía al final:

```
size  mtime  tagged  ext  track  disc  title  artist  album  path      (v3)
size  mtime  tagged  ext               title  artist  album  path      (v2)
```

Qué campo está en cada posición según la versión lo decide un módulo puro, `source/library_row.c` / `include/library_row.h`:

```c
typedef struct { int size, mtime, tagged, ext, track, disc, title, artist, album, path; int fields; } LibraryRow_Layout;
// SCE_FALSE-equivalente (0) si la version no se reconoce. track/disc = -1 cuando la version no los trae.
int LibraryRow_GetLayout(int version, LibraryRow_Layout *out);
```

`Library_Load` acepta las versiones 2 y 3. Una fila v2 se carga con `track = disc = 0` y **`tagged = false`**, sea cual sea lo que diga su campo. Cualquier otra versión se sigue descartando. `Library_Save` escribe siempre v3, así que el índice se convierte en la primera escritura.

- **Alternativa descartada: descartar el índice v2, como dice la política actual.** El usuario lo descartó: perdería la biblioteca al actualizar y tendría que esperar el escaneo completo. Cargar v2 no es una migración de datos: solo reutiliza la marca de "tags pendientes" que la pasada reanudable ya sabe tratar.

### 4. Reescanear conserva el texto de las pistas pendientes

Hoy `Library_CarryTags` copia título, artista y álbum solo si la pista estaba `tagged`. Después de actualizar, todas quedan con `tagged = false`, así que el primer reescaneo las dejaría sin título, artista ni álbum hasta que la pasada de tags llegara a cada una. Eso vaciaría las vistas Artistas y Álbumes durante la relectura.

La copia pasa a hacerse siempre que el archivo no cambió (misma ruta, tamaño y fecha), y lo que se copia de `tagged` es su valor, no siempre `true`. Una pista nunca leída tiene los campos vacíos, así que copiarlos no cambia nada. `track` y `disc` se copian con las mismas reglas.

### 5. Comparadores por vista

| Vista | Orden |
|---|---|
| Dentro de un álbum | `TrackMeta_CompareDisc` |
| Dentro de un artista | `TrackMeta_CompareGrouped` con `group = album` |
| Canciones, Añadidos recientemente | Sin cambios (`Library_CmpTitle`, `Library_CmpRecent`) |

### 6. La cola continua es una vista más: `Library_BuildContinuous`

```c
// Toda la biblioteca. ALBUM: album -> disco -> pista. ARTIST: artista -> album -> disco -> pista.
// El grupo vacio va al final, como el cubo "Desconocido" en las vistas de nombres.
int Library_BuildContinuous(Library_Field field);
```

Para el álbum se usa `TrackMeta_CompareGrouped` con `group = album`. Para el artista, se compara primero el artista (vacío al final) y, si empatan, se aplica `TrackMeta_CompareGrouped` con `group = album`. Así el orden de los grupos es exactamente el de `Library_BuildFieldNames` (`strcasecmp`, vacío al final), y el de cada grupo es el que se ve al entrar en él.

**El arreglo de vista es compartido.** `Menu_LibraryPlaySelected` hace, en este orden:
1. Toma la ruta de la pista elegida de la vista visible.
2. Si está dentro de un grupo y el ajuste es "Seguir con el siguiente", llama a `Library_BuildContinuous` con el campo del grupo. Si no, usa la vista visible, como hoy.
3. Llena la cola desde esa vista.
4. Pone `view_dirty = SCE_TRUE`, para que el siguiente fotograma reconstruya la lista que el usuario estaba viendo.
5. Reproduce con `Menu_PlayQueued(ruta)`, que ya planta la cola en esa ruta con `Queue_SeekToPath`.

- **Alternativa descartada: una cola que crece al llegar al final del grupo.** Exige que la cola avise cuando se acaba y que alguien la rellene. Barajar ya no podría ser una permutación de todo el contenido (lo exige `playback/queue`), y "A continuación" no podría anunciar el álbum siguiente. Como la biblioteca entera cabe en la cola (techo 4000 en los dos), basta con entregarla completa.

### 7. La cola guarda el álbum

`Queue_Add(path, title, album)` y `Queue_PeekAhead(n, &path, &title, &album)`. La Biblioteca pasa `track->album`, que puede ser `""` si la pista no tiene álbum. Carpetas pasa `NULL`. La diferencia importa:
- `album == NULL`: la pista no viene de la Biblioteca. "A continuación" no va a disco y dibuja el marcador.
- `album != NULL`: se llama a `Cover_Get(album[0] ? album : NULL, path, &budget)`, con la misma regla de clave que usa la Biblioteca (el álbum, o la ruta si está vacío).

Reproduciendo usa un presupuesto de 1 lectura por fotograma: son 2 filas, y la segunda llega un fotograma después.

### 8. Carátula del artista: candidatos al construir y resolución perezosa al dibujar

Elegir "la primera pista que tenga carátula" obliga a preguntarle al disco qué álbumes tienen imagen, y hacerlo para todos los artistas al abrir la pestaña serían cientos de lecturas en un solo fotograma.

- Al construir los nombres de artista, `Library_BuildFieldNames(ARTIST)` arma además, para cada artista, la lista de **candidatos**: una pista por álbum distinto, en el orden de la decisión 5, más una por cada pista sin álbum (que se cachean por ruta). Son índices de pista, como mucho uno por pista del índice.
- Se agrega a `cover.c` la función `Cover_Probe(album, path)`, que lee solo la cabecera de 128 bytes y devuelve si hay imagen, si no la hay o si no hay entrada.
- Al dibujar, cada fila de artista visible que todavía no está resuelta prueba su siguiente candidato con `Cover_Probe`, descontando del mismo presupuesto por fotograma que usan las carátulas. El primero con imagen queda como representante. Si se acaban los candidatos, el artista queda resuelto "sin imagen". El resultado se guarda por nombre hasta que la vista se reconstruye.
- La fila resuelta dibuja con `Cover_Get(album, path, &budget)`, como cualquier otra.

El cubo "Desconocido" no tiene candidatos.

### 9. Ajuste y config v4

`config_t` suma `int group_end` (0 = repetir, 1 = seguir con el siguiente), escrito como una línea nueva `group_end = %d` al final del formato. `CONFIG_VERSION` pasa a 4 con `CONFIG_FIELDS_V4 11`. Un v3 completo toma `group_end = 0` y se reescribe como v4, igual que se hizo con v2 → v3.

Una versión anterior que lea un v4 ve `config_ver >= 3` y suficientes campos, así que lo acepta e ignora la línea extra. Volver atrás no pierde los ajustes.

En `menu_settings.c` se agrega la categoría `STR_SETTINGS_PLAYBACK` al final de la lista, con dos ítems de tipo radio y una pista que muestra el valor vigente. Textos nuevos en `lang_strings.h`, en inglés y en español:
- `STR_SETTINGS_PLAYBACK` ("Playback" / "Reproducción")
- `STR_GROUP_END_TITLE` ("When an album or artist ends" / "Al terminar un álbum o artista")
- `STR_GROUP_END_REPEAT` ("Repeat it" / "Repetirlo")
- `STR_GROUP_END_NEXT` ("Play the next one" / "Seguir con el siguiente")

## Risks / Trade-offs

- **[11 minutos de relectura con 4000 pistas]** → Lo decide el usuario: la biblioteca funciona igual mientras tanto (decisiones 3 y 4), la pasada se abandona con volver y se retoma en el siguiente reescaneo. Hasta que termine, los álbumes que todavía no se releyeron siguen ordenados por título (las pistas sin número van por título).
- **[Volver a una versión anterior descarta el índice v3]** → La versión anterior no reconoce v3 y ofrece reescanear. Se acepta: es la política que esa versión ya tiene, y el índice siempre se puede reconstruir.
- **[Álbumes homónimos de distintos artistas se mezclan]** → La vista Álbumes ya agrupa por nombre de álbum, sin mirar el artista. La cola continua sigue ese mismo agrupamiento, así que dos "Greatest Hits" suenan juntos, como ya se ven juntos. Separarlos es otro cambio.
- **[Más presión sobre los 16 slots de carátulas]** → "A continuación" suma 2 carátulas y la vista Artistas no suma slots (usa las mismas claves de álbum). El LRU ya libera con `UI_GpuFreeTexture`, que sincroniza con la GPU antes de soltar una textura.
- **[La vista visible y la cola comparten `library_view`]** → La decisión 6 fija el orden de los pasos y fuerza la reconstrucción de la vista visible. La tarea 3.2 lo verifica en consola: al volver de Reproduciendo, la lista tiene que ser la del grupo, no la biblioteca entera.
- **[Números de pista con formato raro]** → Por ejemplo "A1" en vinilos, o "1-03". `TrackMeta_ParseNumber` exige que el texto empiece por un dígito, así que esas pistas quedan sin número y van al final del álbum por título. Es predecible y queda cubierto por un test.

## Migration Plan

1. El usuario instala v3.4.0 sobre v3.3.x.
2. `config.cfg` v3 se lee y se reescribe como v4, con `group_end = 0`.
3. El índice v2 se carga con todas las pistas pendientes. La biblioteca se ve igual que antes, y el contador muestra los tags pendientes.
4. Cuando el usuario reescanea, se releen los tags (con números) y el índice se guarda como v3.

Para volver atrás: reinstalar la versión anterior. La configuración se conserva y el índice v3 se descarta, con la oferta de reescanear.

## Estrategia de pruebas

- **Compilación**: `scripts/build.sh` sin errores ni avisos con `-Wall -Werror`, con `track_meta.c` y `library_row.c` agregados a `CMakeLists.txt`.
- **Pruebas en PC** (`make -C tests`, junto a las que ya existen):
  - `test_track_meta`: `TrackMeta_ParseNumber` ("3", "03", "3/12", "", "abc", "/12", NULL), `TrackMeta_Id3v1Track` (v1.0 sin pista y v1.1 con pista), `TrackMeta_CompareDisc` (discos, pistas sin número al final, desempate por título y por ruta) y `TrackMeta_CompareGrouped` (grupo vacío al final, grupos sin distinguir mayúsculas).
  - `test_library_row`: diseños v2 y v3, y una versión desconocida que se rechaza.
- **Pruebas en consola**: los scenarios de las specs delta, paso a paso en `tasks.md`, incluida la actualización desde v3.3.x con un índice v2 real.

## CI

Sin cambios en el pipeline. El CI compila el `.vpk`, que toma los módulos nuevos de `CMakeLists.txt`. No corre `make -C tests`, que sigue siendo una verificación local.

## Código de terceros

No se incorpora código ni assets de terceros. Se usan APIs que ya están en el build: mpg123 (`mpg123_id3v2.text`), opusfile (`opus_tags_query`) y libFLAC/libvorbis (comentarios Vorbis).
