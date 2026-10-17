## Context

La motivación está en `proposal.md` (Why) y el comportamiento en las specs: `library/multi-select` (nueva) y los deltas de `library/views`, `library/playlists` y `ui/list-navigation`. Lo que condiciona el cómo:

- **Toda lista de la biblioteca es una lista completa con un filtro encima.** `menu_library.c` arma la vista completa y un `ListFilter` (`list_filter.c`, puro) dice qué posiciones completas se ven. `selection` es una posición visible y `Menu_LibraryFull` la traduce. La lista completa más larga es `LIST_FILTER_MAX` = `LIBRARY_MAX_TRACKS` = 4000.
- **La lista completa no cambia mientras el usuario no reescanee ni edite.** Cambiar el término solo rehace el filtro. Esto permite guardar las marcas como posiciones completas.
- **Añadir hoy es de una fila.** `Menu_LibraryCollect` arma un arreglo de `Playlist_Entry` con la canción, o con las pistas del grupo vía `Library_BuildFieldTracks`, que reescribe el arreglo de pistas de la vista. `Playlist_Append` reserva lugar para todo de una vez (si falla, la playlist no cambia) y busca duplicados recorriendo la playlist entrada por entrada.
- **Un `Playlist_Entry` ocupa 448 bytes.** 4000 entradas son 1,75 MB. Hoy una playlist llena ya ocupa eso en RAM; un arreglo temporal del mismo tamaño encima la duplicaría.
- **`path_index.c` ya existe y es puro:** ordena posiciones por ruta y busca por mitades.
- **El reconocedor de gestos es puro y por fotograma.** `Gesture_Update` recibe `dt_ms`, distingue toque y arrastre, y `caught` convierte el levantar en `RELEASE` sin toque. `Gesture_Reset` pone todo el estado a cero. `TouchList` lo envuelve y lo comparten Biblioteca, Carpetas, elegir carpeta y Ajustes.
- **Los modos de la biblioteca son estados del mismo lazo:** `options_open`, `picker_open`, `confirm_open` y `moving`. Cada uno toma los botones y define su leyenda. El modo selección sigue el mismo patrón.

## Goals / Non-Goals

**Goals:**
- Las marcas, el estado de la casilla Todo, el recorrido con la casilla, el toque mantenido, añadir en bloque y quitar varias como código puro con tests en PC.
- Añadir 4000 canciones sin un arreglo temporal de 1,75 MB y sin comparar cada ruta con toda la playlist.
- Carpetas, elegir carpeta y Ajustes se comportan exactamente igual que antes.

**Non-Goals:**
- Un modo selección genérico para otras pantallas. Vive en la biblioteca.
- Cambiar el formato de las playlists, la cola o el índice.

## Decisions

### 1. Módulo puro `selection.c`: un bitmap fijo sobre posiciones completas

```
 selection.c (puro, tests/test_selection.c)
   Selection { uint8_t bits[LIST_FILTER_MAX / 8]; int count; }
   Selection_Clear(s)
   Selection_Toggle(s, full) / Selection_Set(s, full, on) / Selection_Has(s, full)
   Selection_Count(s)
   Selection_AllState(s, const ListFilter *f)  -> NONE | SOME | ALL | DISABLED
   Selection_ToggleAll(s, const ListFilter *f)  ALL -> desmarca lo visible; si no, lo marca
   Selection_Next(s, from)                      la siguiente marcada >= from, o -1
   Selection_Step(pos, count, step)             recorrido con la casilla (Decisión 4)
```

Las marcas son posiciones de la **lista completa**, así que la búsqueda no las toca y lo oculto sigue contando. `count` se mantiene al marcar y desmarcar, sin recorrer el bitmap. `Selection_Next` recorre en orden de la lista completa, que es el orden de lo añadido.

Son 500 bytes, así que el bitmap es estático en `menu_library.c`. No hay memoria dinámica que liberar: salir del modo es `Selection_Clear`.

*Alternativas descartadas:*
- Una lista de posiciones en el orden en que se marcaron. Habría que ordenarla para añadir, y marcar y desmarcar serían búsquedas lineales.
- Reservar el bitmap al entrar al modo, como se pensó al explorarlo. 500 bytes no justifican un camino de error.

### 2. Toque mantenido en `gesture.c`, activado por quien lo pide

`Gesture_Input` gana `hold_ms`, el umbral que la pantalla pide para este dedo, y 0 lo desactiva. `Gesture_State` acumula `held_ms` mientras el dedo está apoyado, sin arrastrar y sin `caught`. Al llegar al umbral, `Gesture_Update` informa un único `GESTURE_HOLD` con la posición de inicio y marca `caught`, de modo que levantar da `RELEASE` y no `TAP`. Arrastrar después sigue siendo `DRAG`, porque `caught` no impide arrastrar.

El umbral va en la entrada y no en el estado porque `Gesture_Reset` borra el estado entero y lo llaman varios caminos. `GESTURE_HOLD_MS` vale 500, dentro de los 400 a 700 ms de la spec.

`TouchList` gana un campo `hold_ms`, que vale 0 por defecto, y `TouchList_HeldRow(list)`, que devuelve la fila mantenida en este fotograma o -1, igual que `TouchList_PressedRow`. Solo la biblioteca pone `hold_ms`, así que Carpetas, elegir carpeta y Ajustes no cambian. En la pestaña Playlists la biblioteca también lo pide, para que levantar después de mantener no active la fila, y luego ignora la fila mantenida.

*Alternativa descartada:* medir el tiempo en `touch_list.c` con el reloj de la consola. Quedaría fuera del test en PC.

### 3. Estado del modo en `menu_library.c`

```
 static SceBool selecting;        // el modo
 static Selection marks;          // Decisión 1
 static SceBool header_focus;     // la posición actual es la casilla Todo
```

`header_focus` es un booleano aparte, y no `selection = -1`, porque todo el código actual supone `selection >= 0`. Mientras `header_focus` es verdadero, `selection` conserva la última fila, que es adonde vuelve abajo.

**Una sola salida del modo:** `Menu_LibraryEndSelect()` hace `selecting = false`, `header_focus = false` y `Selection_Clear`. La llaman:
- el botón de volver sin término;
- el control de salir del encabezado;
- una acción terminada (añadir o quitar);
- `Menu_LibraryExit`, en cualquier cambio de pantalla;
- `Menu_LibraryFreeOpenPlaylist`, si guardar falló y la playlist abierta se suelta.

Cambiar de vista, entrar o salir de un grupo, reescanear y elegir carpeta no son alcanzables en el modo, porque L, R, confirmar y el menú están reemplazados. Aun así, `Menu_LibrarySetView`, `Menu_LibraryEnter`, `Menu_LibraryLeave` y `Menu_LibraryRescan` también llaman a `EndSelect`, para que una marca nunca apunte a otra lista.

### 4. Recorrido con la casilla Todo

`Selection_Step(pos, count, step)` trabaja sobre `count + 1` posiciones, donde 0 es la casilla y `1..count` son las filas visibles. Reusa la regla de `KeyRepeat_Move`: una pulsación nueva da la vuelta y los pasos repetidos se detienen en el borde. La diferencia es que los pasos repetidos se detienen en `1` y `count`, sin entrar en la casilla. Con `count == 0` la casilla está deshabilitada y la posición no cambia. Al ser pura, la cubre `test_selection`.

### 5. Encabezado

Mientras `selecting`, `Menu_DrawLibraryTopBar` dibuja, en el mismo alto de `TOPBAR_H`:

```
 +----------------------------------------------------------------+
 | [x]  12 seleccionadas            [ buscar...    ]   Todo [-]   |
 +----------------------------------------------------------------+
   salir                              caja de búsqueda   casilla
```

- Las pestañas no se dibujan ni se tocan.
- La caja de búsqueda queda en su lugar, así que el término aplicado se sigue viendo y tocarla abre la búsqueda.
- La casilla se dibuja resaltada cuando `header_focus` es verdadero.
- Cada control tiene un área activa de al menos el mínimo táctil de `ui/rendering`.

El contador usa un texto por tipo de fila, para concordar en género y número: "%d seleccionadas" para canciones y "%d seleccionados" para artistas y álbumes. En inglés, "%d selected".

### 6. Añadir en bloque: `Playlist_AppendFrom` con una fuente y un índice por ruta

```
 typedef int (*Playlist_Source)(int i, Playlist_Entry *out, void *ctx); // 1 si dio la entrada i
 Playlist_Result Playlist_AppendFrom(Playlist *pl, int n, Playlist_Source next, void *ctx,
                                     int *added, int *skipped, int *full);
```

1. Reserva de una vez lugar para `count + n`, recortado a 4000. Si falla, devuelve `PLAYLIST_NOMEM` sin cambios.
2. Arma un `PathIndex` sobre las entradas que la playlist ya tenía. Si falla, libera lo reservado de más y devuelve `PLAYLIST_NOMEM` sin cambios.
3. Pide las `n` entradas de a una, en un único `Playlist_Entry` local, y añade las que no están en el índice. Se detiene si la fuente devuelve 0.
4. Libera el índice.

Así se evitan el arreglo temporal y la búsqueda lineal: comparar 4000 contra 4000 son 16 millones de comparaciones, y con el índice son unas 4000 × 12.

Dentro de una misma tanda no se buscan duplicados, porque en una vista de la biblioteca cada pista aparece una vez y los grupos de Artistas y de Álbumes no comparten pistas. El contrato de `AppendFrom` lo dice. `Playlist_Append` se mantiene para la fila única y para su test.

**La fuente en `menu_library.c`** recorre las marcas con `Selection_Next`:
- En las listas de canciones, cada marca es una entrada.
- En Artistas y Álbumes, cada marca es un grupo: llama a `Library_BuildFieldTracks` y entrega sus pistas en orden.

Como `AppendFrom` necesita `n` antes de empezar, la fuente de grupos hace dos pasadas: una para sumar los tamaños y otra para entregar. Construir el grupo es recorrer el índice, barato al lado de guardar el archivo. Al terminar, `view_dirty` rehace la vista de nombres, igual que hoy con un solo grupo.

El selector y "Nueva playlist…" siguen el camino actual (`AddToExisting`/`AddToNew`), con la fuente en lugar del arreglo. El aviso es `Menu_LibraryNoticeAdded`, sin cambios. Un fallo de memoria en `AppendFrom` no llega a guardar: el aviso es el de no se pudo guardar y el archivo no cambia.

### 7. Quitar varias: `Playlist_RemoveIf`

```
 int Playlist_RemoveIf(Playlist *pl, int (*doomed)(int index, void *ctx), void *ctx, int *first);
```

Compacta en una pasada, conservando el orden. Devuelve cuántas quitó, y en `*first` la posición de la primera quitada (o -1). `playlist.c` no conoce `Selection`: el predicado lo adapta. Se guarda con un solo `PlaylistStore_Save`, que ya escribe a un temporal y renombra, así que un corte deja la versión anterior o la nueva.

Después se sale del modo, se rehace el filtro con el término vigente y la fila actual pasa a ser la visible que ocupa la posición completa `first`. Si esa no se ve o no existe, pasa a ser la última visible.

### 8. Diálogo de confirmación con tipos

El diálogo de borrar playlist (`confirm_open`, `confirm_index`) pasa a tener un tipo: `CONFIRM_DELETE_PLAYLIST` o `CONFIRM_REMOVE_MARKED`. Cada tipo define su título y su botón de acción ("Borrar" o "Quitar"). Los dos abren con cancelar marcado y comparten la geometría, el toque y la leyenda. El título de quitar es "¿Quitar %d canciones de “%s”?".

### 9. Menú de Cuadrado

- `OPTION_SELECT` se agrega después de las acciones de fila, donde la spec de `library/views` lo pide.
- `OPTIONS_MAX` pasa de 4 a 5, el caso de dentro de una playlist. El recuadro mide 5 × 56 + 16 = 296 px. Su borde superior queda en `MiniPlayer_Top() - 304`, por debajo de `TOPBAR_H`.
- En el modo, `Menu_LibraryOpenOptions` arma una sola opción. Si `Selection_Count` es 0, la marca como deshabilitada: se dibuja con `UI_COLOR_TEXT_SECONDARY` y confirmar o tocarla no hace nada.

### 10. Botones del modo y leyenda

`Menu_HandleLibrarySelect`, llamado en lugar de `Menu_HandleLibraryControls` mientras `selecting`, atiende:
- arriba y abajo con `Selection_Step`;
- confirmar: `Toggle` sobre la fila, o `ToggleAll` sobre la casilla;
- volver: quitar el término, o `EndSelect`;
- Triángulo (busca) y Cuadrado (menú).

No atiende L, R ni SELECT. El chequeo de SELECT en `Menu_DisplayLibrary` se salta mientras `selecting`.

La leyenda tiene un cuarto juego de entradas, como el de mover:
- confirmar: "Marcar";
- volver: "Salir", o "Quitar búsqueda" con término;
- Triángulo: "Buscar";
- Cuadrado: "Acciones";
- START: apagar la pantalla, como en las demás.

En cada fila la casilla se dibuja a la izquierda de la carátula, y la fila se corre el ancho de la casilla. El badge y el texto se recortan igual que hoy.

### 11. Textos

Se agregan a `lang_strings.h`, en inglés y en español:
- la opción "Seleccionar";
- los contadores por tipo y "Todo";
- las leyendas "Marcar", "Salir" y "Acciones";
- el título y el botón "Quitar" del diálogo;
- el aviso "Se quitaron %d de “%s”".

`test_lang` ya comprueba que cada texto tenga sus dos idiomas.

## Recursos

| Recurso | Quién lo reserva | Quién lo libera y cuándo | Si algo falla a mitad |
|---|---|---|---|
| `marks` (500 B) | Estático | Nadie: `Selection_Clear` al salir del modo | No reserva |
| Crecimiento de `pl->entries` en `AppendFrom` | `AppendFrom`, una vez al empezar | `Playlist_Free`, por el llamador como hoy (`AddToExisting`/`AddToNew`) | Si la reserva falla, nada cambia |
| `PathIndex.order` (4 B × entradas previas) | `AppendFrom` | `AppendFrom`, antes de volver, en todos los caminos | Si falla, devuelve `NOMEM` y no añade nada |
| Arreglo de pistas de grupo (`Library_BuildFieldTracks`) | `library.c`, como hoy | `library.c`, al rehacer la vista | Sin cambios respecto de hoy |
| `open_playlist` tras `RemoveIf` | Sin reserva nueva: compacta en el lugar | `Menu_LibraryFreeOpenPlaylist`, como hoy | Si `Save` falla, el aviso de hoy suelta la playlist abierta y sale del modo |

## Estrategia de pruebas

**Compilación:** `scripts/build.sh` con `-Wall -Werror`.

**Tests en PC** (`make -C tests`, con ASan y UBSan, también en CI):
- `test_selection` (nuevo):
  - marcar, desmarcar y contar, incluidas las posiciones 0 y 3999 y las que están fuera de rango;
  - `AllState` con filtro vacío, ninguna, algunas y todas visibles marcadas, y con marcas ocultas;
  - `ToggleAll` que no toca lo oculto (la secuencia de "queen" y "abba");
  - `Next` en orden;
  - `Step` con pulsación nueva y repetida en los dos bordes, la vuelta por la casilla y `count == 0`.
- `test_gesture` (ampliado):
  - mantener por encima del umbral informa un único `HOLD` y levantar da `RELEASE`;
  - levantar antes del umbral da `TAP`;
  - arrastrar antes del umbral da `DRAG` sin `HOLD`;
  - con `hold_ms = 0` nunca hay `HOLD`;
  - un toque `caught` no da `HOLD`;
  - arrastrar después del `HOLD` da `DRAG`.
- `test_playlist` (ampliado):
  - `AppendFrom` en orden, con omitidas, con 3998 + 5 que llena y con la fuente que corta antes;
  - `AppendFrom` con fallo inyectado en la reserva y en el índice, que deja la playlist igual;
  - `RemoveIf` sobre ninguna, la primera, alternadas, la última y todas, con `*first` correcto.

**Consola** (Vita, con la biblioteca de prueba armada con las canciones reales del usuario):
1. Mantener el dedo sobre una canción entra al modo con ella marcada, y levantar no reproduce. Mantener el dedo en Carpetas sigue entrando en la carpeta.
2. Seguir los escenarios de "queen" y "abba" (buscar, marcar, buscar, marcar) y añadir a una playlist nueva. Revisar el orden y el aviso.
3. Marcar tres álbumes, añadirlos y revisar el orden de las pistas.
4. Dentro de una playlist, marcar con la casilla Todo, quitar con confirmación, cancelar una vez y confirmar otra, y reiniciar la aplicación.
5. Recorrer arriba y abajo por la casilla, mantener arriba sin pasar por ella, y probar que L, R y SELECT no hacen nada. El nav rail descarta la selección.
6. **Memoria:** con el overlay de debug, anotar el HEAP; repetir 10 veces marcar todo en Canciones, añadir a una playlist y borrarla; volver a anotar el HEAP. Debe quedar igual.
7. **Tiempo:** medir cuánto tarda marcar todo en Canciones y añadirlo a una playlist que ya tiene la mitad de esas canciones.

## Cambios en CI

Ninguno en el workflow. `tests/Makefile` suma `test_selection` a `BIN`, y el CI ya corre `make -C tests`. `selection.c` se agrega a `CMakeLists.txt`.

## Código de terceros

No se incorpora código ni assets de terceros. La casilla se dibuja con primitivas de vita2d, como el resto de los controles.

## Risks / Trade-offs

- [Un toque mantenido de 500 ms entra al modo cuando el usuario solo quería leer con el dedo apoyado] → Entrar al modo es inofensivo: no reproduce, y volver sale sin cambios. El umbral es una constante, fácil de ajustar tras probar en la consola.
- [Guardar una playlist de 4000 entradas tarda en la tarjeta] → Es una sola escritura, igual que guardar hoy una playlist llena. El paso 7 de consola lo mide. Si pasa de un segundo, se muestra el aviso de guardado antes de escribir, en otro cambio.
- [La fuente de grupos construye cada grupo dos veces] → Es el mismo costo que entrar dos veces en el álbum, y no se nota frente a la escritura. El paso 7 lo cubre.
- [La casilla en cada fila resta ancho al título] → El ancho de la casilla sale del margen de la carátula, y el recorte de título antes del badge ya existe.
- [Marcas que apuntan a otra lista si algún camino nuevo rehace la vista sin pasar por `EndSelect`] → Las cuatro funciones que cambian de lista llaman a `EndSelect`, aunque no sean alcanzables en el modo.
