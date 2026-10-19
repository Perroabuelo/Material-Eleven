## MODIFIED Requirements

### Requirement: Avanzar y retroceder por el orden vigente
Avanzar y retroceder SHALL moverse por el orden vigente, sea el natural o el barajado, cualquiera que sea la superficie desde la que se pide. Retroceder SHALL devolver a la pista que sonó antes en ese orden. Avanzar más allá del último puesto SHALL continuar por el primero, y retroceder más allá del primero SHALL continuar por el último.

Cuando la pista en curso ya lleva más de 3 segundos sonando, retroceder SHALL reiniciarla desde el principio en vez de pasar a la anterior, sin mover la cola. Con 3 segundos o menos, retroceder SHALL pasar a la pista anterior como se describe arriba. El umbral SHALL ser el mismo desde todas las superficies (los gatillos de la consola, el botón de anterior en pantalla y el del mini reproductor) y SHALL aplicarse también con la repetición encendida. Reiniciar la pista SHALL NOT cambiar si está en pausa o sonando.

#### Scenario: Retroceder con el barajado encendido
- **WHEN** el barajado está encendido, la pista en curso lleva 3 segundos o menos y el usuario retrocede una pista
- **THEN** suena la pista que sonó inmediatamente antes, y no el vecino anterior del orden natural

#### Scenario: Los extremos del orden se recorren de forma circular
- **WHEN** el usuario retrocede estando en la primera pista del orden vigente, con 3 segundos o menos de esa pista
- **THEN** suena la última pista de ese orden

#### Scenario: Retroceder a mitad de una pista la reinicia
- **WHEN** el usuario reproduce la segunda pista de una carpeta, espera 5 segundos y pulsa L en Reproduciendo
- **THEN** la misma pista vuelve a sonar desde 0:00, la cola no cambia de puesto, y la próxima pista anunciada sigue siendo la misma

#### Scenario: Retroceder al principio de una pista pasa a la anterior
- **WHEN** el usuario reproduce la segunda pista de una carpeta y pulsa L antes de que pase un segundo
- **THEN** suena la primera pista de la carpeta

#### Scenario: Dos retrocesos seguidos
- **WHEN** el usuario reproduce la segunda pista de una carpeta, espera 5 segundos y pulsa L dos veces seguidas, con menos de un segundo entre las dos
- **THEN** la primera pulsación reinicia la segunda pista, y la segunda pulsación pasa a la primera pista de la carpeta

#### Scenario: El mini reproductor sigue la misma regla
- **WHEN** el usuario reproduce una pista, vuelve a Carpetas, espera 5 segundos y toca el botón de anterior del mini reproductor
- **THEN** la misma pista vuelve a sonar desde 0:00, y el mini reproductor sigue mostrando su título

#### Scenario: El botón de anterior en pantalla sigue la misma regla
- **WHEN** el usuario reproduce una pista, espera 5 segundos y toca el botón de anterior del transporte de Reproduciendo
- **THEN** la misma pista vuelve a sonar desde 0:00

#### Scenario: Con la repetición encendida
- **WHEN** el usuario enciende la repetición, espera 5 segundos en la pista en curso y pulsa L
- **THEN** la misma pista vuelve a sonar desde 0:00

#### Scenario: Reiniciar en pausa no reanuda
- **WHEN** el usuario pausa una pista que lleva más de 3 segundos y pulsa L
- **THEN** la barra de progreso vuelve a 0:00 y la pista sigue en pausa, sin sonar
