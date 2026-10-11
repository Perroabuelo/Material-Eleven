# Playback Queue Specification

## Purpose

Define qué es la cola de reproducción —la lista de pistas por la que avanzan siguiente y anterior— y en qué orden se recorre, incluido el orden barajado, de modo que cualquier superficie que mueva la reproducción (la pantalla de reproducción, el mini reproductor o una vista de biblioteca) obtenga el mismo comportamiento.

## Requirements

### Requirement: Origen y orden natural de la cola
La cola SHALL recibir su contenido de quien inicia la reproducción —una carpeta del navegador o la biblioteca—, conservando como orden natural de la cola el orden en que el origen entrega sus pistas. La biblioteca entrega la vista desde la que se reprodujo o, cuando el ajuste "Al terminar un álbum o artista" lo pide, toda la biblioteca agrupada por álbum o por artista (ver `library/views`). Rellenar la cola SHALL reemplazarla por completo, sin dejar pistas de la anterior.

#### Scenario: Reproducción iniciada desde una carpeta
- **WHEN** el usuario reproduce un archivo desde el navegador de carpetas
- **THEN** la cola contiene los archivos reproducibles de esa carpeta, en el orden en que la carpeta los muestra

#### Scenario: Reproducción iniciada desde una vista de biblioteca
- **WHEN** el usuario reproduce una pista desde una vista de biblioteca con el ajuste "Al terminar un álbum o artista" en "Repetirlo"
- **THEN** la cola contiene las pistas de esa vista, en el orden de la vista, y no las de la carpeta donde está el archivo

#### Scenario: Reproducción continua desde un álbum
- **WHEN** el usuario reproduce una pista desde dentro de un álbum con el ajuste en "Seguir con el siguiente"
- **THEN** la cola contiene todas las pistas de la biblioteca, agrupadas por álbum en el orden de la vista Álbumes, y la reproducción empieza por la pista elegida

#### Scenario: Una cola nueva sustituye a la anterior
- **WHEN** el usuario inicia la reproducción desde otra carpeta o desde otra vista
- **THEN** la cola pasa a contener únicamente las pistas del nuevo origen

### Requirement: Orden de reproducción y modo barajado
La cola SHALL mantener un orden de reproducción sobre su contenido. Con el barajado apagado, ese orden SHALL ser el orden natural de la cola. Con el barajado encendido, SHALL ser una permutación del contenido completo en la que cada pista aparece exactamente una vez, de modo que ninguna se repita antes de que hayan sonado las demás y ninguna quede fuera del recorrido.

#### Scenario: Barajado apagado
- **WHEN** el barajado está apagado y la reproducción avanza
- **THEN** las pistas suenan en el orden natural de la cola

#### Scenario: Barajado encendido
- **WHEN** el barajado está encendido y la reproducción recorre la cola entera
- **THEN** cada pista de la cola ha sonado exactamente una vez

### Requirement: Estabilidad del orden barajado
El orden barajado SHALL permanecer fijo hasta que el usuario apague el barajado o la cola se rellene desde otro origen. Al llegar al final del orden, la reproducción SHALL continuar por el principio de ese mismo orden, sin generar uno nuevo.

#### Scenario: Lo anunciado es lo que suena
- **WHEN** el barajado está encendido y se ha anunciado cuál es la próxima pista
- **THEN** al avanzar suena esa pista y no otra

#### Scenario: El final del orden continúa por el principio
- **WHEN** la reproducción llega a la última pista del orden barajado y avanza
- **THEN** suena la primera pista de ese mismo orden barajado

### Requirement: Encender y apagar el barajado sin cortar la reproducción
Encender o apagar el barajado SHALL dejar sonando la pista en curso sin interrumpirla. Al encenderlo, la pista en curso SHALL quedar al frente del nuevo orden y el resto SHALL quedar barajado detrás de ella, de modo que lo que viene cambie de inmediato. Al apagarlo, la reproducción SHALL continuar por el vecino natural de la pista en curso.

#### Scenario: Encender el barajado a mitad de una pista
- **WHEN** el usuario enciende el barajado mientras suena una pista
- **THEN** esa pista sigue sonando sin cortarse, y las próximas pistas anunciadas pasan a ser las del orden barajado

#### Scenario: Apagar el barajado a mitad de una pista
- **WHEN** el usuario apaga el barajado mientras suena una pista
- **THEN** esa pista sigue sonando sin cortarse, y la próxima pista pasa a ser la siguiente en el orden natural de la cola

### Requirement: Avanzar y retroceder por el orden vigente
Avanzar y retroceder SHALL moverse por el orden vigente, sea el natural o el barajado, cualquiera que sea la superficie desde la que se pide. Retroceder SHALL devolver a la pista que sonó antes en ese orden. Avanzar más allá del último puesto SHALL continuar por el primero, y retroceder más allá del primero SHALL continuar por el último.

#### Scenario: Retroceder con el barajado encendido
- **WHEN** el barajado está encendido y el usuario retrocede una pista
- **THEN** suena la pista que sonó inmediatamente antes, y no el vecino anterior del orden natural

#### Scenario: Los extremos del orden se recorren de forma circular
- **WHEN** el usuario retrocede estando en la primera pista del orden vigente
- **THEN** suena la última pista de ese orden

### Requirement: Saltar a una pista elegida por el usuario
Cuando el usuario elige una pista concreta de una lista, SHALL sonar esa pista. Si el barajado está encendido, la pista elegida SHALL quedar al frente del orden y el resto de la cola SHALL quedar barajado detrás de ella.

#### Scenario: Elegir una pista con el barajado encendido
- **WHEN** el usuario toca una pista concreta de una carpeta o de una vista de biblioteca con el barajado encendido
- **THEN** suena la pista que tocó, y las siguientes son las del orden barajado que arranca en ella

### Requirement: El fin de una pista avanza la cola desde cualquier pantalla
Cuando la pista en curso termina sola, el sistema SHALL repetirla si la repetición está encendida, o pasar a la siguiente del orden vigente si está apagada. Esto SHALL ocurrir esté el usuario en Reproduciendo, en Carpetas, en la Biblioteca o en Ajustes, y también con la pantalla apagada con START. El paso a la siguiente pista SHALL ocurrir sin que el usuario toque nada, y sin un silencio más largo que un cambio de pista en Reproduciendo.

#### Scenario: La cola sigue en Carpetas
- **WHEN** el usuario reproduce una pista de una carpeta con al menos dos pistas cortas (de menos de un minuto), vuelve a Carpetas con el botón de cancelar y espera sin tocar nada a que termine la pista en curso
- **THEN** empieza la siguiente pista del orden vigente, y el mini reproductor muestra su título

#### Scenario: La cola sigue en la Biblioteca y en Ajustes
- **WHEN** el usuario reproduce una vista de la Biblioteca con al menos tres pistas cortas, vuelve a la Biblioteca, espera a que termine la primera pista, entra en Ajustes por el nav rail y espera a que termine la segunda
- **THEN** suenan las tres pistas una tras otra, y al volver a Reproduciendo se muestra la tercera

#### Scenario: La repetición se respeta fuera de Reproduciendo
- **WHEN** el usuario enciende la repetición en Reproduciendo, va a la Biblioteca por el nav rail y espera a que termine la pista en curso
- **THEN** la misma pista vuelve a sonar desde el principio

#### Scenario: La cola sigue con la pantalla apagada fuera de Reproduciendo
- **WHEN** el usuario reproduce una carpeta con al menos tres pistas cortas, vuelve a Carpetas, apaga la pantalla con START y espera sin tocar nada a que termine la tercera pista
- **THEN** las tres pistas suenan una tras otra, y al encender la pantalla Carpetas sigue abierta con el mini reproductor de la última pista

#### Scenario: Reproduciendo sigue avanzando como antes
- **WHEN** el usuario reproduce una carpeta con al menos tres pistas cortas y se queda en Reproduciendo sin tocar nada
- **THEN** las tres pistas suenan una tras otra, y la pantalla muestra en cada momento la pista que suena

### Requirement: Pistas que no se pueden abrir
Cuando la pista a la que toca avanzar no se puede abrir, la cola SHALL continuar por la siguiente del orden vigente en la misma dirección del movimiento, probando como mucho una vuelta completa. Si ninguna pista de la cola se puede abrir, el sistema SHALL dejar de reproducir y descargar la pista, sin colgarse y sin cerrarse. Si el usuario está en la pantalla de reproducción, el sistema SHALL salir de ella y devolver el control al usuario, sin quedarse en una pantalla sin pista. En cualquier otra pantalla, SHALL quedarse en la pantalla actual, ahora sin mini reproductor. No se muestra ningún mensaje.

#### Scenario: Una pista ilegible no detiene la reproducción
- **WHEN** la pista a la que toca avanzar ya no existe o su decodificador no la abre
- **THEN** se salta y suena la siguiente pista del orden vigente

#### Scenario: Ninguna pista de la cola se puede abrir
- **WHEN** ninguna de las pistas de la cola se puede abrir
- **THEN** el sistema sale de la pantalla de reproducción y devuelve el control al usuario, sin mostrar ningún mensaje

#### Scenario: Ninguna pista abre fuera de Reproduciendo
- **WHEN** termina una pista que se reprodujo desde la Biblioteca, el usuario no está en Reproduciendo, y ni la pista siguiente ni ninguna otra de la cola se puede abrir. Este caso lo cubre un test en PC, porque en la consola no se puede volver ilegible un archivo mientras suena.
- **THEN** la reproducción se detiene y la pista se descarga sin cambiar de pantalla, después de probar como mucho una vuelta completa a la cola

### Requirement: Techo de la cola
La cola SHALL tener un techo declarado de pistas. Cuando el origen trae más pistas de las que caben, SHALL conservar las que cupieron y permitir reproducirlas, en vez de rechazar la reproducción o escribir fuera de lo reservado.

#### Scenario: Un origen más grande que el techo
- **WHEN** el usuario reproduce desde un origen con más pistas de las que admite la cola
- **THEN** la reproducción funciona sobre las pistas que cupieron
