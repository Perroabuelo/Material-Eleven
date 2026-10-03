## Purpose

Permite escuchar música con la pantalla de la consola apagada desde cualquier pantalla de la aplicación, sin que la reproducción se corte, sin que la consola se suspenda mientras suena un track y sin que la pantalla se vuelva a encender sola.

## ADDED Requirements

### Requirement: START apaga la pantalla desde cualquier pantalla
El sistema SHALL apagar la pantalla de la consola cuando el usuario pulsa START en cualquier pantalla de la aplicación, y SHALL NOT cerrar la aplicación por esa pulsación. Apagar la pantalla SHALL NOT pausar, detener ni cambiar la reproducción en curso. La combinación L + R + START que cancela la entrada de texto de la búsqueda SHALL conservar ese significado y SHALL NOT apagar la pantalla.

#### Scenario: START en Carpetas con música sonando
- **WHEN** suena un track, el usuario está en Carpetas y pulsa START
- **THEN** la pantalla se apaga, la música sigue sonando y la aplicación sigue abierta

#### Scenario: START en Biblioteca sin nada cargado
- **WHEN** no hay ningún track cargado, el usuario está en Biblioteca y pulsa START
- **THEN** la pantalla se apaga y, al encenderla de nuevo, la aplicación sigue en Biblioteca

#### Scenario: START en Reproduciendo y en Ajustes
- **WHEN** el usuario pulsa START en Reproduciendo o en Ajustes
- **THEN** la pantalla se apaga y la aplicación sigue en la misma pantalla

#### Scenario: L + R + START durante la búsqueda
- **WHEN** la entrada de texto de la búsqueda está abierta y el usuario pulsa L + R + START
- **THEN** la búsqueda se cancela como antes, y la pantalla no se apaga

### Requirement: La consola no se suspende mientras suena un track
El sistema SHALL impedir que la consola se suspenda por inactividad mientras haya un track cargado que no esté en pausa, esté la pantalla encendida o apagada. Cuando la reproducción está en pausa o no hay ningún track cargado, el sistema SHALL NOT impedir la suspensión automática.

#### Scenario: Escuchar un álbum entero con la pantalla apagada
- **WHEN** el temporizador de ahorro de energía de la consola está en su valor más corto, el usuario empieza a reproducir una cola cuya duración supera ese temporizador y apaga la pantalla con START sin tocar la consola
- **THEN** la música sigue sonando, pasando de un track al siguiente, más allá del tiempo del temporizador y hasta que la cola termina

#### Scenario: En pausa la consola se suspende con normalidad
- **WHEN** el usuario pausa la reproducción, apaga la pantalla y no toca la consola durante más tiempo que el temporizador de ahorro de energía
- **THEN** la consola se suspende como lo haría con cualquier otra aplicación

### Requirement: El botón PS sigue funcionando durante la reproducción
El sistema SHALL NOT bloquear el botón PS para mantener la reproducción. Con un track sonando, con la pantalla encendida o apagada, el botón PS SHALL llevar al menú de la consola como en cualquier otra aplicación.

#### Scenario: Salir al menú de la consola con música sonando
- **WHEN** suena un track y el usuario pulsa el botón PS
- **THEN** la consola muestra su menú, desde donde se puede cerrar la aplicación desde su LiveArea

### Requirement: La pantalla se vuelve a encender con el botón PS
El sistema SHALL dejar apagada la pantalla que apagó START hasta que el usuario la encienda con el botón PS, como lo hace la consola, y SHALL NOT encenderla por su cuenta. Al encenderla, la aplicación SHALL seguir en la misma pantalla en la que estaba, con la reproducción en el estado en que la dejó.

#### Scenario: La pantalla queda apagada
- **WHEN** el usuario apaga la pantalla con START en Carpetas, con música sonando, y no toca la consola
- **THEN** la pantalla sigue apagada, sin volver a encenderse sola, y la música sigue sonando

#### Scenario: Encender con PS en Reproduciendo
- **WHEN** suena un track, el usuario apaga la pantalla con START en Reproduciendo y después pulsa el botón PS
- **THEN** la pantalla se enciende, la aplicación sigue en Reproduciendo y la música sigue sonando
