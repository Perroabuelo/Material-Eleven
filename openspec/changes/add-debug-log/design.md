## Context

La motivación está en `proposal.md`, y el comportamiento en `specs/dev/debug-log/spec.md`. Aquí solo va lo que da forma a la implementación.

- **Hilos.** La app tiene el hilo principal (pantallas, input y dibujo), un hilo de audio por canal (`vitaaudiolib.c`, prioridad 0x40, que llama al decodificador) y `power_tick_thread` (`utils.c`). Ningún decodificador informa errores a mitad de la pista: al llegar al final ponen `playing` en falso, y el fin de pista lo detecta `Music_Update` en el hilo principal. `Audio_Init` también corre en el hilo principal. Por eso, en este cambio todos los eventos nacen en el hilo principal.
- **Puntos centrales que ya existen.**
  - `main.c` es el único lugar que abre pantallas.
  - `Utils_ReadControls` (`utils.c`) es el único lugar que lee los botones, una vez por fotograma.
  - `Touch_Update` (`touch.c`) es el único lugar que lee el panel táctil.
  - El estado de cada pantalla vive en variables estáticas de su `menu_*.c`, por ejemplo `view`, `selection`, `inside`, `selecting`, `moving`, `options_open`, `picker_open` y `confirm_open` en `menu_library.c`.
- **Cómo se cierra la app.** Se cierra con el botón PS, que termina el proceso sin pasar por el final de `main`. El código de cierre de `main` existe, pero hoy no se ejecuta.
- **El overlay de debug** (`ui_gpu.c`) ya calcula el HEAP en uso con `mallinfo().uordblks` y ya guarda capturas en `ux0:data/ElevenMPV/glitch/`.
- **Memoria.** El heap de newlib es un solo bloque reservado al arrancar. Una fuga se ve en el HEAP del overlay y no en la memoria libre de usuario.

## Goals / Non-Goals

**Goals:**
- Que con el log apagado el costo sea una lectura de un booleano por punto de registro: sin formatear, sin buffer y sin archivo.
- Que el hilo principal nunca espere a la tarjeta, y que solo espere un lock por el tiempo de copiar una línea.
- Que el formato, el recorte, el buffer circular y el descarte de estados repetidos sean lógica pura, probada en PC con sanitizers.

**Non-Goals:**
- Un sistema de niveles de log (debug, info, warn). Cada evento se registra o no según el spec.
- Registrar cada fotograma, el desplazamiento de las listas o el avance de la pista.
- Garantizar las líneas del último segundo ante un cierre con PS o un crash (ver Riesgos).

## Decisions

### 1. Dos módulos: `log_line.c` puro y `debug_log.c` de consola

- **`source/log_line.c` / `include/log_line.h`**, sin SCE:
  - armar una línea en un `LogLine` de 512 bytes del llamador: tiempo, categoría y campos de texto, enteros y botones;
  - el escape de valores y el recorte de UTF-8 con `cut=1`;
  - un buffer circular de bytes sobre memoria que entrega el llamador, que acepta o rechaza líneas enteras y cuenta las rechazadas;
  - el descarte de líneas de estado repetidas, comparando con la última de su canal.
- **`source/debug_log.c` / `include/debug_log.h`**, con SCE: el marcador, la rotación, la reserva de memoria, el lock, el hilo que escribe y la API que llaman los puntos de registro.

*Por qué:* así lo difícil de acertar (escape, recorte, límites del buffer circular, contador de pérdidas) se prueba en PC con ASan y UBSan. Es el mismo patrón de `capture_bmp.c` con el overlay.

*Alternativa descartada:* un `printf` con `sceIoWrite` directo en cada punto. Es más simple, pero escribe a la tarjeta desde el hilo principal en cada evento, y mantener abajo genera unas 20 líneas por segundo.

### 2. La API: armar en la pila y entregar de una vez

```c
if (DebugLog_On()) {
    LogLine line;
    DebugLog_Begin(&line, "track");
    LogLine_Str(&line, "event", "open");
    LogLine_Str(&line, "file", path);
    LogLine_Str(&line, "result", ok ? "ok" : "fail");
    DebugLog_Commit(&line);
}
```

- `DebugLog_On()` lee un booleano estático. Todo lo demás queda dentro del `if`, así que con el log apagado no se formatea nada.
- `DebugLog_Begin` pone el tiempo (`sceKernelGetProcessTimeWide`) y la categoría.
- `DebugLog_Commit` toma el lock, copia la línea al buffer circular y lo suelta.
- Para el estado de pantalla, `DebugLog_CommitState(&line, DEBUG_STATE_LIBRARY)` hace lo mismo, pero solo si la línea difiere de la última de ese canal.
- `LogLine` vive en la pila de quien registra: 512 bytes, solo dentro del `if`.

*Por qué no una función variádica con formato:* el escape necesita saber dónde empieza y dónde termina cada valor. Con `printf` habría que escapar antes de formatear, y eso obliga a buffers intermedios en cada llamador.

### 3. Activación, rotación y arranque

`DebugLog_Init()` se llama en `main.c` justo después de `sceIoMkdir("ux0:data/ElevenMPV")`, antes de cargar la configuración, para registrar también lo que pase al cargar. Hace esto, en orden:

1. Hace `sceIoGetstat` del marcador. Si no existe, el estado queda en apagado y termina.
2. Hace un solo `malloc` con el buffer circular (64 KB) y los cuatro canales de estado (4 × 512 B).
3. Crea el lock (`sceKernelCreateLwMutex`).
4. Borra `debug.prev.log` y renombra `debug.log` a `debug.prev.log` (`sceIoRename`). Que no existan no es un error.
5. Crea `debug.log` vacío, para comprobar que se puede escribir.
6. Crea e inicia el hilo que escribe.
7. Prende el booleano y registra la línea `log event=start`.

Si cualquier paso falla, se deshace lo hecho hasta ahí (ver Recursos), el estado queda en "falló" con el motivo para el overlay y la app sigue con el log apagado.

La versión sale de `VITA_VERSION`, que CMake ya define. Se pasa al código con `target_compile_definitions(... APP_VERSION="${VITA_VERSION}")`, de modo que el log dice `version=04.20`.

### 4. El hilo que escribe

- **Prioridad:** más baja que el hilo principal y que el audio, por ejemplo 0xA0. **Pila:** 16 KB.
- **Ciclo:** cada 250 ms hace `sceKernelDelayThread` y después vacía el buffer:
  1. toma el lock, copia hasta 4 KB a un bloque de su pila y lo suelta;
  2. abre `debug.log` con `SCE_O_WRONLY | SCE_O_APPEND`, escribe y cierra;
  3. repite mientras quede algo en el buffer.
- **Abrir y cerrar en cada vaciado** en lugar de dejar el archivo abierto:
  - Cerrar asienta los datos en la tarjeta, así que cerrar con PS pierde como máximo lo que llegó después del último vaciado.
  - Ningún descriptor queda abierto mientras la consola está suspendida.
  - El costo, un open y un close cada 250 ms, recae en un hilo que no dibuja.
- **Con 250 ms**, una línea tarda menos de un segundo en llegar a la tarjeta, como pide el spec, con margen aunque el hilo se atrase un ciclo.

### 5. Buffer lleno: descartar y contar

`LogRing_Push` rechaza la línea completa si no cabe, y suma 1 al contador de descartadas. No hay líneas a medias. Cuando el contador es mayor que cero, el siguiente `Push` primero intenta meter `log event=dropped count=N`, y solo si eso cabe pone el contador en cero y agrega la línea nueva.

Con 64 KB y líneas de unos 100 bytes caben unas 650 líneas. Es más de medio minuto al ritmo de mantener abajo, y el hilo vacía cada 250 ms. En la práctica solo se descarta si la tarjeta se traba varios segundos.

El overlay muestra dos totales de la sesión: líneas aceptadas y líneas descartadas.

### 6. Estado de pantalla: una función por pantalla, una vez por fotograma

Cada `menu_*.c` gana una función estática, `Menu_LogLibraryState()` y sus equivalentes, que su bucle llama una vez por fotograma después de procesar el input. Dentro del `if (DebugLog_On())`, la función arma la línea con los campos del spec y llama a `DebugLog_CommitState`.

Al abrir una pantalla, `main.c` llama a `DebugLog_Screen(to)`. Esa función registra la línea `screen` y vacía los cuatro canales de estado, para que la primera línea de estado de la pantalla nueva siempre se escriba.

Campos mínimos de cada pantalla (los nombres los fija el spec):
- **`folders`:** `dir` (la carpeta abierta), `sel`, `count`.
- **`library`:**
  - `view`: `songs`, `recent`, `artists`, `albums` o `playlists`;
  - `group`: el nombre del artista, álbum o playlist abierto, o `-`;
  - `sel` y `count` de la lista visible, con el filtro de búsqueda aplicado;
  - `mode`: `select` si `selecting`, `move` si `moving`, si no `normal`;
  - `menu`: `options`, `picker` o `confirm`, según cuál esté abierto; si no, `none`.
- **`settings`:** `category` e `item`, como índices.
- **`nowplaying`:** `file` y `paused`.

*Alternativa descartada:* que cada punto que cambia el estado registre por su cuenta. En `menu_library.c` hay decenas de puntos así, y cada uno olvidado sería un hueco en la verificación. Comparar una vez por fotograma no deja huecos, y su costo (armar 512 bytes y un `memcmp`) solo se paga con el log prendido.

### 7. Input y tacto

- **`Utils_ReadControls`** registra los flancos de los botones físicos: `down = pad.buttons & ~old_pad.buttons` y `up = old_pad.buttons & ~pad.buttons`, antes de `ScreenOff_Filter`, porque se registra lo que el usuario apretó y no lo que la app dejó pasar. La tabla de máscara a nombre (`SCE_CTRL_CROSS` → `CROSS`) vive en `debug_log.c`. `LogLine_Buttons` en el módulo puro recibe la máscara y la tabla, y une los nombres con `+`.
- **`Touch_Update`** registra `touch event=down` en el fotograma en que se apoya el dedo y `event=up` en el que se levanta, con la posición ya pasada a píxeles de pantalla. Los movimientos intermedios no se registran.
- La autorrepetición no genera líneas de `input` (el botón sigue apretado). Sí genera líneas de estado, porque cambia `sel`.

### 8. Pistas, cola, playlists y escaneo

- **`track event=open`:** al final de `Audio_Init` (`audio.c`), que conoce `file_type`, la frecuencia y los canales. El camino de falla registra `result=fail`.
- **`track event=end`:** en `Music_Update`, cuando ve que la pista terminó, antes de pasar a la siguiente.
- **`queue`:** en `Music_HandleNext`, después de mover la cola, con la posición y el largo de `queue.c`.
- **`playlist`:** en `PlaylistStore_Save`, con el nombre de la playlist, el número de entradas y el resultado.
- **`library event=scan`:** al terminar `Library_RunScan`, con `Library_Count()` y el resultado.

### 9. Overlay

La página de estadísticas suma una línea, después de la del HEAP:
- `LOG    apagado`
- `LOG    activo  1234 lineas  perdidas 0`
- `LOG    fallo: <motivo>` (`sin memoria`, `no se pudo crear debug.log` o `no se pudo crear el hilo`)

`debug_log.h` expone `DebugLog_GetStatus()` con el estado, el motivo y los dos totales.

### 10. Regla nueva en `openspec/config.yaml`

En `rules.design` se agrega: si el cambio tiene escenarios de consola, el design indica qué líneas de `debug.log` verifican cada uno, y agrega al log los campos que falten, sin renombrar ni quitar los que ya define `dev/debug-log`.

### 11. README

Una sección corta en inglés: cómo prender el log con el marcador, dónde queda, que la sesión anterior se guarda como `debug.prev.log`, y que el log incluye rutas y nombres de canciones, así que conviene revisarlo antes de adjuntarlo a un issue público.

## Recursos

| Recurso | Lo toma | Lo libera |
|---|---|---|
| Bloque de buffer y canales (`malloc`) | `DebugLog_Init`, paso 2 | `DebugLog_Shutdown`; o `DebugLog_Init` si falla un paso posterior |
| LwMutex | `DebugLog_Init`, paso 3 | `DebugLog_Shutdown`, después de que el hilo terminó; o `DebugLog_Init` si falla un paso posterior |
| Hilo que escribe | `DebugLog_Init`, paso 6 | `DebugLog_Shutdown`: pone la bandera de salida, espera con `sceKernelWaitThreadEnd`, hace un último vaciado desde el hilo principal y `sceKernelDeleteThread` |
| Descriptor de `debug.log` | el hilo, en cada vaciado | el mismo vaciado, siempre; también si `sceIoWrite` falla |
| `LogLine` | la pila de quien registra | al salir del `if` |

`DebugLog_Shutdown` va en el código de cierre de `main`, antes de `UI_Debug_Free`. Hoy ese código no corre porque la app se cierra con PS, que termina el proceso y le devuelve todo al sistema. Aun así queda correcto para cuando haya otra forma de salir. Si el log nunca se prendió, `DebugLog_Shutdown` no hace nada.

Ninguno de estos recursos crece con el uso: se reservan una vez por sesión.

## Estrategia de pruebas

1. **Tests en PC: `tests/test_log_line.c`**, que se suma a `BIN` en `tests/Makefile` y corre con ASan y UBSan:
   - tiempo con tres decimales (0, 999 µs, 3 500 000 µs, más de una hora);
   - valores sin comillas, con espacio, con `"`, con `\` y con salto de línea, y un valor vacío;
   - recorte: un valor de 2000 bytes ASCII, y otro con caracteres de 2 y de 3 bytes cortado justo en medio de uno; la línea mide como máximo 512 bytes, termina en `cut=1\n` y es UTF-8 válido;
   - botones: máscara vacía, uno solo, varios en el orden de la tabla;
   - buffer circular: llenar, vaciar parcialmente, dar la vuelta al final del bloque y leer en orden; rechazar una línea que no cabe y contarla; la línea `dropped` entra antes que la siguiente y lleva el contador correcto; si `dropped` no cabe, se sigue contando;
   - descarte de estados: igual, distinta, otra vez la primera (el escenario del spec), y que vaciar los canales hace que la siguiente se escriba;
   - camino de error: un buffer de tamaño 0 o menor que una línea rechaza todo sin escribir fuera.
2. **Compilación:** `scripts/build.sh` sin warnings con `-Werror`.
3. **Consola, siguiendo los escenarios del spec**, con `scripts/vita.ps1 -App` para desplegar y leyendo `debug.log` desde la PC por USB:
   - sin marcador: no aparece ningún archivo; el overlay dice `apagado`; el HEAP en Carpetas comparado con el de v4.1.0;
   - con marcador: la línea `start`; la rotación al cerrar con PS y reabrir; el marcador creado a mitad de sesión;
   - el recorrido corto por la Biblioteca, la pista dañada, el fin de pista con la cola, guardar una playlist y el toque, cada uno comparado línea a línea con el escenario;
   - 30 segundos sin input: ninguna línea `library`;
   - mantener abajo 10 segundos con una pista sonando, comparado de ojo con el log apagado; anotar si aparece `dropped`;
   - abrir la Biblioteca, esperar 2 s y cerrar con PS: la línea `screen` ya está en el archivo.
4. **Memoria**, según la regla del proyecto:
   - Con el log prendido, anotar el HEAP del overlay, recorrer diez veces Biblioteca, Ajustes, Reproduciendo y Carpetas, y anotarlo de nuevo; tiene que quedar igual. El log lo deja escrito en los `heap_kb` de las líneas `screen`, que son el mismo dato.
   - Comparar el HEAP con el log prendido y apagado: la diferencia tiene que ser el bloque de unos 66 KB y nada más.
   - Para el camino de error en la consola, una build de prueba no versionada que fuerza la falla del `malloc` muestra `LOG fallo: sin memoria` y la app funciona normal. El mismo camino lo cubre el test en PC del buffer de tamaño 0.

## Cambios en CI

Ninguno. El job de tests ya corre `make -C tests`, y `test_log_line` entra por `BIN`. El job del `.vpk` compila los archivos nuevos al sumarlos a `add_executable` en `CMakeLists.txt`.

## Código de terceros

Ninguno.

## Risks / Trade-offs

- **[Cierre con PS o crash pierde la cola]** Se pierde lo que llegó después del último vaciado, menos de 250 ms en el caso normal. → Es aceptable para verificar escenarios, porque se espera un momento antes de cerrar. Para crashes queda como mejora futura el rescate desde el volcado (fuera de alcance).
- **[El hilo que escribe no corre si el principal no cede]** Con prioridad baja, un trabajo largo del hilo principal sin esperas podría retrasar los vaciados. → El principal cede en cada fotograma, y los escaneos hacen E/S, que también cede. Si aun así se llena el buffer, se pierde y se cuenta, sin trabar nada. Mantener abajo 10 s lo pone a prueba.
- **[Un `menu_*.c` olvida un campo o arma mal la línea]** Una verificación leería un estado incorrecto. → Las pruebas de consola comparan línea a línea con los escenarios del spec, y la regla nueva de `config.yaml` hace que cada cambio revise los campos que usa.
- **[La pila del hilo principal]** Cada `LogLine` son 512 bytes en la pila, solo con el log prendido. → El overlay ya muestra la pila usada y su máximo. Se comprueba en la consola que el máximo no se acerque al límite con el log prendido.
- **[Privacidad]** El log tiene rutas y nombres de canciones. → Está apagado por defecto, y el README lo advierte.
- **[Formato congelado]** Renombrar un campo rompe verificaciones escritas contra él. → Los nombres están en el spec, y la regla de `config.yaml` solo permite agregar campos.
- **[`sceIoRename` falla, por ejemplo con la tarjeta llena]** → Se intenta borrar `debug.log`. Si tampoco se puede, el arranque del log falla con su motivo y la app sigue normal.

## Migration Plan

No hay datos que migrar. Quien no crea el marcador no ve ningún cambio. Para volver atrás basta con borrar el marcador.
