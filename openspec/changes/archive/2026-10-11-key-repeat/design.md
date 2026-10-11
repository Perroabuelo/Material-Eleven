## Context

Todas las pantallas leen el pad en `Utils_ReadControls` (`source/utils.c`), una vez por fotograma. Esa función deja en la global `pressed` solo el flanco (`pad.buttons & ~old_pad.buttons`), lo pasa por `ScreenOff_Filter` y expone los botones mantenidos con `Utils_HeldButtons()`. No existe ninguna noción de repetición.

Las listas mueven la selección con el mismo patrón repetido en cuatro sitios:

```
if (pressed & SCE_CTRL_UP) sel--; else if (pressed & SCE_CTRL_DOWN) sel++;
Utils_SetMax(&sel, 0, count - 1);      // pasar del final -> primera
Utils_SetMin(&sel, count - 1, 0);      // pasar del inicio -> última
```

- `menu_library.c`, `Menu_HandleLibraryControls` (página de 5 filas, `ROWS_PER_PAGE`).
- `menu_displayfiles.c`, `Menu_HandleControls` (página de `FILES_PER_PAGE` = 5).
- `menu_displayfiles.c`, el bucle de elegir carpeta de la biblioteca (misma lista, mismo tamaño de página).
- `menu_settings.c`, los ítems de la categoría vigente (todos visibles a la vez).

El bucle de cada pantalla va sincronizado con el vsync (~60 fps), pero un fotograma puede alargarse cuando se decodifica una carátula o se llena el atlas de texto.

Motivación y alcance: ver proposal.md. Comportamiento exigido: ver `specs/ui/list-navigation/spec.md`.

## Goals / Non-Goals

**Goals:**
- Un único sitio que decide cuándo hay un paso repetido, en código puro probado en PC.
- Un único helper que aplica un paso a una selección, para que las cuatro listas no repitan la lógica de bordes.
- Que `pressed` siga significando exactamente lo mismo para todo el resto del código.

**Non-Goals:**
- Repetir botones distintos de ↑/↓.
- Tocar cómo se dibujan las listas o cómo se calcula la ventana visible (eso es del cambio `touch-navigation`).

## Decisions

### 1. Máscara aparte, no repetición dentro de `pressed`

`Utils_ReadControls` sigue calculando `pressed` igual que hoy y, además, calcula los pasos repetidos de ↑/↓ en un estado propio. Solo las listas los consumen.

- Alternativa descartada: inyectar los pasos repetidos en `pressed`. Rompería Reproduciendo (↑ alterna letras), el combo de escape del IME y cualquier pantalla futura que asuma flancos. Habría que filtrar en cada sitio que no debe repetir, que son la mayoría.

### 2. Módulo puro `key_repeat` basado en tiempo

`include/key_repeat.h` / `source/key_repeat.c`, sin vita2d ni SCE (mismo estilo que `screen_off`). Recibe por fotograma la máscara de mantenidos, el flanco y la hora actual en microsegundos; quien lee el reloj (`sceKernelGetProcessTimeWide`) es `utils.c`.

```
                 pulsa (flanco)                suelta / cambia de dirección
   IDLE ---------------------------> HELD -----------------------------> IDLE
                                      |
          t >= inicio + 400 ms        |  paso de 1 fila cada 66 ms
          t >= inicio + 2000 ms       |  paso de 1 página cada 66 ms
```

Constantes (en el header, como `#define`): retardo 400 ms, intervalo 66 ms (~15 pasos/s), paso a páginas a los 2000 ms.

- A lo sumo un paso por fotograma: si un fotograma se alarga, no se dispara una ráfaga para "ponerse al día"; el siguiente paso se reprograma desde ahora. Así un tirón al decodificar una carátula no hace saltar la selección varias filas de golpe.
- Si ↑ y ↓ están mantenidos a la vez, no hay repetición. Cambiar de dirección reinicia el retardo.
- El estado vuelve a IDLE cuando no hay ninguna de las dos mantenida.
- Alternativa descartada: contar fotogramas, como hacen `FILTER_ABORT_FRAMES` y similares. Es más simple, pero con fotogramas lentos la repetición se notaría desigual; la hora es igual de fácil de probar en PC pasando valores inventados.

Salida: `KeyRepeat_Update(...)` devuelve un `KeyRepeat_Step` con la dirección (-1, 0, +1), si es una pulsación nueva o un paso repetido, y si el paso es de página.

### 3. Helper puro para mover la selección

En el mismo módulo, `KeyRepeat_Move(int sel, int count, KeyRepeat_Step step, int page)` devuelve la nueva selección:

- Pulsación nueva: ±1, y da la vuelta en los bordes (lo que hoy hacen `Utils_SetMax`/`Utils_SetMin`).
- Paso repetido: ±1 o ±`page`, y se recorta a `[0, count-1]` sin dar la vuelta.
- `count <= 0`: devuelve 0.

`utils.c` expone `Utils_ListStep(int *sel, int count, int page)`, que aplica el paso del fotograma con ese helper. Las cuatro listas cambian sus tres líneas por esa llamada. Ajustes pasa como página su número de ítems (los muestra todos), así que un salto de página la lleva al borde, que es lo que una página significa ahí.

- Alternativa descartada: exportar solo la máscara y que cada pantalla haga la cuenta. Volvería a duplicar la lógica de bordes en cuatro sitios.

### 4. El filtro de START sigue antes

`ScreenOff_Filter` sigue aplicándose al flanco. El estado de repetición se alimenta con los mantenidos sin START, así que apagar la pantalla no interfiere.

## Risks / Trade-offs

- [El ritmo no se siente bien en la consola] → Las tres constantes están juntas en el header; se ajustan en la verificación en consola sin cambiar la lógica ni los tests (los tests usan las constantes, no números literales).
- [Un diálogo del sistema (IME) abierto mientras se mantiene ↓] → El IME también pasa por `Utils_ReadControls`, pero no consume pasos de lista; al cerrarse, la flecha ya no estará mantenida o el estado se reinicia con la siguiente pulsación.
- [Salto de página en Biblioteca dispara carga de carátulas] → Ya hay presupuesto de carátulas por fotograma (`COVER_BUDGET_PER_FRAME`); un paso cada 66 ms no supera lo que hoy hace pulsar rápido.

## Memoria y recursos

El cambio no reserva memoria dinámica, no abre archivos ni crea hilos: el estado de repetición es una estructura estática en `utils.c`.

## Licencias

No se incorpora código ni assets de terceros.

## Estrategia de pruebas

- **Compilación**: el `.vpk` compila limpio con `-Wall -Werror` (el nuevo `source/key_repeat.c` se añade a `CMakeLists.txt`).
- **Tests en PC** (`tests/test_key_repeat.c`, con ASan/UBSan):
  - pulsación nueva produce un paso inmediato y nada más hasta el retardo;
  - con el botón mantenido, primer paso repetido al cumplirse el retardo y luego uno por intervalo;
  - pasado el umbral, los pasos son de página;
  - soltar y volver a pulsar reinicia retardo y tamaño de paso;
  - ↑ y ↓ a la vez no repiten; cambiar de dirección reinicia;
  - un fotograma de 1 s da un solo paso, no una ráfaga;
  - `KeyRepeat_Move`: vuelta en pulsación nueva, recorte en paso repetido y en página, `count` 0 y 1.
- **Consola**: recorrer los escenarios de la spec en Biblioteca (vista Canciones con una biblioteca grande), Carpetas, elegir carpeta y Ajustes, y comprobar que Reproduciendo, L/R y confirmar no repiten.

## CI

Sin cambios en el workflow: `make -C tests` ya corre todo lo listado en `BIN` de `tests/Makefile`; basta con añadir `test_key_repeat` ahí.
