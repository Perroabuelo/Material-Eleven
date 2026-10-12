## Context

Ver `proposal.md` (Why) y las delta specs para el comportamiento. Lo que condiciona el cómo:

- **El diálogo de texto vive dentro de Carpetas.** `source/menus/menu_displayfiles.c` (unas 80–230 líneas) abre el IME del sistema y lo atiende con un lazo propio. Ese lazo tiene tres salidas comprobadas en consola: fallo de composición antes de componer nunca, L+R+START mantenido 90 fotogramas y un tope de 7200 fotogramas. Después drena el aborto. Dibuja de fondo la pantalla de Carpetas y deshabilita el apagado de pantalla mientras gira. Todo eso está acoplado a `Menu_DrawFoldersContent`, `UI_SCREEN_FOLDERS` y `Dirbrowse_*`.
- **La conversión de texto del diálogo está rota fuera del ASCII.** El término inicial se carga byte a byte como unidades UTF-16 y el resultado se recorta con `(char)input[j]`. Una "ó" (U+00F3) vuelve como el byte 0xF3, que no es UTF-8 válido. `Menu_Utf8ToUtf16`, que ya existe para el título, no se usa para el texto.
- **La comparación de Carpetas es ASCII.** `Dirbrowse_NameContains` (`source/dirbrowse.c:161`) usa `strncasecmp`. Los nombres de archivo en la Vita están en UTF-8.
- **Las vistas de la biblioteca son un único arreglo compartido.** `library.c` guarda una vista de pistas (`library_view`, índices al índice global) y una de nombres. Construir otra, como la cola continua, pisa la vista visible. `menu_library.c` trabaja con `selection` como posición dentro de esa vista y reconstruye cuando `view_dirty`. `library.c` incluye cabeceras SCE y no se prueba en PC.
- **Reproducir ya plantea la cola por ruta.** `Menu_LibraryPlaySelected` vuelca la vista entera en la cola y llama a `Menu_PlayQueued(track->path)`, que hace `Queue_SeekToPath`. Para que la cola sea la vista sin filtrar basta con encontrar la pista elegida: el resto ya lo hace.
- **El estado de la biblioteca es estático y sobrevive a salir de la pantalla.** Así se cumple hoy "volver a la misma fila". Lo que se agregue al mismo nivel lo conserva igual.
- `LIBRARY_MAX_TRACKS` es 4000. Ninguna vista, de pistas o de nombres, pasa de ahí.
- Las teclas: en Carpetas △ y □ están libres en la pantalla principal. En Biblioteca, △ reescanea y □ elige carpeta.

## Goals / Non-Goals

**Goals:**
- Una sola implementación del diálogo de búsqueda y de sus salidas, que usan las dos pantallas.
- Una sola regla de comparación (mayúsculas, tildes, ñ) en código puro, probada en PC.
- La lista filtrada de la biblioteca como capa de índices sobre la vista ya construida, sin tocar cómo `library.c` construye ni ordena.

**Non-Goals:**
- Cambiar el orden de ninguna vista o el algoritmo de construcción de `library.c`.
- Plegado Unicode completo (normalización NFD, alfabetos no latinos, ligaduras). Ver Decisión 2.
- Buscar mientras se escribe.

## Decisions

### 1. Módulo compartido del diálogo: `search_dialog.c`

El lazo del IME se mueve sin cambios de comportamiento a `source/search_dialog.c` / `include/search_dialog.h`:

```
SearchDialog_Result SearchDialog_Prompt(const char *title_utf8,
                                        const char *current_utf8,
                                        char *out, size_t out_cap,
                                        void (*draw_background)(void),
                                        UI_Screen rail_screen);
// CONFIRMED (out tiene el término, puede ser "") | CANCELLED | ABANDONED | FAILED
```

- `draw_background` dibuja el cuerpo de la pantalla que llama, sin leyenda ni rail. El módulo pone la leyenda de "Cancelar la búsqueda" y dibuja el rail de `rail_screen` descartando su resultado, como hoy.
- Las constantes `FILTER_*` y el comentario que documenta lo probado en consola sobre el atenuador se mueven tal cual. Son la memoria de un defecto que ya se pagó.
- Quien llama decide qué hacer con el resultado. Carpetas llama a `Dirbrowse_SetFilter` o `Dirbrowse_ClearFilter` solo con `CONFIRMED`. Biblioteca aplica el término a la lista vigente.

*Alternativa descartada:* copiar el lazo en `menu_library.c`. Duplicaría las tres salidas, y la próxima corrección se haría solo en una de las dos copias.

### 2. Comparación pura: `text_fold.c`

`source/text_fold.c` / `include/text_fold.h`, sin SCE ni vita2d:

```
size_t TextFold_Fold(const char *utf8, char *out, size_t cap);   // a forma comparable
int TextFold_Contains(const char *haystack_utf8, const char *folded_needle);
```

El plegado decodifica UTF-8 (con `TextEncoding_Utf8SeqLen` para validar) y lleva cada punto de código a su forma base en minúscula:

- ASCII: `tolower`.
- Latin-1 Supplement y Latin Extended-A (U+00C0–U+017F): una tabla de base sin diacrítico. Á→a, Ü→u, Ñ→n, Ç→c, Ø→o, ß→ss, Æ→ae, Œ→oe.
- Todo lo demás se copia tal cual, incluidos los bytes inválidos. Un nombre con japonés sigue encontrándose por su texto exacto, aunque sin plegar.

`TextFold_Contains` pliega el pajar en un buffer de pila del tamaño de `LIBRARY_PATH_MAX` y busca la aguja ya plegada. El término se pliega una vez al aplicarlo, no por cada entrada.

Trata la ñ como n, como pide la spec: es lo que espera quien escribe sin teclado español, y no hay en la colección un caso donde confundirlas moleste.

*Alternativa descartada:* normalización Unicode completa (utf8proc, ICU). Agrega una dependencia de terceros por un caso que la tabla resuelve para el alfabeto latino, que es el de las colecciones de esta app.

### 3. Conversión UTF-8 ↔ UTF-16 pura

`Menu_Utf8ToUtf16` sale de `menu_displayfiles.c` a `text_encoding.c`, que ya es puro y se prueba en PC, junto a su inversa `Utf16ToUtf8`. Se trabaja con `uint16_t` para no depender de `SceWChar16`. La inversa:

- Convierte los pares sustitutos a una secuencia de 4 bytes.
- Reemplaza un sustituto suelto por U+FFFD.
- Nunca corta una secuencia a la mitad: si no cabe entera, se detiene antes.

El diálogo usa las dos para el título, para el término inicial y para el resultado. Esto corrige el desfigurado de las tildes.

### 4. Lista filtrada en la biblioteca: `list_filter.c`

`source/list_filter.c` / `include/list_filter.h`, puro:

```
typedef int (*ListFilter_Match)(int index, void *ctx);
int  ListFilter_Build(ListFilter *f, int count, ListFilter_Match match, void *ctx);
int  ListFilter_Count(const ListFilter *f);
int  ListFilter_ToFull(const ListFilter *f, int visible);   // -1 si fuera de rango
int  ListFilter_ToVisible(const ListFilter *f, int full);   // -1 si no está
```

- `ListFilter` tiene un arreglo `int visible[LIBRARY_MAX_TRACKS]` (16 KB) y su cuenta. No reserva memoria dinámica, así que no hay camino de fallo.
- `menu_library.c` tiene dos: uno para la lista exterior de la pestaña y otro para la lista de dentro de un grupo. Más dos términos (`char[64]` plegado y original): el original para mostrarlo y recargar el diálogo, el plegado para comparar.
- La función de coincidencia la pone `menu_library.c`, porque sabe qué campos mirar en cada vista (Decisión 5). `list_filter.c` no conoce pistas.
- **Toda la pantalla trabaja en índices visibles.** `selection`, el dibujado, el contador y ↑↓←→ miran `ListFilter_Count` y `ListFilter_ToFull`. Sin término, el filtro es la identidad: se construye con todo, y así no se ramifica el código en dos caminos.
- Se reconstruye cuando se reconstruye la vista (`view_dirty`) y cuando cambia el término. Reconstruir la vista después de una cola continua rehace también el filtro con el mismo término. Así vuelve igual de Reproduciendo.

### 5. Qué compara cada lista

En `menu_library.c`, una función de coincidencia por tipo de lista:

| Lista | Coincide si contiene el término |
|---|---|
| Canciones, Recientes | `Menu_LibraryTrackTitle(t)`, `t->artist` o `t->album` |
| Artistas, Álbumes | `Library_NameAt(i)`, o `Lang_Get(STR_UNKNOWN)` para el cubo |
| Dentro de un grupo | `Menu_LibraryTrackTitle(t)` |

El cubo se compara por la etiqueta del idioma vigente, porque es la que el usuario ve. Cambiar el idioma con un término aplicado puede cambiar si el cubo aparece. Es coherente con lo que se ve.

### 6. Reproducir desde una lista filtrada

`Menu_LibraryPlaySelected` toma `full = ListFilter_ToFull(&f, selection)` y `track = Library_ViewTrack(full)`, y sigue igual: vuelca la vista completa (o la continua) y llama a `Menu_PlayQueued(track->path)`. El filtro no participa de la cola. Ver la spec `library/views`.

### 7. Ciclo de vida de los términos

| Evento | Término exterior | Término del grupo |
|---|---|---|
| Cambiar de pestaña (L/R) | se borra | se borra |
| Entrar en un grupo | se conserva | empieza vacío |
| Volver con término de grupo | – | se borra (no sale) |
| Volver sin término de grupo | – | sale del grupo |
| Volver en la lista exterior con término | se borra | – |
| Reescanear / elegir carpeta | se borra | se borra, y se sale del grupo |
| Nav rail, Reproduciendo | se conserva | se conserva |

`outer_selection` guarda la posición visible de la lista exterior. Al volver de un grupo, como el término exterior no cambió, sigue valiendo.

### 8. Botones, caja y menú de Cuadrado

- **△** abre `SearchDialog_Prompt` en las dos pantallas. En Biblioteca solo en `LIBRARY_STATE_READY`.
- **Caja de búsqueda en la Biblioteca.** Una píldora como la de Carpetas, en la barra superior, a la izquierda de las pestañas. Muestra el término o el texto "Buscar en la biblioteca". La línea de la ruta de la carpeta se recorta hasta el borde izquierdo de la píldora. El ancho final se ajusta en consola: las pestañas en español son las más anchas.
- **□** abre un menú superpuesto, dibujado y atendido dentro del lazo de la biblioteca, sin un lazo propio. Una bandera `square_menu_open` y un índice de opción. Con el menú abierto, `Menu_HandleLibraryControls` atiende solo el menú. El menú se dibuja sobre la lista, anclado abajo a la derecha, encima del mini reproductor. El toque se atiende con `UI_TouchTarget`, como el mini reproductor.
- **Sin resultados.** Cuando `ListFilter_Count` es 0 y hay término, se dibuja un placeholder con "No hay resultados para «término»". La leyenda pone el botón de volver junto a "Quitar filtro" (el `STR_HINT_CLEAR_FILTER` existente).
- **Leyenda de Biblioteca:** △ Buscar, □ Opciones (`STR_HINT_FOLDER` pasa a `STR_HINT_OPTIONS`: "Options" / "Opciones", porque ahora abre un menú y no elige carpeta directamente). Volver muestra "Quitar filtro" cuando hay término en la lista vigente y "Volver" cuando solo se puede salir del grupo. **Leyenda de Carpetas:** suma △ Buscar, y quedan 5 entradas.

Textos nuevos en `lang_strings.h`: `STR_HINT_SEARCH` ("Search" / "Buscar"), `STR_SEARCH_LIBRARY` ("Search library" / "Buscar en la biblioteca"), `STR_NO_RESULTS` con `%s` ("No results for “%s”" / "No hay resultados para «%s»"), `STR_MENU_RESCAN` y `STR_MENU_CHOOSE_FOLDER`. `test_lang` ya comprueba que los dos idiomas lleven los mismos especificadores.

## Risks / Trade-offs

- [Mover el lazo del IME reintroduce el defecto de composición que ya se corrigió] → El movimiento es la primera tarea, sola y sin cambios de comportamiento. Se verifica en consola, con los mismos pasos que la corrección original, antes de que Biblioteca lo use.
- [La caja y las pestañas no caben juntas en la barra superior en español] → Se mide en consola en los dos idiomas. Si no caben, la ruta de la carpeta pasa a la segunda línea, debajo de la caja, que ya es donde está.
- [△ cambia de función en la Biblioteca y quien reescaneaba con △ abre la búsqueda] → Es la decisión del usuario. La leyenda lo anuncia, y la nota de versión lo dice.
- [Con 4000 pistas, filtrar compara tres campos por pista con plegado: hasta 12 000 plegados al confirmar] → Se hace una vez por término o por reconstrucción, no por fotograma. Si en consola se nota, se pliega cada campo solo hasta el largo de la aguja más el campo, o se cachea el plegado al construir la vista.
- [Plegado parcial: un nombre en otro alfabeto con tildes (vietnamita, por ejemplo) no se pliega] → Aceptado. Sigue encontrándose por su texto exacto.
- [`ListFilter` tiene 16 KB estáticos por instancia, dos instancias] → 32 KB de .bss, sin camino de fallo. Es menos que una miniatura en caché (64 KB).

## Recursos

- **Diálogo IME:** `SearchDialog_Prompt` llama a `sceImeDialogInit`. Si falla, devuelve `FAILED` sin nada abierto que cerrar. Si se abrió, el mismo módulo llama a `sceImeDialogTerm` en todas las salidas: confirmar, cancelar o abandono (después de drenar el aborto). El apagado de pantalla se deshabilita al entrar al lazo y se rehabilita antes de cualquier retorno posterior a ese punto.
- **Color de fondo del diálogo:** sigue siendo `static` en el módulo, porque el diálogo conserva el puntero.
- **Buffers de título, texto inicial y texto de entrada:** en la pila de `SearchDialog_Prompt`, que no retorna hasta que el diálogo terminó (`Term`).
- **`ListFilter` y los términos:** almacenamiento estático de `menu_library.c`. Sin reserva ni liberación.
- No se abren archivos ni se crean hilos.

## Estrategia de pruebas

- **Compilación:** `-Wall -Werror` con VitaSDK, en CI.
- **Tests en PC (`tests/`, con ASan y UBSan):**
  - `test_text_fold`: mayúsculas, tildes, diéresis, ñ, ß, UTF-8 inválido o truncado, aguja vacía, aguja más larga que el pajar, coincidencia al final, texto no latino copiado tal cual.
  - `test_text_encoding` (existente, ampliado): ida y vuelta de "acción", pares sustitutos, sustituto suelto, buffer justo y buffer que corta una secuencia.
  - `test_list_filter`: identidad sin término, ninguna coincidencia, `ToFull` y `ToVisible` en los bordes y fuera de rango, orden conservado, `count` igual a 0 y `count` igual a `LIBRARY_MAX_TRACKS`.
- **Consola (pasos en `tasks.md`):** los escenarios de las tres delta specs. En particular:
  - Abrir y cerrar el diálogo en Carpetas y en Biblioteca, incluido L+R+START.
  - Escribir "acción" y volver a abrirlo.
  - Reproducir un resultado único y comprobar la pista siguiente.
  - Volver desde Reproduciendo con un filtro aplicado.
  - El menú de □ con botones y con toque.
  - Medir la barra superior en los dos idiomas.
- Vita3K no sirve para esta verificación: se cae con 3 o más badges de formato y su IME no es el de la consola.

## CI

No requiere cambios. Los tests nuevos se agregan a `BIN` y a sus reglas en `tests/Makefile`, y el job existente ya corre `make -C tests`. Los fuentes nuevos se agregan a `CMakeLists.txt`.

## Terceros

No se incorpora código ni assets de terceros. La tabla de plegado es propia.
