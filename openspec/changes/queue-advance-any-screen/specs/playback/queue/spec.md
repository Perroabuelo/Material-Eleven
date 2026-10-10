## ADDED Requirements

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

## MODIFIED Requirements

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
