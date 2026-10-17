## Why

La verificación en consola es la parte más lenta de cada cambio: en `multi-select` la implementación tomó unos 16 minutos y las pruebas en la Vita, unos 70. Gran parte de ese tiempo se va en que el usuario describa lo que vio: qué pantalla, qué fila, qué marcó, qué se guardó, cuánto marcaba el HEAP. Hoy la app no deja ningún registro de lo que hizo, así que nada de eso se puede leer después. Este cambio agrega un log de desarrollo, apagado por defecto, que registra la pantalla, el input, el estado mínimo de cada pantalla y las acciones. Con él, quien revisa puede verificar escenarios y diagnosticar fallos leyendo un archivo, en vez de depender del relato.

Rama: `change/add-debug-log`

Es también la base de un cambio posterior que inyectará guiones de input desde los mismos puntos centrales donde este cambio lee el input.

## What Changes

- **Un log de desarrollo apagado por defecto.** Se prende solo si al arrancar la app existe el archivo marcador `ux0:data/ElevenMPV/debug`. Sin el marcador la app se comporta exactamente igual que hoy: no reserva memoria para el log, no crea archivos ni hilos.
- **Dónde escribe.** Escribe en `ux0:data/ElevenMPV/debug.log`. Al arrancar con el log prendido, el `debug.log` anterior pasa a `debug.prev.log`, de modo que abrir la app después de un cierre inesperado no borra el registro de esa sesión.
- **Qué registra en esta versión:**
  - el arranque, con la versión de la app y la fecha y hora de la consola;
  - cada salto de pantalla, con el HEAP en uso en ese momento;
  - los botones que se aprietan y se sueltan, y dónde se apoya y se levanta el dedo;
  - una línea de estado mínima por pantalla (Carpetas, Biblioteca, Ajustes, Reproduciendo), escrita solo cuando ese estado cambia;
  - la apertura de cada pista, con su resultado, el fin de cada pista y el avance de la cola;
  - el resultado de guardar una playlist y de escanear la biblioteca.
- **Un formato estable.** Una línea por evento: tiempo desde el arranque, categoría y campos `clave=valor`. El formato forma parte del spec, para que una verificación escrita contra él no se rompa sin aviso.
- **Sin trabar la UI ni el audio.** Las líneas se acumulan en memoria y un hilo de baja prioridad las escribe a la tarjeta cada cuarto de segundo, aproximadamente. Si el buffer se llena, se cuentan las líneas perdidas en vez de esperar.
- **El overlay de debug muestra el estado del log:** apagado o activo, cuántas líneas lleva y cuántas se perdieron.
- **Una regla nueva en `openspec/config.yaml`.** El design de un cambio con escenarios de consola dice qué líneas del log verifican cada escenario, y agrega al log los campos que falten. Así el log crece con lo que de verdad se verifica, sin instrumentar de más en este cambio.

## Capabilities

### New Capabilities

- `dev/debug-log`: el log de desarrollo. Define cómo se prende, dónde escribe y cómo conserva la sesión anterior, el formato de las líneas, qué eventos registra como mínimo, que apagado no cambia nada, que prendido no traba la UI ni el audio, y que el overlay muestra su estado.

### Modified Capabilities

Ninguna. El log observa el comportamiento de las pantallas sin cambiarlo.

## Impact

- **Módulo puro nuevo:** formatea las líneas, las acumula en un buffer circular con su contador de líneas perdidas y descarta las líneas de estado repetidas. Lleva test en PC.
- **Módulo nuevo del lado de la consola:** lee el marcador, rota el archivo, reserva el buffer y crea el lock y el hilo que escribe. Lo libera todo al salir, o si algo falla a mitad del arranque.
- **Puntos de registro:**
  - `source/main.c`: arranque y saltos de pantalla;
  - `source/utils.c` (`Utils_ReadControls`): botones;
  - `source/touch.c` (`Touch_Update`): tacto;
  - `source/menus/menu_audioplayer.c`: apertura, fin y avance de pistas;
  - `source/playlist_store.c` y `source/library.c`: guardado y escaneo;
  - una línea de estado en cada `source/menus/menu_*.c`.
- **`source/ui_gpu.c`:** una línea del overlay con el estado del log.
- **`openspec/config.yaml`:** la regla nueva del design.
- **`README.md`:** cómo prender el log, y el aviso de que registra rutas y nombres de canciones.
- No se agregan dependencias de terceros.

## Fuera de alcance

- Traer `debug.log` y las capturas a la PC. Eso lo hace `scripts/vita.ps1`, que es local y no se versiona.
- Rescatar la cola del buffer desde un volcado `.psp2dmp` con `tools/crashdump/psp2dmp.py`. Sin esto, un cierre inesperado puede perder como máximo el último cuarto de segundo del log.
- Registrar desde los hilos de audio. Hoy ningún decodificador informa errores a mitad de la pista, y el fin de pista se detecta en el hilo principal. El módulo queda preparado para usarse desde otros hilos.
- Instrumentar el estado completo de cada pantalla. Cada cambio futuro agrega los campos que pidan sus escenarios.
- Inyectar input desde un guion. Es el cambio siguiente.
- Prender o apagar el log desde la app.

## Notas de version

Versión objetivo: **v4.2.0** (minor: funcionalidad nueva), la siguiente a v4.1.0.

- Developer log, off by default: create an empty file named `debug` in `ux0:data/ElevenMPV/` and the app records screens, input and playback events to `ux0:data/ElevenMPV/debug.log`. The previous session is kept as `debug.prev.log`.
- The debug overlay shows whether the log is on, how many lines it has written and how many it dropped.
