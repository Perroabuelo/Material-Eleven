## ADDED Requirements

### Requirement: Cambiar de pantalla no acumula memoria
Cambiar de una pantalla de nivel superior a otra (con el nav rail, con un botón físico, al reproducir una canción o al volver de Reproduciendo) SHALL NOT dejar memoria tomada por la pantalla anterior. El usuario SHALL poder cambiar de pantalla cualquier cantidad de veces en una misma sesión sin que la aplicación se cierre ni se degrade por ello. El overlay de debug SHALL mostrar la pila usada por el hilo principal, para poder comprobarlo en la consola.

#### Scenario: La pila no crece al recorrer las pantallas con el nav rail
- **WHEN** el usuario abre el overlay de debug, anota la pila usada que muestra estando en Carpetas, recorre con el nav rail Biblioteca, Ajustes, Reproduciendo y Carpetas, y repite el recorrido diez veces
- **THEN** al volver a Carpetas, la pila usada que muestra el overlay es la misma que la anotada al principio

#### Scenario: La pila no crece al reproducir y volver
- **WHEN** el usuario, con el overlay de debug abierto, reproduce una canción desde Carpetas, vuelve con el botón de volver, reproduce otra desde la Biblioteca, vuelve, y repite esa secuencia diez veces
- **THEN** en Carpetas y en la Biblioteca, la pila usada que muestra el overlay es la misma que antes de la primera reproducción

#### Scenario: La pila no crece con SELECT y volver
- **WHEN** el usuario, con el overlay de debug abierto, pulsa SELECT en Carpetas para abrir Ajustes y pulsa el botón de volver para regresar a Carpetas, veinte veces seguidas
- **THEN** en Carpetas, la pila usada que muestra el overlay es la misma después de la vigésima vuelta que después de la primera

#### Scenario: Una sesión larga de navegación no cierra la aplicación
- **WHEN** el usuario alterna entre Carpetas y Biblioteca con el nav rail durante cinco minutos seguidos, reproduciendo una canción de vez en cuando
- **THEN** la aplicación sigue abierta y responde igual que al principio
