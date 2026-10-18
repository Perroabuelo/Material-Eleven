# Debug Log Specification

## Purpose

Define el log de desarrollo de la app: un registro en la tarjeta, apagado por defecto, de lo que la app hizo durante una sesión (pantallas, input, estado mínimo de cada pantalla y acciones), con un formato estable para poder diagnosticar fallos y verificar escenarios leyendo un archivo.

## Requirements

### Requirement: El log se prende solo con el marcador
El log SHALL prenderse solo si, al arrancar la app, existe `ux0:data/ElevenMPV/debug` (un archivo, de cualquier contenido). Crear o borrar el marcador con la app abierta SHALL NOT cambiar nada hasta el próximo arranque. Sin el marcador, la app SHALL NOT crear, abrir ni modificar `debug.log` ni `debug.prev.log`, y SHALL NOT reservar memoria ni crear hilos para el log.

#### Scenario: Sin marcador no hay log
- **WHEN** el usuario borra `ux0:data/ElevenMPV/debug`, `debug.log` y `debug.prev.log`, abre la app, navega por las cuatro pantallas y la cierra
- **THEN** en `ux0:data/ElevenMPV/` no existe `debug.log` ni `debug.prev.log`

#### Scenario: Con marcador el log empieza al arrancar
- **WHEN** el usuario crea un archivo vacío `ux0:data/ElevenMPV/debug` y abre la app
- **THEN** existe `ux0:data/ElevenMPV/debug.log`, y su primera línea es de categoría `log`, con el evento `start`, la versión de la app y la fecha y hora de la consola

#### Scenario: El marcador no actúa a mitad de sesión
- **WHEN** el usuario abre la app sin el marcador, lo crea con la app abierta desde el modo USB de VitaShell y vuelve a la app
- **THEN** no se crea `debug.log` hasta que la app se cierra y se vuelve a abrir

### Requirement: El arranque conserva la sesión anterior
Al arrancar con el log prendido, si existe `debug.log`, el sistema SHALL renombrarlo a `debug.prev.log`, reemplazando el `debug.prev.log` que hubiera, antes de escribir la primera línea de la sesión nueva. Así, abrir la app después de un cierre inesperado SHALL NOT borrar el registro de la sesión que se cerró.

La excepción es cuando la app se relanza a sí misma entre dos escenarios de una suite (ver `dev/console-scripts`). En ese caso, el sistema SHALL NOT rotar el archivo y SHALL seguir escribiendo al final del mismo `debug.log`, de modo que la suite completa quede en un solo archivo. Cada proceso nuevo SHALL empezar igual con su línea `log` `event=start`.

#### Scenario: Reabrir después de cerrar con PS
- **WHEN** con el log prendido, el usuario abre la app, entra en la Biblioteca, cierra la app con el botón PS y la vuelve a abrir
- **THEN** `debug.prev.log` contiene la sesión anterior, incluida su línea `screen` con `to=library`, y `debug.log` empieza con la línea `start` de la sesión nueva

#### Scenario: La suite queda en un solo archivo
- **WHEN** con el log prendido, el usuario abre la app con una suite de tres escenarios y la deja terminar
- **THEN** `debug.log` empieza con la línea `start` del primer proceso, contiene tres líneas `log` con `event=start` y las tres líneas `script` con `event=result`, y `debug.prev.log` es la sesión anterior a la suite

### Requirement: Formato de las líneas
Cada línea del log SHALL tener la forma `<tiempo> <categoría> <campo>=<valor> ...`, separada por espacios y terminada en salto de línea, donde:
- `<tiempo>` son los segundos desde el arranque de la app, con exactamente tres decimales (por ejemplo `12.345`);
- `<categoría>` es una palabra en minúsculas de la tabla de eventos;
- cada `<valor>` va tal cual si no está vacío y no contiene espacios, comillas dobles, barras invertidas ni saltos de línea. Si no, va entre comillas dobles, con `\"`, `\\` y `\n` en lugar de las comillas, las barras y los saltos de línea que contenga.

Una línea SHALL NOT pasar de 512 bytes, salto de línea incluido. Si un valor no cabe, el sistema SHALL recortarlo sin cortar un carácter UTF-8 por la mitad, cerrar sus comillas y terminar la línea con el campo `cut=1`.

El nombre de una categoría y el de un campo que este spec define SHALL NOT cambiar de significado ni desaparecer sin modificar este spec. Un cambio SHALL poder agregar campos nuevos al final de una línea; quien lee el log ignora los que no conoce.

#### Scenario: Valor con espacios y comillas
- **WHEN** el test en PC formatea un evento `track` con el campo `file` igual a `ux0:/Mis cosas/Say "Hi".mp3`
- **THEN** la línea contiene `file="ux0:/Mis cosas/Say \"Hi\".mp3"`

#### Scenario: Valor demasiado largo
- **WHEN** el test en PC formatea un evento cuyo valor es un texto de 2000 bytes con caracteres de dos bytes en UTF-8
- **THEN** la línea mide como máximo 512 bytes, termina en `cut=1` seguido de salto de línea, sus comillas están cerradas y el valor recortado es UTF-8 válido

#### Scenario: Tiempo con tres decimales
- **WHEN** el test en PC formatea un evento a los 3 500 000 microsegundos del arranque
- **THEN** la línea empieza con `3.500 `

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
| `nowplaying` | cuando cambia su estado | `file`, `paused` (`0` o `1`), `view` (`normal`, `lyrics` o `visualizer`), `style` (`spectrum`, `scope`, `vu`, `disc` o `pulse`: el estilo del visualizador elegido en la sesión, también cuando la vista abierta no es la de visualizador) |
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

#### Scenario: Recorrer las vistas y los estilos de Reproduciendo
- **WHEN** con el log prendido, el usuario reproduce una pista, en Reproduciendo pulsa D-pad abajo, luego D-pad derecha dos veces, luego D-pad arriba y luego D-pad arriba otra vez
- **THEN** el log tiene, en este orden, líneas `nowplaying` con `view=normal style=spectrum`, `view=visualizer style=spectrum`, `view=visualizer style=scope`, `view=visualizer style=vu`, `view=lyrics style=vu` y `view=normal style=vu`

### Requirement: El estado de una pantalla se registra solo cuando cambia
El sistema SHALL escribir una línea de estado de pantalla (`folders`, `library`, `settings` o `nowplaying`) solo si difiere de la última línea de estado que escribió esa pantalla, y siempre al abrirla. Lo que cambia sin que cambie ningún campo de la línea, como el desplazamiento de la lista en píxeles o el avance de la pista, SHALL NOT producir líneas.

#### Scenario: Sin input, sin líneas de estado
- **WHEN** con el log prendido, el usuario deja la Biblioteca abierta sin tocar nada durante 30 segundos mientras suena una pista
- **THEN** en esos 30 segundos el log no tiene ninguna línea `library`

#### Scenario: Estado repetido
- **WHEN** el test en PC entrega al log dos veces seguidas la misma línea de estado de `library`, luego una distinta y luego la primera otra vez
- **THEN** el log tiene tres líneas `library`: la primera, la distinta y otra vez la primera

### Requirement: Apagado no cambia la app
Con el log apagado, la app SHALL comportarse igual que sin este cambio. El overlay de debug SHALL mostrar el HEAP en uso igual que antes de este cambio.

#### Scenario: HEAP sin marcador
- **WHEN** el usuario, sin el marcador, abre la app, abre el overlay de debug en Carpetas y anota el HEAP en uso, e instala la versión anterior a este cambio y repite lo mismo
- **THEN** los dos HEAP en uso son iguales, con una diferencia de no más de unos pocos KB

### Requirement: Prendido no traba la UI ni el audio
Con el log prendido, la app SHALL NOT escribir a la tarjeta desde el hilo de la UI ni desde los hilos de audio. Las líneas de un evento SHALL quedar en `debug.log` en no más de un segundo, aunque la app siga abierta. Si no queda espacio para una línea, el sistema SHALL descartarla y contarla en vez de esperar, y SHALL registrar cuántas descartó con una línea `log` `event=dropped` en cuanto vuelva a haber espacio. Cerrar la app con el botón PS SHALL perder como máximo las líneas del último segundo.

#### Scenario: Mantener abajo en una lista larga
- **WHEN** con el log prendido y una pista sonando, el usuario mantiene abajo durante 10 segundos en la vista Canciones de una biblioteca de 291 canciones
- **THEN** la lista se mueve con la misma fluidez que con el log apagado, la pista suena sin cortes, y el log tiene una línea `library` por cada fila por la que pasó la selección o, si perdió líneas, una línea `event=dropped` que las cuenta

#### Scenario: Las líneas llegan a la tarjeta con la app abierta
- **WHEN** con el log prendido, el usuario abre la Biblioteca, espera dos segundos, cierra la app con el botón PS y lee `debug.log` desde la PC
- **THEN** el archivo tiene la línea `screen` con `to=library`

### Requirement: El log no hace crecer la memoria
Con el log prendido, la memoria que el log toma SHALL reservarse una vez al arrancar y no crecer con el uso. Repetir una acción SHALL NOT hacer crecer el HEAP en uso más que con el log apagado.

#### Scenario: Diez vueltas por las pantallas
- **WHEN** con el log prendido, el usuario recorre con el nav rail Biblioteca, Ajustes, Reproduciendo y Carpetas diez veces
- **THEN** el `heap_kb` de la última línea `screen` con `to=folders` es igual al de la primera, con una diferencia de no más de unos pocos KB

### Requirement: El overlay muestra el estado del log
El overlay de debug SHALL mostrar una línea con el estado del log. Con el log apagado dice que está apagado. Con el log prendido dice que está activo, cuántas líneas lleva escritas en la sesión y cuántas perdió. Si el log no pudo prenderse, por ejemplo por falta de memoria o porque no pudo crear el archivo, la línea SHALL decir que falló, y la app SHALL seguir funcionando con el log apagado.

#### Scenario: Overlay con el log apagado
- **WHEN** el usuario, sin el marcador, abre el overlay de debug con L + R + SELECT
- **THEN** el overlay tiene una línea que dice que el log está apagado

#### Scenario: Overlay con el log prendido
- **WHEN** el usuario, con el marcador, navega un rato y abre el overlay de debug
- **THEN** el overlay dice que el log está activo, con un número de líneas mayor que cero que crece al seguir navegando, y 0 líneas perdidas
