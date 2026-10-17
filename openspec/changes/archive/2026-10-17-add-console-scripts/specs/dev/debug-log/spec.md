## MODIFIED Requirements

### Requirement: El arranque conserva la sesión anterior
Al arrancar con el log prendido, si existe `debug.log`, el sistema SHALL renombrarlo a `debug.prev.log`, reemplazando el `debug.prev.log` que hubiera, antes de escribir la primera línea de la sesión nueva. Así, abrir la app después de un cierre inesperado SHALL NOT borrar el registro de la sesión que se cerró.

La excepción es cuando la app se relanza a sí misma entre dos escenarios de una suite (ver `dev/console-scripts`). En ese caso, el sistema SHALL NOT rotar el archivo y SHALL seguir escribiendo al final del mismo `debug.log`, de modo que la suite completa quede en un solo archivo. Cada proceso nuevo SHALL empezar igual con su línea `log` `event=start`.

#### Scenario: Reabrir después de cerrar con PS
- **WHEN** con el log prendido, el usuario abre la app, entra en la Biblioteca, cierra la app con el botón PS y la vuelve a abrir
- **THEN** `debug.prev.log` contiene la sesión anterior, incluida su línea `screen` con `to=library`, y `debug.log` empieza con la línea `start` de la sesión nueva

#### Scenario: La suite queda en un solo archivo
- **WHEN** con el log prendido, el usuario abre la app con una suite de tres escenarios y la deja terminar
- **THEN** `debug.log` empieza con la línea `start` del primer proceso, contiene tres líneas `log` con `event=start` y las tres líneas `script` con `event=result`, y `debug.prev.log` es la sesión anterior a la suite

### Requirement: Eventos que se registran
Con el log prendido, el sistema SHALL registrar al menos estos eventos, con al menos estos campos:

| Categoría | Cuándo | Campos |
|---|---|---|
| `log` | al arrancar | `event=start`, `version`, `time` (fecha y hora de la consola, `AAAA-MM-DD HH:MM:SS`) |
| `log` | cuando vuelve a haber espacio después de perder líneas | `event=dropped`, `count` (líneas perdidas desde la anterior) |
| `screen` | al abrir cada pantalla de nivel superior | `to` (`folders`, `library`, `settings` o `nowplaying`), `heap_kb` (HEAP en uso, el mismo que muestra el overlay) |
| `input` | en el fotograma en que se aprieta o se suelta algún botón | `down` y/o `up`: los botones de ese fotograma unidos con `+`, por ejemplo `down=CROSS+UP` |
| `touch` | al apoyar y al levantar el dedo del panel frontal | `event` (`down` o `up`), `x`, `y` (en píxeles de pantalla, 0 a 959 y 0 a 543) |
| `folders` | cuando cambia su estado | `dir`, `sel`, `count` |
| `library` | cuando cambia su estado | `view`, `group` (`-` fuera de un grupo), `sel`, `count`, `mode` (`normal`, `select` o `move`), `menu` (`none`, `options`, `picker` o `confirm`) |
| `settings` | cuando cambia su estado | `category`, `item`, `lang` (el idioma elegido en Ajustes: `system`, `english` o `spanish`) |
| `nowplaying` | cuando cambia su estado | `file`, `paused` (`0` o `1`) |
| `track` | al intentar abrir una pista | `event=open`, `file`, `result` (`ok` o `fail`) y, si abrió, `format`, `rate`, `channels` |
| `track` | al terminar una pista por llegar al final | `event=end`, `file` |
| `queue` | cuando la cola pasa a otra pista | `pos` (desde 0), `count` |
| `playlist` | al guardar una playlist | `name`, `entries`, `result` (`ok` o `fail`) |
| `library` | al terminar un escaneo de la biblioteca | `event=scan`, `tracks`, `result` (`ok` o `fail`) |
| `script` | durante una suite de escenarios | los eventos y campos que define `dev/console-scripts` |

`sel` SHALL contar desde 0 sobre la lista que la pantalla muestra, y `count` SHALL ser el largo de esa lista. Los botones SHALL nombrarse `CROSS`, `CIRCLE`, `SQUARE`, `TRIANGLE`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `L`, `R`, `START` y `SELECT`.

#### Scenario: Recorrido corto por la Biblioteca
- **WHEN** con el log prendido, el usuario abre la Biblioteca desde el nav rail, baja dos filas en Canciones con el botón abajo y luego pulsa Cuadrado
- **THEN** el log tiene, en este orden: una línea `screen` con `to=library` y `heap_kb`; líneas `input` con `down=DOWN` y `up=DOWN` dos veces; líneas `library` con `view=songs`, `sel=1` y luego `sel=2`; una línea `input` con `down=SQUARE`, y una línea `library` con `menu=options`

#### Scenario: Pista que no abre
- **WHEN** con el log prendido, el usuario elige en Carpetas un `.mp3` dañado (un archivo de texto renombrado)
- **THEN** el log tiene una línea `track` con `event=open`, el `file` de ese archivo y `result=fail`

#### Scenario: Fin de pista y avance de la cola
- **WHEN** con el log prendido, el usuario reproduce una carpeta de tres pistas cortas desde la primera y deja que la primera termine
- **THEN** el log tiene una línea `track` con `event=end` y el `file` de la primera, seguida de una línea `queue` con `pos=1` y `count=3`, y de una línea `track` con `event=open`, el `file` de la segunda y `result=ok`

#### Scenario: Guardar una playlist
- **WHEN** con el log prendido, el usuario añade un álbum de 12 canciones a una playlist nueva llamada "Prueba"
- **THEN** el log tiene una línea `playlist` con `name=Prueba`, `entries=12` y `result=ok`

#### Scenario: Toque en la pantalla
- **WHEN** con el log prendido, el usuario toca la pantalla cerca del centro y levanta el dedo
- **THEN** el log tiene una línea `touch` con `event=down` y otra con `event=up`, las dos con `x` entre 400 y 560 e `y` entre 200 y 340
