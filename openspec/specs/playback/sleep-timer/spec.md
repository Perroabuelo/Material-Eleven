# playback/sleep-timer Specification

## Purpose
Define el temporizador para dormir: detiene la reproducción por su cuenta después de un tiempo elegido o al terminar la pista en curso, con un fundido al final, para que el usuario pueda dormirse escuchando música sin que siga sonando toda la noche.

## Requirements

### Requirement: Opciones del temporizador
El temporizador SHALL ofrecer exactamente estas opciones, en este orden: "Apagado" ("Off"), "15 min", "30 min", "45 min", "60 min", "90 min" y "Al terminar la pista" ("End of track"). Al arrancar la app, el temporizador SHALL estar en "Apagado". Elegir una opción con tiempo SHALL empezar a contar ese tiempo desde el momento de la elección, reemplazando cualquier temporizador que estuviera activo. Elegir "Apagado" SHALL cancelar el temporizador activo sin cambiar la reproducción.

#### Scenario: Elegir 30 minutos
- **WHEN** el usuario elige "30 min" con una pista sonando
- **THEN** el temporizador queda activo, la música sigue sonando sin cambios, y la barra de estado muestra que faltan 30 minutos

#### Scenario: Reemplazar un temporizador activo
- **WHEN** el usuario elige "15 min", espera dos minutos y elige "60 min"
- **THEN** el temporizador cuenta 60 minutos desde la segunda elección, y la barra de estado muestra que faltan 60 minutos

#### Scenario: Cancelar el temporizador
- **WHEN** el usuario elige "15 min" y después elige "Apagado"
- **THEN** el indicador desaparece de la barra de estado y la música sigue sonando pasados los 15 minutos

### Requirement: El tiempo vence con un fundido y una pausa
Cuando vence el tiempo de un temporizador con tiempo, el sistema SHALL bajar el volumen de lo que suena de forma continua hasta el silencio durante los últimos 10 segundos antes del vencimiento, y al vencer SHALL pausar la reproducción y dejar el temporizador en "Apagado". Si en esos 10 segundos cambia la pista, el fundido SHALL continuar en la pista nueva desde el volumen al que había llegado, sin volver al volumen completo. Al reanudar después del vencimiento, la música SHALL sonar con su volumen normal. El temporizador SHALL contar el tiempo también con la reproducción en pausa. Si vence con la reproducción en pausa, SHALL quedar en "Apagado" sin cambiar nada más.

#### Scenario: Vencer 15 minutos
- **WHEN** el usuario reproduce una cola de más de 15 minutos, elige "15 min" y no toca la consola
- **THEN** pasados unos 14 minutos y 50 segundos la música empieza a bajar de volumen, a los 15 minutos queda en pausa, y el indicador desaparece de la barra de estado

#### Scenario: Reanudar después de vencer
- **WHEN** venció un temporizador y el usuario pulsa reanudar
- **THEN** la pista sigue desde donde se pausó, con su volumen normal

#### Scenario: Vencer con la reproducción en pausa
- **WHEN** el usuario elige "15 min", pausa la reproducción enseguida y espera 15 minutos
- **THEN** el temporizador queda en "Apagado" y la reproducción sigue en pausa, en la misma posición

#### Scenario: Cancelar durante el fundido final
- **WHEN** la música ya está bajando de volumen en los últimos 10 segundos de un temporizador y el usuario elige "Apagado"
- **THEN** la música vuelve a su volumen normal y sigue sonando

### Requirement: Modo de fin de pista
Con "Al terminar la pista", cuando la pista en curso termina sola, el sistema SHALL pasar a la pista que toca según las reglas de la cola (la siguiente del orden vigente o la misma si la repetición está encendida), SHALL dejarla cargada en pausa en 0:00 sin que llegue a sonar, y SHALL dejar el temporizador en "Apagado". Si el usuario cambia de pista a mano antes de que termine, el modo SHALL aplicarse a la pista que esté sonando cuando una termine sola. Este modo no tiene fundido.

#### Scenario: La pista termina y la siguiente queda en pausa
- **WHEN** el usuario reproduce la primera pista de una carpeta de tres pistas cortas, elige "Al terminar la pista" y espera a que termine
- **THEN** no suena nada más, la pantalla muestra la segunda pista en pausa en 0:00, y el indicador desaparece de la barra de estado

#### Scenario: Reanudar continúa la cola
- **WHEN** después de que el modo de fin de pista pausó la reproducción, el usuario pulsa reanudar
- **THEN** suena la segunda pista desde el principio, y al terminar suena la tercera

#### Scenario: Con la repetición encendida
- **WHEN** la repetición está encendida, el usuario elige "Al terminar la pista" y la pista en curso termina
- **THEN** la misma pista queda cargada en pausa en 0:00

### Requirement: El temporizador funciona en cualquier pantalla
El temporizador SHALL seguir contando y SHALL vencer igual esté el usuario en Reproduciendo, en Carpetas, en la Biblioteca o en Ajustes, y también con la pantalla apagada con START. Al vencer, la reproducción SHALL quedar en pausa y, como define `playback/screen-off`, la consola SHALL volver a poder suspenderse sola por inactividad.

#### Scenario: Vencer con la pantalla apagada
- **WHEN** el usuario reproduce una carpeta de pistas cortas, elige "Al terminar la pista", vuelve a Carpetas y apaga la pantalla con START
- **THEN** al terminar la pista en curso la música se detiene, y al encender la pantalla con PS Carpetas sigue abierta con el mini reproductor de la pista siguiente en pausa

#### Scenario: Vencer en la Biblioteca
- **WHEN** el usuario elige "15 min" en Reproduciendo, va a la Biblioteca y espera sin tocar nada
- **THEN** a los 15 minutos la música se pausa, y el mini reproductor de la Biblioteca muestra la pista en pausa

### Requirement: Indicador en la barra de estado
Mientras hay un temporizador activo, la barra de estado de las pantallas que la muestran (Reproduciendo, Carpetas y Biblioteca) SHALL mostrar un indicador con una luna dibujada como geometría. Con un temporizador con tiempo, SHALL mostrar junto a la luna los minutos que faltan, redondeados hacia arriba, con la forma "23 min". En el modo de fin de pista, SHALL mostrar junto a la luna "Fin de pista" en español o "End of track" en inglés. Sin temporizador activo, el indicador SHALL NOT aparecer. El indicador SHALL NOT solaparse con la hora ni con la batería.

#### Scenario: Minutos redondeados hacia arriba
- **WHEN** el usuario elige "15 min" y espera 30 segundos
- **THEN** el indicador muestra "15 min"

#### Scenario: Último minuto
- **WHEN** a un temporizador con tiempo le faltan 20 segundos
- **THEN** el indicador muestra "1 min"

#### Scenario: Indicador del modo de fin de pista en los dos idiomas
- **WHEN** el usuario elige "Al terminar la pista" con la interfaz en español, y después cambia el idioma a inglés en Ajustes y vuelve a Carpetas
- **THEN** el indicador muestra la luna con "Fin de pista" y después con "End of track", sin solaparse con la hora ni con la batería

### Requirement: El temporizador no se recuerda entre arranques
El temporizador SHALL durar solo mientras la app sigue abierta. Al cerrar la app y volver a abrirla, SHALL estar en "Apagado", y SHALL NOT guardarse en `config.cfg` ni en ningún otro archivo.

#### Scenario: Reabrir la app
- **WHEN** el usuario elige "60 min", cierra la app desde su LiveArea y la vuelve a abrir
- **THEN** la barra de estado no muestra el indicador, y el menú del temporizador marca "Apagado"
