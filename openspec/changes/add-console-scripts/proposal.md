## Why

`add-debug-log` hizo que la consola se pudiera observar, pero no controlar: cada escenario de consola todavía necesita que el usuario apriete los botones, recuerde el guion y cierre la app. Este cambio permite que la app ejecute sola una suite de escenarios escrita en un archivo. Inyecta botones y toques, espera estados del log y deja un resultado `pass` o `fail` por escenario en `debug.log`. Así, el agente puede verificar la funcionalidad (navegación, botones, acciones) sin intervención, y el usuario solo despliega, abre la app y vuelve al modo USB. Que se vea bien, ande fluido y suene bien lo sigue verificando el usuario a mano.

Rama: `change/add-console-scripts`

## What Changes

- **Una suite de escenarios en la tarjeta.** Si al arrancar existen el marcador del log (`ux0:data/ElevenMPV/debug`) y `ux0:data/ElevenMPV/suite.txt`, la app ejecuta la suite en vez de esperar al usuario. Sin el marcador, la suite se ignora.
- **Un guion de texto simple.** Una línea por comando: `scenario`, `press`, `hold`, `tap`, `swipe`, `wait`, `sleep` y `seed`. Un error de sintaxis se informa con su número de línea y no ejecuta nada.
- **Esperar estados, no tiempo.** `wait library sel=2` espera una línea del log con esos campos y falla el escenario si no llega antes del timeout. El `wait` es a la vez la sincronización y la verificación.
- **Cada escenario en un proceso nuevo.** Entre escenarios, la app restaura `config.cfg` y `lastdir.txt` a como estaban al empezar la suite y se relanza a sí misma. Así un escenario que falla no deja estado a medias para el siguiente.
- **Resultado en el log.** La categoría nueva `script` registra el inicio de la suite, cada escenario, cada paso, el resultado de cada escenario con el motivo si falló, y un resumen final. Durante una suite, `debug.log` no rota al relanzar, y la suite completa queda en un solo archivo.
- **Al terminar, la app sale sola.** Muestra el resumen unos segundos y cierra. Un cierre inesperado a mitad de un escenario queda registrado como `interrupted` en el siguiente arranque.
- **El usuario siempre puede cortar.** Cualquier botón físico o toque real aborta la suite y le devuelve la app.
- **Repetible.** `seed N` fija la semilla del orden aleatorio de la cola.
- **Suites versionadas.** Las suites viven en `tests/console/`, una por capacidad, y crecen con cada cambio. Se agrega una regla en `openspec/config.yaml` para que el design de un cambio con escenarios de consola diga qué suite los cubre.

## Capabilities

### New Capabilities

- `dev/console-scripts`: la ejecución de suites de escenarios en la consola. Define cuándo se activa, el lenguaje del guion, cómo se inyecta el input, cómo se espera y se decide un resultado, el aislamiento entre escenarios, el aborto por el usuario, el cierre y las líneas `script` del log.

### Modified Capabilities

- `dev/debug-log`: el arranque no rota `debug.log` cuando la app se relanza dentro de una suite, y la tabla de eventos suma la categoría `script`.

## Impact

- **Módulo puro nuevo:** parsea el guion, ejecuta los pasos fotograma a fotograma (qué botones y qué toque entrega cada fotograma) y decide si una línea del log cumple un `wait`. Lleva test en PC.
- **Módulo nuevo del lado de la consola:** lee la suite y su estado, restaura la configuración, relanza el proceso, detecta un cierre inesperado y sale al terminar.
- **Puntos de inyección:**
  - `source/utils.c` (`Utils_ReadControls`): botones;
  - `source/touch.c` (`Touch_Update`): tacto;
  - `source/ui_gpu.c`: el overlay lee el pad por su cuenta, así que también tiene que ignorar el pad físico durante una suite;
  - `source/debug_log.c`: entrega al guion cada línea que se registra, para los `wait`.
- **`source/main.c`:** arranque de la suite, después del log y antes de cargar la configuración.
- **`source/menus/menu_audioplayer.c`:** `srand` usa la semilla del guion si la hay.
- **`tests/console/`:** suites nuevas, con un `README.md` que explica el formato y los fixtures que pide cada suite.
- **`openspec/config.yaml`:** la regla nueva del design.
- **`README.md`:** cómo correr una suite.
- No se agregan dependencias de terceros.

## Fuera de alcance

- Desplegar la suite, esperar a que la Vita vuelva al modo USB y traer `debug.log`. Eso lo hace `scripts/vita.ps1`, que es local y no se versiona.
- Comunicarse con la consola por red (FTP, vitacompanion). Se descartó: todo pasa por USB.
- Escribir en el teclado del sistema (el diálogo de búsqueda de la Biblioteca). Un escenario que lo abre no puede seguir.
- Capturas de pantalla desde el guion, y verificar imagen, fluidez o audio. Eso sigue siendo verificación humana.
- Restaurar playlists, la carpeta de escaneo o el índice de la biblioteca entre escenarios. Son fixtures que prepara quien despliega la suite.
- Lanzar la app sin intervención del usuario.

## Notas de version

Versión objetivo: **v4.3.0** (minor: funcionalidad nueva), la siguiente a v4.2.0.

- Developer test suites: with the developer log on, a `suite.txt` in `ux0:data/ElevenMPV/` makes the app replay scripted button presses and touches, check the result of each scenario and record it in `debug.log`, then close by itself. Pressing any button stops the suite.
