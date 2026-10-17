## MODIFIED Requirements

### Requirement: Añadir canciones a una playlist
El sistema SHALL permitir añadir a una playlist, desde el menú de Cuadrado de la biblioteca:
- la canción marcada, en cualquier lista de canciones de la biblioteca;
- todas las pistas de un artista o de un álbum, marcándolo en la vista Artistas o Álbumes, en el orden en que se muestran al entrar en él;
- en modo selección, todas las filas marcadas, como define `library/multi-select`.

Lo que se añade desde el modo selección SHALL seguir el orden de la lista completa, sin filtro, y no el orden en que se marcaron las filas. Cada artista o álbum marcado SHALL aportar sus pistas en el orden en que se muestran al entrar en él, en el puesto que ocupa en la lista.

"Añadir a playlist…" SHALL abrir un selector con "Nueva playlist…" en primer lugar y después las playlists existentes, de la modificada más recientemente a la más antigua. El selector SHALL recorrerse con arriba y abajo, SHALL elegir con el botón de confirmar o al tocar una opción, y SHALL cerrarse sin añadir nada con el botón de volver o al tocar fuera de él. Las canciones añadidas SHALL ir al final de la playlist.

Una canción que ya está en la playlist SHALL NOT añadirse otra vez. Tras añadir, un único aviso breve SHALL decir en qué playlist se añadió y cuántas canciones se añadieron, y cuántas se omitieron por estar ya en ella si hubo alguna. Una playlist SHALL admitir como mucho 4000 canciones, el mismo techo que la cola. Si lo que se añade no cabe completo, SHALL añadirse lo que quepa, en orden, y el aviso SHALL decir que la playlist está llena. Si añadir falla a mitad de camino por falta de memoria, el archivo de la playlist SHALL quedar como estaba y un aviso SHALL decir que no se pudo guardar.

#### Scenario: Añadir una canción a una playlist existente
- **WHEN** existen las playlists "Viaje" y "Favoritas", y el usuario pulsa Cuadrado sobre una canción de la vista Canciones, elige "Añadir a playlist…" y luego "Viaje"
- **THEN** la canción queda al final de "Viaje", y un aviso breve dice que se añadió a "Viaje"

#### Scenario: La última usada va arriba
- **WHEN** el usuario acaba de añadir una canción a "Viaje", que no era la primera del selector, y abre "Añadir a playlist…" sobre otra canción
- **THEN** el selector muestra "Nueva playlist…" y justo debajo "Viaje", así que añadir a la misma playlist es pulsar Cuadrado, confirmar "Añadir a playlist…", bajar una vez y confirmar

#### Scenario: Añadir un álbum entero
- **WHEN** el usuario marca un álbum de 10 pistas en la vista Álbumes, pulsa Cuadrado y añade el álbum a "Viaje"
- **THEN** las 10 pistas quedan al final de "Viaje" en el orden del disco, y el aviso dice que se añadieron 10

#### Scenario: Canciones que ya estaban
- **WHEN** "Viaje" ya contiene 3 de las 10 pistas de un álbum y el usuario añade ese álbum a "Viaje"
- **THEN** se añaden solo las 7 que faltaban, sin repetir las otras 3, y el aviso dice que se añadieron 7 y que 3 ya estaban; lo verifica además un test en PC

#### Scenario: Cerrar el selector sin elegir
- **WHEN** el usuario abre el selector de playlists y pulsa el botón de volver
- **THEN** el selector se cierra, ninguna playlist cambia y la lista conserva su selección y su búsqueda

#### Scenario: Playlist llena
- **WHEN** un test en PC añade canciones a una playlist que tiene 3998
- **THEN** quedan 4000, las que no cupieron no se añaden y la operación informa que la playlist se llenó

#### Scenario: Añadir varias canciones marcadas
- **WHEN** el usuario, en modo selección en la vista Canciones, marca la décima, la segunda y la quinta canción, en ese orden, y las añade a "Viaje"
- **THEN** "Viaje" termina con la segunda, la quinta y la décima, en ese orden, y un único aviso dice que se añadieron 3

#### Scenario: Añadir varios álbumes marcados
- **WHEN** el usuario, en modo selección en la vista Álbumes, marca un álbum de 4 pistas y otro de 6 que va antes en la lista, y los añade a una playlist nueva llamada "Mezcla"
- **THEN** "Mezcla" contiene primero las 6 pistas del álbum que va antes, en el orden del disco, y después las 4 del otro, y el aviso dice que se añadieron 10

#### Scenario: Añadir lo marcado tras dos búsquedas
- **WHEN** el usuario, en modo selección en la vista Canciones, marca resultados de "queen" y luego de "abba", y los añade a "Viaje"
- **THEN** todas las marcadas quedan en "Viaje" en el orden de la vista Canciones completa, intercaladas según ese orden

#### Scenario: Falta de memoria al añadir
- **WHEN** un test en PC hace fallar una reserva a mitad de añadir varias tandas de canciones a una playlist
- **THEN** la operación informa el fallo y la playlist guardada no cambia

### Requirement: Quitar y mover canciones dentro de una playlist
Dentro de una playlist, el menú de Cuadrado sobre una canción SHALL ofrecer "Quitar de la playlist" y "Mover".

"Quitar de la playlist" SHALL sacarla de la playlist sin confirmación, guardar el cambio y dejar seleccionada la canción que ocupaba su lugar, o la anterior si era la última.

En modo selección dentro de una playlist, "Quitar de la playlist" SHALL quitar todas las canciones marcadas, disponibles o atenuadas, también las que la búsqueda oculta. Antes de quitarlas SHALL pedir confirmación con un diálogo que dice cuántas canciones se quitarán y de qué playlist, y ofrece quitar o cancelar, con cancelar marcado al abrirse. Cancelar, el botón de volver o tocar fuera del diálogo SHALL dejar la playlist intacta y el modo activo con las mismas marcas. Confirmar SHALL quitarlas, guardar el cambio en una sola escritura, salir del modo, mostrar un aviso breve con cuántas se quitaron y dejar seleccionada la canción que ahora ocupa el lugar de la primera quitada, o la última de la lista si no queda ninguna después.

"Mover" SHALL entrar en un modo en que la canción queda resaltada y arriba y abajo la desplazan un puesto por pulsación dentro de la lista. Confirmar SHALL dejarla en su nuevo puesto y guardar el cambio. El botón de volver SHALL cancelar el movimiento y devolverla a su puesto original. Mientras dura el modo mover, la leyenda SHALL mostrar solo las acciones de ese modo, y L y R, Triángulo, Cuadrado y el toque sobre la lista SHALL NOT hacer nada.

#### Scenario: Quitar una canción
- **WHEN** el usuario está dentro de una playlist de 5 canciones, marca la tercera, pulsa Cuadrado y elige "Quitar de la playlist"
- **THEN** la playlist queda con 4 canciones, la que era cuarta pasa a ser tercera y queda seleccionada, y el cambio sigue ahí tras reiniciar la aplicación

#### Scenario: Mover una canción
- **WHEN** el usuario marca la quinta canción de una playlist, elige "Mover", pulsa arriba tres veces y confirma
- **THEN** esa canción queda segunda, las que estaban entre la segunda y la cuarta bajan un puesto, y el orden nuevo sigue tras reiniciar; lo verifica además un test en PC sobre la operación de mover

#### Scenario: Cancelar un movimiento
- **WHEN** el usuario elige "Mover" sobre una canción, la desplaza dos puestos y pulsa el botón de volver
- **THEN** la canción vuelve a su puesto original y la playlist no cambia

#### Scenario: Los extremos no envuelven
- **WHEN** el usuario mueve la primera canción de una playlist y pulsa arriba
- **THEN** la canción sigue primera

#### Scenario: Quitar varias canciones
- **WHEN** el usuario, en modo selección dentro de una playlist de 10 canciones, marca la segunda, la cuarta y la quinta, elige "Quitar de la playlist" y confirma el diálogo
- **THEN** la playlist queda con 7 canciones en su orden, la que era tercera queda seleccionada en el segundo puesto, un aviso dice que se quitaron 3, la vista sale del modo y el cambio sigue ahí tras reiniciar; lo verifica además un test en PC sobre la operación de quitar varias

#### Scenario: El diálogo abre en cancelar
- **WHEN** el usuario, en modo selección dentro de una playlist con 3 canciones marcadas, elige "Quitar de la playlist" y pulsa confirmar sin mover la marca del diálogo
- **THEN** la playlist no cambia y la vista sigue en modo selección con las 3 canciones marcadas

#### Scenario: Quitar canciones atenuadas
- **WHEN** una playlist tiene dos canciones atenuadas, el usuario las marca en modo selección y las quita confirmando el diálogo
- **THEN** las dos salen de la playlist y las demás quedan en su orden

#### Scenario: Quitar todas
- **WHEN** el usuario, en modo selección dentro de una playlist, activa la casilla Todo sin búsqueda, elige "Quitar de la playlist" y confirma
- **THEN** la playlist queda vacía, sigue existiendo en la pestaña Playlists, y la pantalla dice que está vacía
