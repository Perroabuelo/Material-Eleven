## MODIFIED Requirements

### Requirement: La biblioteca presenta la colección por canciones, artistas, álbumes y añadidos recientemente
El sistema SHALL ofrecer en la pantalla de biblioteca cinco formas de recorrer la colección indexada —todas las canciones, agrupada por artista, agrupada por álbum, ordenada por lo añadido más recientemente, y las playlists del usuario— y SHALL permitir al usuario cambiar entre ellas sin salir de la pantalla. La pestaña Playlists SHALL ir a continuación de Añadidos recientemente. Lo que contiene y cómo se presenta lo define `library/playlists`.

#### Scenario: Cambiar de vista
- **WHEN** el usuario está viendo la lista de canciones y elige la agrupación por artista
- **THEN** la pantalla pasa a mostrar los artistas de su colección, sin salir de la biblioteca

#### Scenario: Entrar en un artista
- **WHEN** el usuario elige un artista de la lista de artistas
- **THEN** ve las pistas de ese artista que hay en su biblioteca

#### Scenario: Entrar en un álbum
- **WHEN** el usuario elige un álbum de la lista de álbumes
- **THEN** ve las pistas de ese álbum que hay en su biblioteca

#### Scenario: Lo añadido recientemente
- **WHEN** el usuario elige la vista de añadidos recientemente
- **THEN** ve sus pistas ordenadas de la más reciente a la más antigua según la fecha del archivo

#### Scenario: Llegar a las playlists
- **WHEN** el usuario está en la vista Añadidos recientemente y pulsa R
- **THEN** la pantalla pasa a la pestaña Playlists, sin salir de la biblioteca; y pulsar R otra vez lleva a la vista Canciones

#### Scenario: Entrar en una playlist
- **WHEN** el usuario elige una playlist en la pestaña Playlists
- **THEN** ve sus canciones en el orden de la playlist

### Requirement: Reproducir desde una vista convierte esa vista en la cola
El sistema SHALL tomar como cola de reproducción la lista de pistas de la vista desde la que el usuario inició la reproducción, en el orden en que esa vista las muestra. Las acciones de pista siguiente y anterior SHALL recorrer esa cola, y no la carpeta en la que se encuentre el archivo reproducido. Reproducir desde dentro de una playlist SHALL tomar como cola esa playlist, como define `library/playlists`.

Una búsqueda aplicada SHALL NOT cambiar la cola. Si el usuario reproduce una pista desde una lista filtrada, la cola SHALL ser la misma vista sin filtrar, en su orden completo, y la reproducción SHALL empezar por la pista elegida. Es el mismo resultado que elegir esa pista sin haber buscado.

La única excepción es reproducir desde dentro de un álbum o de un artista con el ajuste "Al terminar un álbum o artista" en "Seguir con el siguiente". En ese caso la cola SHALL ser toda la biblioteca:
- Dentro de un álbum: agrupada por álbum, con los álbumes en el orden de la vista Álbumes y las pistas de cada uno en el orden del disco.
- Dentro de un artista: agrupada por artista, con los artistas en el orden de la vista Artistas y las pistas de cada uno en el orden que muestra ese artista.

La reproducción SHALL empezar por la pista elegida. Con el ajuste en "Repetirlo", la cola SHALL ser solo la vista abierta, como hasta ahora. En las vistas Canciones y Añadidos recientemente, y dentro de una playlist, el ajuste SHALL NOT cambiar nada.

#### Scenario: Reproducir desde un álbum
- **WHEN** el usuario abre un álbum en la biblioteca y reproduce una de sus pistas
- **THEN** la pista siguiente es la siguiente del álbum, aunque los archivos del álbum estén repartidos en carpetas distintas

#### Scenario: Reproducir desde el navegador de carpetas
- **WHEN** el usuario reproduce un archivo desde el navegador de carpetas
- **THEN** la cola sigue siendo la carpeta que estaba viendo, igual que antes de existir la biblioteca

#### Scenario: La cola sobrevive al cambio de pantalla
- **WHEN** el usuario reproduce desde una vista de biblioteca y luego se va a otra pantalla usando el nav rail
- **THEN** la reproducción continúa y la pista siguiente sigue saliendo de la cola con la que empezó

#### Scenario: Repetir el álbum
- **WHEN** el ajuste está en "Repetirlo", el usuario reproduce la última pista de un álbum y la deja terminar
- **THEN** suena la primera pista de ese mismo álbum

#### Scenario: Seguir con el siguiente álbum
- **WHEN** el ajuste está en "Seguir con el siguiente", el usuario reproduce la última pista de un álbum y la deja terminar
- **THEN** suena la primera pista del álbum que sigue en la vista Álbumes, y "A continuación" lo anunciaba antes de que terminara

#### Scenario: Seguir con el siguiente artista
- **WHEN** el ajuste está en "Seguir con el siguiente", el usuario reproduce la última pista de un artista y la deja terminar
- **THEN** suena la primera pista del artista que sigue en la vista Artistas

#### Scenario: Del último grupo vuelve al primero
- **WHEN** el ajuste está en "Seguir con el siguiente" y termina la última pista del último álbum de la vista Álbumes
- **THEN** suena la primera pista del primer álbum de la vista Álbumes

#### Scenario: Seguir con el siguiente y barajar
- **WHEN** el ajuste está en "Seguir con el siguiente", el usuario reproduce desde un álbum y enciende el barajado
- **THEN** las pistas siguientes salen de toda la biblioteca, y no solo de ese álbum

#### Scenario: El ajuste no cambia la vista Canciones
- **WHEN** el ajuste está en "Seguir con el siguiente" y el usuario reproduce desde la vista Canciones
- **THEN** la cola es la vista Canciones en su orden, igual que con "Repetirlo"

#### Scenario: Reproducir un resultado de búsqueda
- **WHEN** el usuario filtra la vista Canciones con un término que deja una sola pista y la reproduce
- **THEN** la pista empieza a sonar, y la pista siguiente es la que le sigue en la vista Canciones completa, no la misma pista otra vez

#### Scenario: Reproducir un resultado dentro de un álbum
- **WHEN** el ajuste está en "Repetirlo", el usuario filtra un álbum de 10 pistas con un término que deja la pista 4 y la reproduce
- **THEN** después de la pista 4 suena la pista 5 del álbum, y al terminar la 10 suena la 1

#### Scenario: Reproducir un resultado dentro de una playlist
- **WHEN** el usuario filtra una playlist de 8 canciones con un término que deja la quinta y la reproduce
- **THEN** después de la quinta suena la sexta de la playlist, y al terminar la octava suena la primera

### Requirement: Reescanear y cambiar de carpeta se piden desde la biblioteca
El sistema SHALL ofrecer desde la propia pantalla de biblioteca las acciones de reescanear la carpeta vigente y de elegir una carpeta distinta, sin que el usuario tenga que buscarlas en otra pantalla. Las dos acciones SHALL reunirse en un menú que se abre con el botón Cuadrado. El menú SHALL recorrerse con arriba y abajo, SHALL ejecutar la opción marcada con el botón de confirmar o al tocarla, y SHALL cerrarse sin hacer nada con el botón de volver o al tocar fuera de él. Mientras el menú está abierto, la lista de debajo SHALL NOT recibir los botones. Reescanear o elegir otra carpeta SHALL quitar cualquier búsqueda aplicada en la biblioteca.

El menú SHALL depender de la fila marcada cuando se abre. Antes de reescanear y elegir carpeta, SHALL ofrecer las acciones que esa fila admite según `library/playlists`:
- Sobre una canción fuera de una playlist: "Añadir a playlist…".
- Sobre un artista o un álbum: añadir sus pistas a una playlist.
- Sobre una canción dentro de una playlist: "Quitar de la playlist" y "Mover".
- Sobre una playlist en la pestaña Playlists: "Renombrar" y "Borrar".

Al abrirse, la marca del menú SHALL estar en la primera opción. Con la pestaña Playlists vacía, o dentro de una playlist vacía, el menú SHALL ofrecer solo reescanear y elegir carpeta.

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
- **THEN** el menú muestra "Añadir a playlist…" marcada en primer lugar, y debajo "Reescanear" y "Elegir carpeta"

#### Scenario: El menú sobre una playlist
- **WHEN** el usuario marca una playlist en la pestaña Playlists y pulsa Cuadrado
- **THEN** el menú muestra "Renombrar" y "Borrar", y debajo "Reescanear" y "Elegir carpeta"

#### Scenario: El menú en la pestaña Playlists vacía
- **WHEN** no hay playlists, el usuario está en la pestaña Playlists y pulsa Cuadrado
- **THEN** el menú muestra solo "Reescanear" y "Elegir carpeta"

### Requirement: La biblioteca permite buscar en la lista que se ve
El sistema SHALL permitir al usuario reducir la lista que la biblioteca muestra a las entradas que contienen un término escrito. La búsqueda SHALL abrirse tanto con el botón Triángulo como tocando una caja de búsqueda en la barra superior de la biblioteca, y SHALL presentar la entrada de texto del sistema, con el término vigente ya cargado si la lista tenía uno. La entrada de texto SHALL respetar la asignación de confirmar y cancelar de la consola.

Lo que se compara depende de la lista:
- En Canciones y en Añadidos recientemente, una pista coincide cuando su título, su artista o su álbum contiene el término. El título que se compara es el que muestra la fila: el del tag, o el nombre del archivo si no lo trae.
- En Artistas y en Álbumes, un grupo coincide cuando su nombre contiene el término. El cubo de los desconocidos se compara por la etiqueta que muestra.
- Dentro de un artista o de un álbum, una pista coincide cuando su título contiene el término.
- En Playlists, una playlist coincide cuando su nombre contiene el término.
- Dentro de una playlist, una canción coincide cuando el título que muestra su fila contiene el término, esté disponible o atenuada.

La comparación SHALL NOT distinguir entre mayúsculas y minúsculas ni entre letras con y sin tilde o diéresis, y SHALL tratar la ñ como n. Las entradas que coinciden SHALL conservar el orden que tienen en la lista completa.

El término SHALL pertenecer a la lista en la que se escribió:
- Cambiar de pestaña SHALL borrar los términos de la pestaña que se deja.
- Entrar en un artista, en un álbum o en una playlist SHALL empezar sin término, y volver de él SHALL devolver la lista exterior con el término que tenía.
- Ir a otra pantalla con el nav rail y volver a la biblioteca SHALL conservar el término y la fila seleccionada.

Mientras hay un término aplicado, la pantalla SHALL mostrarlo en la caja de búsqueda. El contador de la lista SHALL contar las entradas que quedan. La búsqueda SHALL estar disponible solo cuando la biblioteca tiene contenido.

#### Scenario: Buscar una canción por su título
- **WHEN** el usuario está en la vista Canciones, pulsa Triángulo y confirma el término "yellow"
- **THEN** la lista muestra solo las pistas cuyo título, artista o álbum contiene "yellow", en el mismo orden que tenían en la vista completa, y el contador cuenta esas pistas

#### Scenario: Buscar por artista desde Canciones
- **WHEN** el usuario está en la vista Canciones y confirma el nombre de un artista como término
- **THEN** la lista muestra las pistas de ese artista, aunque su título no contenga el término

#### Scenario: Abrir la búsqueda tocando la caja
- **WHEN** el usuario toca la caja de búsqueda de la barra superior de la biblioteca
- **THEN** aparece la entrada de texto del sistema, igual que con Triángulo

#### Scenario: Buscar en la vista Artistas
- **WHEN** el usuario está en la vista Artistas y confirma un término
- **THEN** la lista muestra solo los artistas cuyo nombre contiene el término

#### Scenario: Buscar dentro de un álbum
- **WHEN** el usuario está dentro de un álbum y confirma un término
- **THEN** la lista muestra solo las pistas de ese álbum cuyo título contiene el término

#### Scenario: Buscar una playlist
- **WHEN** existen las playlists "Viaje", "Viaje a la costa" y "Favoritas", y el usuario confirma "viaje" en la pestaña Playlists
- **THEN** la lista muestra "Viaje" y "Viaje a la costa", en el orden que tenían

#### Scenario: Buscar dentro de una playlist
- **WHEN** el usuario está dentro de una playlist y confirma un término
- **THEN** la lista muestra solo las canciones de esa playlist cuyo título contiene el término, en el orden de la playlist

#### Scenario: Sin distinguir tildes ni mayúsculas
- **WHEN** la biblioteca tiene una pista titulada "Canción del Mariachi" y el usuario busca "cancion"
- **THEN** la pista aparece en los resultados; y buscar "CANCIÓN" también la encuentra. Lo verifica además un test en PC sobre la comparación

#### Scenario: El término guardado vuelve a la entrada de texto
- **WHEN** el usuario tiene un término aplicado y vuelve a abrir la búsqueda
- **THEN** la entrada de texto aparece con ese término ya escrito, con sus tildes intactas

#### Scenario: Cambiar de pestaña borra la búsqueda
- **WHEN** el usuario tiene un término aplicado en la vista Canciones y pasa a la vista Artistas con R
- **THEN** la vista Artistas se muestra completa; y al volver a Canciones con L, también Canciones se muestra completa

#### Scenario: Entrar y salir de un artista filtrado
- **WHEN** el usuario filtra la vista Artistas con "beat", entra en uno de los artistas resultantes y vuelve con el botón de volver
- **THEN** dentro del artista la lista aparece completa, y al volver la vista Artistas sigue filtrada con "beat" y con ese artista seleccionado

#### Scenario: La búsqueda sobrevive a un paso por otra pantalla
- **WHEN** el usuario tiene un término aplicado, va a Ajustes con el nav rail y vuelve a la biblioteca con el nav rail
- **THEN** la lista sigue filtrada con ese término y con la misma fila seleccionada

#### Scenario: Sin contenido no hay búsqueda
- **WHEN** la biblioteca no tiene carpeta elegida, o no se ha escaneado, o el escaneo no encontró pistas, y el usuario pulsa Triángulo
- **THEN** no aparece ninguna entrada de texto, y la leyenda no ofrece buscar
