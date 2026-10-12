## Context

Ver proposal.md (Why). Estado actual que condiciona el enfoque:

- `source/touch.c` (de VitaComix) lee solo `report[0]` del panel frontal y expone pressed/held/released como flancos. Reserva su estado con `malloc` en `Touch_Init` y lo libera en `Touch_Shutdown`. No distingue toque de arrastre ni mide velocidad.
- `UI_TouchTarget` (`source/ui_theme.c`) dispara en el flanco de apoyo (`Touch_CheckPressed`). Así funcionan el nav rail, el mini reproductor, el transporte de Reproduciendo y la caja de filtro de Carpetas.
- No hay desplazamiento independiente: la Biblioteca dibuja páginas fijas (`first = selection - selection % ROWS_PER_PAGE`, `menu_library.c`), Carpetas una ventana deslizante atada a `position` (`Dirbrowse_DisplayFiles`), y Ajustes dibuja todas sus filas desde `HEADER_H`.
- Ya existe un arrastre propio: las letras de Reproduciendo (`Menu_LyricsHandleDrag`), que solo arrastra si el toque empezó sobre ellas. Es el antecedente del criterio "el toque es de quien lo empezó".
- `UI_DrawTextClipped` activa y desactiva el recorte de vita2d por su cuenta (`vita2d_disable_clipping` al terminar). Un recorte exterior para la lista se perdería después del primer texto recortado.
- Este cambio va después de `key-repeat` (módulo `key_repeat`, helper `Utils_ListStep(&sel, count, page)` y capability `ui/list-navigation`) y `library-search` (búsqueda con △, caja tocable, menú de □ en la Biblioteca).

## Goals / Non-Goals

**Goals:**
- Un único modelo de desplazamiento y de gestos, compartido por Biblioteca, Carpetas y Ajustes, en código puro probado en PC.
- Que las filas se activen al soltar y que un arrastre nunca active nada, sin cambiar cómo responden hoy los botones del rail y del mini reproductor.

**Non-Goals:**
- Multitáctil, panel trasero, gestos fuera de los de la propuesta.
- Animaciones de rebote o sobre-desplazamiento elástico en los extremos.
- Reescribir el arrastre de letras de Reproduciendo para usar el módulo nuevo (puede hacerse después; no es necesario).

## Decisions

### D1. Dos módulos puros: `gesture` y `list_scroll`
- `gesture` (`source/gesture.c`, `include/gesture.h`): recibe por fotograma `{down, x, y, dt_ms}` y devuelve un evento: `GESTURE_NONE`, `GESTURE_PRESS` (apoyo, para resaltar), `GESTURE_DRAG` (con `dy` del fotograma), `GESTURE_TAP` (al soltar sin haber pasado el umbral, con la posición de apoyo) o `GESTURE_RELEASE` (al soltar un arrastre, con la velocidad en px/ms). El umbral de arrastre es de 16 px en coordenadas de pantalla (unos 2 mm en la Vita). La velocidad de salida es el promedio de los últimos 4 fotogramas de movimiento, para que un fotograma suelto con ruido no la dispare.
- `list_scroll` (`source/list_scroll.c`, `include/list_scroll.h`): estado `{offset, velocity, content_h, viewport_h, row_h}`. Operaciones: arrastrar (`offset -= dy`, acotado a `[0, max(0, content_h - viewport_h)]`), soltar con velocidad, avanzar un fotograma de inercia con `dt_ms`, detener, `EnsureVisible(index)` y `RowAt(y)` (índice de fila bajo una y relativa al área, o -1).
- Ninguno incluye vita2d ni cabeceras SCE: la pantalla traduce `touch.c` a la entrada y dibuja con el resultado. Mismo patrón que `seek_bar`.
- **Alternativa descartada**: un solo módulo que mezcle gesto y lista. Separados, el reconocedor sirve también para controles que no son listas (pestañas, menú de □) y cada test es más corto.

### D2. Inercia con decaimiento exponencial en función del tiempo
Cada fotograma: `velocity *= pow(FRICTION, dt_ms / 16.67)` con `FRICTION = 0.95`, se detiene por debajo de 0.02 px/ms o al tocar un extremo (velocidad a 0, sin rebote). Una salida por debajo de 0.3 px/ms no inicia inercia. Usar `dt_ms` (de `sceKernelGetProcessTimeWide`, medido por la pantalla) y no "un fotograma" hace que un fotograma lento no frene la lista de golpe. Los valores son constantes en `list_scroll.c` y se afinan en consola (ver Open Questions); los tests comprueban propiedades (monótono, termina, no sale del rango), no números exactos.
- **Alternativa descartada**: sin inercia. Con 2000 canciones obliga a decenas de arrastres.

### D3. El toque es de quien lo empezó
Al apoyar el dedo, la pantalla decide el dueño por la zona del apoyo, en este orden: nav rail (`x < UI_RAIL_WIDTH`), mini reproductor (`y >= MiniPlayer_Top()` con pista cargada), barra superior (`y < TOPBAR_H`/`HEADER_H`), superposiciones (menú de □, aviso), y por último el área de la lista. Solo si el dueño es la lista se alimenta `gesture`; si no, la lista ignora ese toque hasta que se levante el dedo. El rail, el mini reproductor y la barra superior siguen con `UI_TouchTarget` en el flanco de apoyo: como un arrastre que empieza en la lista no genera un flanco de apoyo nuevo al pasar sobre ellos, no los dispara (escenario "Arrastrar sobre el mini reproductor no lo acciona") sin tocar ese código.
- **Alternativa descartada**: pasar todos los controles a "activar al soltar". Cambia el tacto de lo que ya funciona y amplía el cambio sin necesidad.

### D4. Ventana de dibujo por desplazamiento, con recorte anidable
Las listas (Biblioteca, Carpetas, elegir carpeta y opciones de Ajustes) dibujan desde `first = offset / row_h` hasta cubrir el área, con `y = area_top + i * row_h - offset`. Para que las filas parciales no pisen la barra superior, el contador de la Biblioteca, la leyenda o el mini reproductor, se recorta al área de la lista. Como `UI_DrawTextClipped` hoy apaga el recorte, se agrega en `ui_theme.c` una pila de recorte (`UI_PushClip`/`UI_PopClip`) que intersecta con el recorte vigente y lo restaura al salir; `UI_DrawTextClipped` pasa a usarla. El área de la lista termina en `min(MiniPlayer_Top(), parte superior del contador o de la leyenda)`.
- **Alternativa descartada**: dibujar la lista primero y tapar con la barra y el mini reproductor. El contador de la Biblioteca y la franja sobre la leyenda no tienen fondo propio, y quedarían filas a medias visibles.
- El presupuesto de carátulas por fotograma (`COVER_BUDGET_PER_FRAME`) se mantiene: con desplazamiento libre se ven a lo sumo 6 filas, una más que hoy.

### D5. Selección y vista: la vista sigue a la selección solo cuando la mueve un botón
`EnsureVisible(selection)` se llama cuando `Utils_ListStep` (flanco o repetición de `key-repeat`) o ←→ cambian la selección, al entrar o salir de un grupo, al cambiar de vista y al volver a la pantalla. No se llama cada fotograma, así que un arrastre puede dejar la selección fuera de la vista. Cualquier botón de dirección detiene la inercia antes de mover la selección. Una fila a medias visible cuenta como no visible y se alinea al borde por el que entra. Volver de Reproduciendo restaura la selección (ya lo hace `nav-shell`) y llama a `EnsureVisible`.

### D6. Toque, resaltado y activación
- `GESTURE_PRESS`: la fila bajo el dedo (`RowAt`) se dibuja con un resaltado de toque (el mismo lavado de acento de la selección, más tenue) mientras no haya arrastre. La selección no cambia todavía.
- `GESTURE_TAP`: si la fila del apoyo sigue existiendo y el apoyo no detuvo una inercia, `selection = fila` y se llama a la misma función que Confirmar (`Menu_LibraryEnter`/`Menu_LibraryPlaySelected`, `Dirbrowse_OpenFile` o `Dirbrowse_Navigate`, `item_activate` de Ajustes).
- Un apoyo que encuentra la lista en movimiento la detiene y marca el gesto como "atrapado": su `TAP` se descarta.
- Un `TAP` cuya posición final cae fuera del área de la lista se descarta (el reconocedor informa la posición de apoyo y la de salida).

### D7. Ajustes
La lista de categorías cabe entera: usa `gesture` + `RowAt` sin desplazamiento (`content_h <= viewport_h`, `list_scroll` lo trata como lista fija). Tocar una categoría la selecciona y pone `item_index = 0`, igual que L/R. La lista de opciones usa `list_scroll` completo, que solo se desplaza si una categoría tiene más opciones de las que caben.

### D8. Estado de `touch.c`
Se reemplaza el `malloc` de `touchState` por una variable estática; `Touch_Init` solo activa el muestreo y `Touch_Shutdown` queda vacío (se conserva por simetría con `main.c`). Así el cambio no agrega ni deja ninguna reserva. `gesture` y `list_scroll` usan estructuras estáticas por pantalla, sin memoria dinámica, sin archivos ni hilos. Al cambiar de pantalla o de vista, la pantalla llama a `Gesture_Reset` y `ListScroll_Stop`, para que un dedo apoyado o una inercia no sigan en la pantalla siguiente.

### Estrategia de pruebas
- **Compilación**: `.vpk` limpio con `-Wall -Werror`, en CI.
- **Tests en PC** (`tests/test_gesture.c`, `tests/test_list_scroll.c`, con ASan/UBSan):
  - toque bajo el umbral → `TAP` en la posición de apoyo; sobre el umbral → `DRAG` y nunca `TAP`; levantar fuera informa la salida;
  - velocidad de salida promediada, ruido de un fotograma no inicia inercia;
  - arrastre acotado a los extremos; lista más corta que el área no se mueve;
  - inercia monótona decreciente, termina en un número finito de pasos, se detiene en el extremo, `dt_ms` grande no la saca del rango;
  - `EnsureVisible` para fila arriba, abajo, ya visible y a medias; `RowAt` en bordes de fila, fuera del área y con lista vacía.
- **Consola** (Vita3K no sirve: se cae con 3 o más badges de formato y su emulación táctil es limitada). Con una biblioteca de al menos 200 pistas y una carpeta de al menos 20 entradas, recorrer en orden cada escenario de los requisitos que este cambio agrega en `specs/ui/list-navigation/spec.md`. Además, con el overlay de debug abierto, anotar la pila y el HEAP en Carpetas, recorrer veinte veces Biblioteca → arrastre largo con inercia → toque en un álbum → volver → Carpetas → arrastre → Ajustes, y comprobar que pila y HEAP son los mismos al volver a Carpetas.

### CI
Ninguno en el workflow: `build.yml` ya corre `make -C tests`. Solo se agregan los dos tests a `BIN` y sus reglas en `tests/Makefile`, y los fuentes nuevos a `CMakeLists.txt`.

### Terceros
No se incorpora código ni assets de terceros.

## Risks / Trade-offs

- [El ruido del panel hace que un toque quieto supere el umbral y no active] → umbral de 16 px; si en consola falla, se sube (constante única en `gesture.c`).
- [La pila de recorte rompe el dibujo de otro texto recortado] → `UI_DrawTextClipped` se migra en el mismo commit que la pila y se verifica en consola en las cuatro pantallas, en los dos idiomas.
- [Reescribir el dibujo de las tres listas trae regresiones en la selección por botones] → `EnsureVisible` está cubierto en PC, y la verificación en consola recorre los escenarios de botones de `nav-shell` y `library/views` (volver a la misma fila) además de los nuevos.
- [Inercia sentida distinta a la de un teléfono] → constantes afinables en consola, sin cambiar specs.
- [Más filas visibles por fotograma (6 en vez de 5) y desplazamiento rápido piden más carátulas] → se mantiene el presupuesto por fotograma; la lista no espera carátulas (requisito existente de `library/views`).

## Migration Plan

Sin datos ni configuración nuevos. Se publica como v3.9.0 después de v3.7.0 (`key-repeat`) y v3.8.0 (`library-search`). Si algo falla en consola, se revierte el PR: no hay estado persistente que migrar.

## Open Questions

- Valores finales de umbral de arrastre, fricción y velocidad mínima de inercia: se afinan en la consola durante la verificación, sin cambiar specs ni tareas.
