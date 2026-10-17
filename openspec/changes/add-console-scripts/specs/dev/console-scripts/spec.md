## Purpose

Define cómo la app ejecuta sola una suite de escenarios de prueba escrita en un archivo de la tarjeta: inyecta botones y toques, espera estados del log de desarrollo y registra un resultado por escenario, para verificar la funcionalidad en la consola sin intervención del usuario.

## ADDED Requirements

### Requirement: La suite se activa solo con el marcador y el archivo
La app SHALL ejecutar una suite solo si, al arrancar, existen `ux0:data/ElevenMPV/debug` y `ux0:data/ElevenMPV/suite.txt`, y el log de desarrollo arrancó bien. Si falta el marcador o el log falló, la app SHALL ignorar `suite.txt`, sin leerlo ni modificarlo, y SHALL comportarse como sin este cambio.

#### Scenario: Suite sin marcador
- **WHEN** el usuario copia un `suite.txt` válido a `ux0:data/ElevenMPV/` sin el marcador `debug` y abre la app
- **THEN** la app responde a los botones físicos como siempre, `suite.txt` sigue con el mismo contenido y no existe `debug.log`

#### Scenario: Suite con marcador
- **WHEN** el usuario copia el marcador y un `suite.txt` con un escenario que solo hace `wait screen to=folders`, y abre la app
- **THEN** `debug.log` tiene una línea `script` con `event=suite` y `scenarios=1`, y una línea `script` con `event=result`, el nombre del escenario y `result=pass`

### Requirement: El lenguaje del guion
`suite.txt` SHALL ser texto UTF-8 con un comando por línea. Las líneas vacías y lo que sigue a `#` SHALL ignorarse. Los comandos SHALL ser:

| Comando | Efecto |
|---|---|
| `scenario <nombre>` | empieza un escenario; los pasos que siguen le pertenecen hasta el próximo `scenario` |
| `press <botones> [xN]` | aprieta los botones (unidos con `+`, por ejemplo `L+R+SELECT`) y los suelta, N veces (1 si se omite) |
| `hold <botones> <duración>` | mantiene los botones apretados durante la duración y los suelta |
| `tap <x> <y>` | apoya el dedo en el panel frontal en (x, y) y lo levanta |
| `swipe <x1> <y1> <x2> <y2> <duración>` | apoya el dedo en (x1, y1), lo mueve en línea recta hasta (x2, y2) durante la duración y lo levanta |
| `wait <categoría> <campo>=<valor> ... [timeout=<duración>]` | espera una línea del log que cumpla la condición (ver el requirement de `wait`) |
| `sleep <duración>` | espera la duración sin hacer nada |
| `seed <n>` | fija la semilla del orden aleatorio de la cola para el resto del escenario |

Los botones SHALL usar los nombres del log (`CROSS`, `CIRCLE`, `SQUARE`, `TRIANGLE`, `UP`, `DOWN`, `LEFT`, `RIGHT`, `L`, `R`, `START`, `SELECT`). Las coordenadas SHALL ser píxeles de pantalla, de 0 a 959 y de 0 a 543. Una duración SHALL escribirse como un entero seguido de `ms` o de `s`. Un `<valor>` de `wait` SHALL poder ir entre comillas dobles, con las mismas secuencias de escape que el log.

Si el archivo tiene un error (un comando desconocido, un botón desconocido, argumentos de más o de menos, una coordenada fuera de rango, un paso antes del primer `scenario`, un archivo sin escenarios), la app SHALL NOT ejecutar ningún paso, SHALL registrar el número de la primera línea con error y el motivo, SHALL renombrar `suite.txt` a `suite.done.txt` y SHALL seguir como un arranque normal con el log prendido.

#### Scenario: Guion válido
- **WHEN** el test en PC parsea un guion con comentarios, líneas vacías, dos escenarios y un comando de cada tipo
- **THEN** obtiene dos escenarios con sus pasos en orden, con los botones, las coordenadas, las duraciones y las condiciones del texto

#### Scenario: Botón desconocido
- **WHEN** el test en PC parsea un guion cuya línea 7 es `press TRIANGULO`
- **THEN** el parseo falla en la línea 7 con un motivo que nombra el botón

#### Scenario: Error en la consola
- **WHEN** con el marcador, el usuario copia un `suite.txt` cuya línea 3 es `presss CROSS` y abre la app
- **THEN** `debug.log` tiene una línea `script` con `event=error` y `line=3`, ninguna línea `script` con `event=step`, existe `suite.done.txt` y no `suite.txt`, y la app responde a los botones físicos

### Requirement: La app ve el input del guion y no el físico
Mientras corre una suite, cada lectura de botones y del panel frontal de la app, incluida la del overlay de debug, SHALL recibir lo que entrega el guion y no lo que hace el usuario. Un `press` SHALL producir en la app un fotograma en que los botones se aprietan y uno posterior en que se sueltan, sin que se dispare la autorrepetición. Un `hold` SHALL mantenerlos apretados el tiempo indicado, con la autorrepetición que eso produzca. Las líneas `input` y `touch` del log SHALL registrar el input del guion como si fuera físico.

#### Scenario: Bajar dos filas
- **WHEN** una suite tiene el escenario `wait library view=songs`, `press DOWN x2`, `wait library sel=2`, y empieza en la Biblioteca en Canciones con la selección en la primera fila
- **THEN** el log tiene dos pares de líneas `input` con `down=DOWN` y `up=DOWN`, líneas `library` con `sel=1` y `sel=2`, y el escenario termina con `result=pass`

#### Scenario: Mantener abajo
- **WHEN** una suite hace `hold DOWN 2s` en Canciones con una biblioteca de más de 50 canciones
- **THEN** el log tiene una sola línea `input` con `down=DOWN`, una con `up=DOWN` dos segundos después, y más de dos líneas `library` con `sel` creciente

#### Scenario: Toque inyectado
- **WHEN** una suite hace `tap 480 272`
- **THEN** el log tiene una línea `touch` con `event=down` y otra con `event=up`, las dos con `x=480` e `y=272`

### Requirement: Qué cumple un wait
Un `wait <categoría> <campo>=<valor> ...` SHALL cumplirse con la primera línea del log de esa categoría que tenga todos los campos indicados con exactamente esos valores. Los demás campos de la línea no importan. Las líneas candidatas SHALL ser las registradas desde que empezó el último `press`, `hold`, `tap` o `swipe` del escenario (o desde que empezó el escenario, si no hubo ninguno), más la última línea de estado de cada pantalla (`folders`, `library`, `settings`, `nowplaying`). Si no se cumple antes del timeout (5 segundos si se omite), el escenario SHALL terminar con `result=fail`, y el motivo SHALL incluir el número de línea del `wait` y la última línea de esa categoría que se registró, si hubo alguna.

#### Scenario: Campos de más en la línea
- **WHEN** el test en PC evalúa `wait library sel=2` contra la línea `library view=songs group=- sel=2 count=291 mode=normal menu=none`
- **THEN** la condición se cumple

#### Scenario: Valor distinto
- **WHEN** el test en PC evalúa `wait library sel=2` contra la línea `library view=songs sel=20`
- **THEN** la condición no se cumple

#### Scenario: El estado actual ya cumple
- **WHEN** en la consola, una suite abre la Biblioteca, espera `wait library view=songs`, y luego hace un segundo `wait library view=songs` sin input entre los dos
- **THEN** los dos `wait` se cumplen enseguida y el escenario termina con `result=pass`

#### Scenario: Timeout
- **WHEN** una suite tiene en la línea 12 `wait library sel=5 timeout=1s` sin ningún input antes, con la selección en 0
- **THEN** el log tiene, alrededor de un segundo después, una línea `script` con `event=result`, `result=fail`, un motivo con `line=12` y un campo `last` con la última línea `library`

### Requirement: Cada escenario empieza desde el mismo estado
Antes del primer escenario, la app SHALL guardar una copia de `config.cfg` y de `lastdir.txt`, tal como están (o el hecho de que no existen). Después de cada escenario, pase o falle, la app SHALL restaurarlos desde esa copia y SHALL relanzar su propio proceso. El escenario siguiente SHALL empezar en un proceso nuevo, en la pantalla de inicio, sin pista cargada y con la configuración restaurada. Si el proceso no puede relanzarse, la app SHALL registrar el motivo, terminar la suite como abortada y seguir funcionando con el input físico.

#### Scenario: Un ajuste no pasa al escenario siguiente
- **WHEN** una suite tiene un escenario que cambia el idioma en Ajustes y un segundo escenario que abre Ajustes
- **THEN** la línea `settings` del segundo escenario tiene en `lang` el idioma de antes de la suite, y al terminar la suite `config.cfg` es igual byte a byte al de antes de empezar

#### Scenario: Una pista no sigue sonando
- **WHEN** una suite tiene un escenario que reproduce una pista y un segundo escenario que hace `wait screen to=folders`
- **THEN** entre los dos escenarios hay una línea `log` con `event=start`, y el segundo escenario no tiene líneas `track` hasta que abre una pista

### Requirement: El usuario puede cortar la suite
Mientras corre una suite, si el usuario aprieta un botón físico o toca el panel frontal, la app SHALL abortar la suite enseguida, SHALL registrar `event=abort` con `reason=user`, SHALL renombrar `suite.txt` a `suite.done.txt`, SHALL restaurar `config.cfg` y `lastdir.txt` y SHALL seguir respondiendo al input físico desde ese momento, sin relanzarse.

#### Scenario: Abortar con un botón
- **WHEN** a mitad de una suite de tres escenarios, el usuario aprieta Círculo
- **THEN** el log tiene una línea `script` con `event=abort` y `reason=user`, ninguna línea `script` posterior, existe `suite.done.txt`, y la app responde a los botones físicos

### Requirement: Un cierre inesperado queda registrado
Si la app se cierra durante un escenario (con el botón PS o por un crash) y se vuelve a abrir, la app SHALL registrar ese escenario con `result=interrupted`, SHALL restaurar `config.cfg` y `lastdir.txt`, SHALL renombrar `suite.txt` a `suite.done.txt` y SHALL arrancar normal, sin continuar la suite.

#### Scenario: Cerrar con PS a mitad de la suite
- **WHEN** a mitad del segundo escenario de una suite, el usuario cierra la app con el botón PS y la vuelve a abrir
- **THEN** `debug.log` tiene una línea `script` con `event=result`, el nombre del segundo escenario y `result=interrupted`, existe `suite.done.txt`, y la app responde a los botones físicos

### Requirement: Al terminar, la app muestra el resultado y sale
Al terminar el último escenario, la app SHALL registrar `event=end` con el número de escenarios que pasaron y fallaron, SHALL restaurar `config.cfg` y `lastdir.txt`, SHALL renombrar `suite.txt` a `suite.done.txt`, reemplazando el que hubiera, SHALL mostrar el resumen en pantalla unos segundos, SHALL asegurarse de que todas las líneas estén en `debug.log` y SHALL cerrarse.

#### Scenario: Suite completa
- **WHEN** el usuario abre la app con una suite de tres escenarios en la que el segundo falla
- **THEN** la app se cierra sola, y `debug.log` tiene tres líneas `script` con `event=result` (`pass`, `fail`, `pass`), y como última línea `script` una con `event=end`, `pass=2` y `fail=1`

### Requirement: Indicador en pantalla
Mientras corre una suite, la app SHALL mostrar sobre la pantalla el nombre del escenario actual, su número y el total, y un aviso de que cualquier botón la detiene.

#### Scenario: Indicador visible
- **WHEN** el usuario abre la app con una suite de dos escenarios
- **THEN** la pantalla muestra "1/2" con el nombre del primer escenario, y después "2/2" con el del segundo

### Requirement: La semilla hace repetible el orden aleatorio
Con `seed <n>`, el orden aleatorio de la cola SHALL depender solo de `n` y del contenido de la cola. Dos escenarios con la misma semilla que mezclan la misma carpeta SHALL obtener el mismo orden.

#### Scenario: Mismo orden con la misma semilla
- **WHEN** una suite tiene dos escenarios idénticos que hacen `seed 42`, activan el aleatorio y reproducen la misma carpeta de cinco pistas, pasando cada pista con R
- **THEN** las líneas `track` con `event=open` de los dos escenarios nombran los mismos archivos en el mismo orden

### Requirement: Las líneas script del log
Durante una suite, el log SHALL registrar, en la categoría `script`:

| Cuándo | Campos |
|---|---|
| al empezar la suite (solo en el primer proceso) | `event=suite`, `scenarios` |
| al empezar cada escenario | `event=scenario`, `name`, `index` (desde 1), `total` |
| al empezar cada paso | `event=step`, `line` (número de línea en `suite.txt`), `cmd` (el texto del comando) |
| al terminar cada escenario | `event=result`, `name`, `result` (`pass`, `fail` o `interrupted`) y, si no pasó, `reason` y, si aplica, `line` y `last` |
| al terminar la suite | `event=end`, `pass`, `fail`, `interrupted` |
| si el usuario la corta | `event=abort`, `reason=user` |
| si el guion tiene un error | `event=error`, `line`, `reason` |
| si el proceso no pudo relanzarse | `event=abort`, `reason` |

#### Scenario: Pasos numerados
- **WHEN** una suite tiene en las líneas 4 y 5 `press CROSS` y `wait screen to=library`
- **THEN** el log tiene una línea `script` con `event=step`, `line=4` y `cmd="press CROSS"`, seguida más adelante de otra con `line=5`
