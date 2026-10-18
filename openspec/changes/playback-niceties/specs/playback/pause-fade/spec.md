## Purpose

Define el fundido corto con que la reproducción se pausa y se reanuda, para que el sonido no se corte en seco ni haga un clic audible en auriculares.

## ADDED Requirements

### Requirement: Pausar y reanudar con un fundido corto
Cuando el usuario pausa la reproducción, desde cualquier superficie (el botón de confirmar o el botón de reproducir en Reproduciendo, o el del mini reproductor), el sistema SHALL bajar el volumen de lo que suena de forma continua hasta el silencio en unos 100 ms, y solo entonces detener el sonido. Al reanudar, SHALL subir el volumen de forma continua desde el silencio hasta el volumen normal en unos 100 ms. El fundido SHALL estar siempre activo y SHALL NOT ser configurable. El estado de pausa que muestran la pantalla, el mini reproductor y la leyenda SHALL cambiar en el mismo momento de la pulsación, sin esperar a que termine el fundido. Pulsar de nuevo durante un fundido SHALL invertirlo desde el volumen al que había llegado, sin saltos de volumen.

#### Scenario: Pausar sin clic
- **WHEN** el usuario, con auriculares, pausa una pista que suena fuerte y después la reanuda
- **THEN** el sonido se apaga y vuelve a entrar suavemente, sin un clic ni un corte seco, y la pista sigue desde donde se pausó

#### Scenario: La pantalla cambia en el acto
- **WHEN** el usuario pausa una pista en Reproduciendo
- **THEN** el botón de reproducir y la leyenda muestran el estado de pausa en el mismo fotograma de la pulsación

#### Scenario: Pausar y reanudar enseguida
- **WHEN** el usuario pulsa pausa y vuelve a pulsar antes de que pasen 100 ms
- **THEN** la música sigue sonando, con una bajada breve de volumen y sin ningún corte seco

### Requirement: Buscar no dispara el fundido
Buscar una posición con la barra de progreso SHALL NOT producir el fundido de pausa ni de reanudación, y SHALL NOT cambiar si la reproducción está en pausa o sonando.

#### Scenario: Buscar con la música sonando
- **WHEN** el usuario suelta la barra de progreso en otra posición mientras la pista suena
- **THEN** la pista sigue sonando desde la posición nueva, y el estado de pausa no cambia en ningún momento

#### Scenario: Buscar en pausa
- **WHEN** el usuario suelta la barra de progreso en otra posición con la pista en pausa
- **THEN** la pista sigue en pausa en la posición nueva, sin sonar
