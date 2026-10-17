## MODIFIED Requirements

### Requirement: Reescanear y cambiar de carpeta se piden desde la biblioteca
El sistema SHALL ofrecer desde la propia pantalla de biblioteca las acciones de reescanear la carpeta vigente y de elegir una carpeta distinta, sin que el usuario tenga que buscarlas en otra pantalla. Las dos acciones SHALL reunirse en un menú que se abre con el botón Cuadrado. El menú SHALL recorrerse con arriba y abajo, SHALL ejecutar la opción marcada con el botón de confirmar o al tocarla, y SHALL cerrarse sin hacer nada con el botón de volver o al tocar fuera de él. Mientras el menú está abierto, la lista de debajo SHALL NOT recibir los botones. Reescanear o elegir otra carpeta SHALL quitar cualquier búsqueda aplicada en la biblioteca.

El menú SHALL depender de la fila marcada cuando se abre. Antes de reescanear y elegir carpeta, SHALL ofrecer las acciones que esa fila admite según `library/playlists`, seguidas de "Seleccionar" donde `library/multi-select` ofrece el modo selección:
- Sobre una canción fuera de una playlist: "Añadir a playlist…" y "Seleccionar".
- Sobre un artista o un álbum: añadir sus pistas a una playlist y "Seleccionar".
- Sobre una canción dentro de una playlist: "Quitar de la playlist", "Mover" y "Seleccionar".
- Sobre una playlist en la pestaña Playlists: "Renombrar" y "Borrar".

Al abrirse, la marca del menú SHALL estar en la primera opción. Con la pestaña Playlists vacía, o dentro de una playlist vacía, el menú SHALL ofrecer solo reescanear y elegir carpeta.

En modo selección, el menú SHALL ofrecer solo la acción sobre lo marcado: "Añadir a playlist…" fuera de una playlist y "Quitar de la playlist" dentro de una. SHALL NOT ofrecer reescanear ni elegir carpeta. Sin ninguna fila marcada, la acción SHALL mostrarse deshabilitada y SHALL NOT ejecutarse.

En los estados sin contenido (sin carpeta, sin escanear, carpeta sin música), el botón de confirmar SHALL seguir ejecutando directamente la acción que corresponde a ese estado.

#### Scenario: Pedir un reescaneo
- **WHEN** el usuario añadió música y quiere verla en la biblioteca
- **THEN** puede pulsar Cuadrado, elegir "Reescanear" en el menú y lanzar el reescaneo desde la pantalla de biblioteca

#### Scenario: Cambiar de carpeta
- **WHEN** el usuario quiere apuntar la biblioteca a otra carpeta
- **THEN** puede pulsar Cuadrado, elegir "Elegir carpeta" en el menú y elegirla desde la pantalla de biblioteca

#### Scenario: Cerrar el menú sin hacer nada
- **WHEN** el usuario abre el menú de Cuadrado y pulsa el botón de volver
- **THEN** el menú se cierra, no se reescanea ni se cambia de carpeta, y la lista sigue igual, con su selección y su búsqueda

#### Scenario: Reescanear quita la búsqueda
- **WHEN** el usuario tiene un término aplicado y reescanea desde el menú de Cuadrado
- **THEN** al terminar el escaneo la lista se muestra completa, sin término

#### Scenario: Triángulo ya no reescanea
- **WHEN** la biblioteca tiene contenido y el usuario pulsa Triángulo
- **THEN** se abre la búsqueda y no empieza ningún escaneo

#### Scenario: Confirmar en una biblioteca sin escanear
- **WHEN** la biblioteca tiene carpeta pero nunca se escaneó, y el usuario pulsa el botón de confirmar
- **THEN** empieza el escaneo, igual que antes de este cambio

#### Scenario: El menú sobre una canción
- **WHEN** el usuario marca una canción en la vista Canciones y pulsa Cuadrado
- **THEN** el menú muestra "Añadir a playlist…" marcada en primer lugar, luego "Seleccionar", y debajo "Reescanear" y "Elegir carpeta"

#### Scenario: El menú sobre una canción de una playlist
- **WHEN** el usuario está dentro de una playlist, marca una canción y pulsa Cuadrado
- **THEN** el menú muestra "Quitar de la playlist", "Mover", "Seleccionar", "Reescanear" y "Elegir carpeta", en ese orden

#### Scenario: El menú sobre una playlist
- **WHEN** el usuario marca una playlist en la pestaña Playlists y pulsa Cuadrado
- **THEN** el menú muestra "Renombrar" y "Borrar", y debajo "Reescanear" y "Elegir carpeta"

#### Scenario: El menú en la pestaña Playlists vacía
- **WHEN** no hay playlists, el usuario está en la pestaña Playlists y pulsa Cuadrado
- **THEN** el menú muestra solo "Reescanear" y "Elegir carpeta"

#### Scenario: El menú en modo selección
- **WHEN** el usuario está en modo selección en la vista Álbumes con dos álbumes marcados y pulsa Cuadrado
- **THEN** el menú muestra solo "Añadir a playlist…", y no ofrece reescanear ni elegir carpeta

#### Scenario: Acción deshabilitada sin marcas
- **WHEN** el usuario está en modo selección sin ninguna fila marcada, pulsa Cuadrado y pulsa confirmar sobre "Añadir a playlist…"
- **THEN** la opción se ve deshabilitada, no se abre el selector de playlists y el menú sigue abierto
