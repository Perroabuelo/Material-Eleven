# Library Playlists Specification

## Purpose

Define las playlists del usuario: listas de canciones de su biblioteca, con nombre, que se crean y se editan en la consola, se guardan entre sesiones y se reproducen como cola. Cubre cómo se guardan, cómo se crean, renombran y borran, cómo se añaden, quitan y mueven canciones, qué pasa con las canciones que la biblioteca ya no tiene y cómo se presenta cada playlist.

## Requirements

### Requirement: Las playlists se guardan como archivos M3U en la carpeta de datos
El sistema SHALL guardar cada playlist como un archivo `.m3u8` (texto UTF-8) propio dentro de `ux0:data/ElevenMPV/playlists/`. El archivo SHALL contener el nombre visible de la playlist y, por cada canción y en su orden, la ruta absoluta del archivo en la consola junto con su título, su artista y su álbum. El nombre visible SHALL conservarse tal como lo escribió el usuario, aunque contenga caracteres que no admite un nombre de archivo.

Guardar una playlist SHALL ser seguro ante un corte: si la consola se apaga o la aplicación se cierra a mitad de escritura, al volver a abrirla la playlist SHALL estar en su versión anterior o en la nueva, y nunca a medias ni vacía. Un archivo de esa carpeta que no se pueda leer como playlist SHALL ignorarse sin que la aplicación se cierre, y el resto de las playlists SHALL seguir apareciendo.

#### Scenario: Una playlist sobrevive al reinicio
- **WHEN** el usuario crea la playlist "Para correr", le añade tres canciones, cierra la aplicación desde LiveArea y la vuelve a abrir
- **THEN** la pestaña Playlists muestra "Para correr" con sus tres canciones en el mismo orden

#### Scenario: El archivo es un M3U legible
- **WHEN** el usuario crea la playlist "Viaje" con dos canciones y se abre su archivo en `ux0:data/ElevenMPV/playlists/` con un editor de texto
- **THEN** el archivo es texto UTF-8 que empieza con `#EXTM3U`, lleva el nombre "Viaje" y una entrada por canción con su ruta `ux0:`; lo verifica además un test en PC que escribe y vuelve a leer una playlist

#### Scenario: Nombre con caracteres no válidos para un archivo
- **WHEN** el usuario crea una playlist llamada `AC/DC: lo mejor?`
- **THEN** la playlist se guarda y aparece en la pestaña con ese nombre exacto; lo verifica además un test en PC sobre el saneado del nombre de archivo

#### Scenario: Un archivo dañado no rompe la pestaña
- **WHEN** en la carpeta de playlists hay un archivo vacío o con contenido que no es una playlist, además de dos playlists válidas
- **THEN** la pestaña Playlists muestra las dos válidas y la aplicación sigue funcionando; un test en PC cubre la lectura de entradas inválidas

#### Scenario: Guardado interrumpido
- **WHEN** un test en PC simula que la escritura del archivo nuevo falla a mitad de camino
- **THEN** el archivo de la playlist sigue teniendo su contenido anterior completo

### Requirement: Crear una playlist con nombre
El sistema SHALL permitir crear una playlist eligiendo "Nueva playlist…" en el selector de playlists. Elegirla SHALL presentar la entrada de texto del sistema para escribir el nombre, con la asignación de confirmar y cancelar de la consola y con las mismas salidas que la búsqueda de la biblioteca: descartar la entrada, el abandono automático si no llega a presentarse y mantener L, R y START juntos. Confirmar un nombre SHALL crear la playlist y añadirle en el mismo paso lo que el usuario estaba añadiendo.

El nombre SHALL tener al menos un carácter que no sea espacio y como mucho 64 caracteres. Dos playlists SHALL NOT tener el mismo nombre, comparando sin distinguir mayúsculas y minúsculas ni letras con y sin tilde o diéresis. Si el nombre está vacío, la playlist SHALL NOT crearse y no se añade nada. Si el nombre ya existe, la playlist SHALL NOT crearse, no se añade nada y un aviso breve SHALL decir que ya hay una playlist con ese nombre.

#### Scenario: Crear y añadir en un paso
- **WHEN** el usuario pulsa Cuadrado sobre una canción, elige "Añadir a playlist…", elige "Nueva playlist…" y confirma el nombre "Favoritas"
- **THEN** existe la playlist "Favoritas" con esa canción como única entrada, y un aviso breve lo confirma

#### Scenario: Descartar el nombre
- **WHEN** el usuario elige "Nueva playlist…" y descarta la entrada de texto
- **THEN** no se crea ninguna playlist, la lista de la biblioteca sigue como estaba y la reproducción en curso no se interrumpe

#### Scenario: Nombre vacío
- **WHEN** el usuario elige "Nueva playlist…", deja el nombre vacío o solo con espacios y confirma
- **THEN** no se crea ninguna playlist

#### Scenario: Nombre repetido
- **WHEN** existe la playlist "Favoritas" y el usuario intenta crear otra llamada "favoritas"
- **THEN** no se crea una segunda playlist, la canción no se añade a ninguna y un aviso breve dice que ya existe una playlist con ese nombre

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

### Requirement: Renombrar y borrar una playlist
En la pestaña Playlists, el menú de Cuadrado sobre una playlist SHALL ofrecer "Renombrar" y "Borrar".

"Renombrar" SHALL presentar la entrada de texto del sistema con el nombre actual ya cargado, y SHALL seguir las mismas reglas de nombre que al crear: no vacío, como mucho 64 caracteres y sin repetir el de otra playlist. Renombrar SHALL conservar las canciones y su orden. Descartar la entrada SHALL dejar el nombre como estaba.

"Borrar" SHALL pedir confirmación con un diálogo que nombra la playlist y ofrece borrar o cancelar, con cancelar marcado al abrirse. Confirmar SHALL eliminar la playlist y su archivo. Cancelar, el botón de volver o tocar fuera del diálogo SHALL dejarla intacta. Borrar una playlist SHALL NOT borrar ni tocar los archivos de música que contiene, y SHALL NOT interrumpir la reproducción en curso aunque suene desde esa playlist.

#### Scenario: Renombrar
- **WHEN** el usuario pulsa Cuadrado sobre "Viaje", elige "Renombrar" y confirma "Viaje a la costa"
- **THEN** la playlist aparece como "Viaje a la costa", con las mismas canciones en el mismo orden

#### Scenario: Renombrar a un nombre existente
- **WHEN** existen "Viaje" y "Favoritas" y el usuario renombra "Viaje" como "FAVORITAS"
- **THEN** "Viaje" conserva su nombre y un aviso breve dice que ya existe una playlist con ese nombre

#### Scenario: Borrar con confirmación
- **WHEN** el usuario pulsa Cuadrado sobre "Viaje", elige "Borrar" y confirma la opción de borrar del diálogo
- **THEN** "Viaje" desaparece de la pestaña y no vuelve a aparecer tras reiniciar, y las canciones siguen en la biblioteca

#### Scenario: Cancelar el borrado
- **WHEN** el usuario elige "Borrar" sobre una playlist y pulsa confirmar sin mover la marca del diálogo
- **THEN** la playlist sigue existiendo, porque la opción marcada al abrir era cancelar

#### Scenario: Borrar la playlist que suena
- **WHEN** suena una canción reproducida desde "Viaje" y el usuario borra "Viaje"
- **THEN** la canción sigue sonando, y siguiente y anterior siguen recorriendo la cola con la que empezó

### Requirement: Reproducir una playlist
Dentro de una playlist, confirmar o tocar una canción SHALL reproducirla y SHALL tomar como cola la playlist completa, en su orden, empezando por la canción elegida. El ajuste "Al terminar un álbum o artista" SHALL NOT cambiar esta cola. El barajado y la repetición SHALL comportarse como con cualquier otra cola.

La cola SHALL ser una copia de la playlist en el momento de reproducirla: añadir, quitar, mover, renombrar o borrar en esa playlist mientras suena SHALL NOT cambiar la pista en curso ni lo que siguiente y anterior recorren. Los cambios SHALL regir la próxima vez que el usuario reproduzca desde la playlist.

#### Scenario: Reproducir desde la tercera canción
- **WHEN** el usuario entra en una playlist de 6 canciones y reproduce la tercera
- **THEN** suena la tercera, después la cuarta, y al terminar la sexta suena la primera

#### Scenario: Canciones de álbumes distintos
- **WHEN** la playlist contiene canciones de tres álbumes en carpetas distintas y el usuario la reproduce
- **THEN** la pista siguiente es la siguiente de la playlist, y no la siguiente de la carpeta o del álbum de la canción en curso

#### Scenario: El ajuste de álbum no afecta
- **WHEN** el ajuste "Al terminar un álbum o artista" está en "Seguir con el siguiente" y el usuario reproduce la última canción de una playlist y la deja terminar
- **THEN** suena la primera canción de la playlist

#### Scenario: Editar la playlist que suena
- **WHEN** suena la segunda canción de una playlist reproducida desde ella, y el usuario quita la tercera
- **THEN** al terminar la segunda suena la que había sido tercera; y si el usuario vuelve a reproducir la playlist, la quitada ya no está

### Requirement: Canciones que la biblioteca ya no tiene
Una canción de una playlist cuya ruta ya no está en la biblioteca SHALL mostrarse dentro de la playlist atenuada, con el título y el artista que se guardaron al añadirla, y SHALL poder quitarse como cualquier otra. Esto ocurre cuando su archivo se borró o se movió y la biblioteca se reescaneó, o cuando la biblioteca apunta a otra carpeta. El sistema SHALL NOT quitarla de la playlist por su cuenta, para que vuelva a estar disponible si el archivo regresa a su ruta.

Al reproducir, toda canción que no se pueda abrir, atenuada o no, SHALL saltarse como define `playback/queue`. El contador de la playlist SHALL contar todas sus canciones, disponibles o no.

#### Scenario: Archivo borrado y biblioteca reescaneada
- **WHEN** una canción de "Viaje" se borra de la tarjeta desde el PC, el usuario reescanea la biblioteca y abre "Viaje"
- **THEN** la canción aparece atenuada con su título y su artista, y al reproducir "Viaje" se salta sin cortar la reproducción

#### Scenario: El archivo vuelve
- **WHEN** el usuario copia de nuevo ese archivo a la misma ruta y reescanea
- **THEN** la canción vuelve a mostrarse normal en "Viaje" y suena al llegarle el turno

#### Scenario: Quitar una canción atenuada
- **WHEN** el usuario pulsa Cuadrado sobre una canción atenuada y elige "Quitar de la playlist"
- **THEN** la canción sale de la playlist

### Requirement: Presentación de las playlists
La pestaña Playlists SHALL mostrar una fila por playlist, de la modificada más recientemente a la más antigua, con su carátula, su nombre y su cantidad de canciones. Crear una playlist, añadirle o quitarle canciones, mover una canción o renombrarla SHALL contar como modificarla. Reproducirla SHALL NOT contar como modificarla.

La carátula de cada playlist SHALL componerse con las carátulas de los primeros álbumes distintos que aparecen en ella, en su orden, saltando las canciones cuyo álbum no tiene carátula:
- Con cuatro o más álbumes distintos con carátula, SHALL ser un mosaico de dos por dos con las cuatro primeras.
- Con uno a tres, SHALL ser la carátula del primero.
- Sin ninguno, o con la playlist vacía, SHALL ser el marcador por defecto.

La carátula SHALL actualizarse cuando un cambio en la playlist altera cuáles son esos primeros álbumes. Como en el resto de la biblioteca, la presentación de la lista SHALL NOT quedar retenida esperando carátulas.

Las canciones dentro de una playlist SHALL presentarse como las de un álbum: con la carátula de su álbum, título, artista y badge de formato.

Si no hay ninguna playlist, la pestaña SHALL decir que no hay playlists y cómo crear una: con Cuadrado y "Añadir a playlist…" sobre cualquier canción. Si se entra en una playlist vacía, la pantalla SHALL decir que está vacía, en vez de mostrar una lista vacía sin explicación.

#### Scenario: Mosaico de cuatro álbumes
- **WHEN** las primeras canciones de una playlist son de los álbumes A, A, B, C, B y D, y los cuatro tienen carátula
- **THEN** la fila de la playlist muestra un mosaico con las carátulas de A, B, C y D; lo verifica además un test en PC sobre la elección de los álbumes

#### Scenario: Pocos álbumes distintos
- **WHEN** todas las canciones de una playlist son de dos álbumes con carátula
- **THEN** la fila muestra solo la carátula del primero de ellos

#### Scenario: Sin carátulas
- **WHEN** ninguna canción de la playlist tiene carátula de álbum disponible
- **THEN** la fila muestra el marcador por defecto

#### Scenario: El mosaico se actualiza
- **WHEN** el usuario mueve al primer puesto una canción de un álbum que no aparecía en el mosaico
- **THEN** al volver a la pestaña Playlists, el mosaico incluye la carátula de ese álbum

#### Scenario: Orden por última modificación
- **WHEN** existen "Viaje", creada ayer, y "Favoritas", creada hoy, y el usuario añade una canción a "Viaje"
- **THEN** la pestaña muestra "Viaje" primero y "Favoritas" después

#### Scenario: Sin playlists
- **WHEN** el usuario abre la pestaña Playlists sin haber creado ninguna
- **THEN** la pantalla dice que no hay playlists y explica que se crean con Cuadrado y "Añadir a playlist…" sobre una canción

#### Scenario: Recorrer la pestaña con muchas playlists
- **WHEN** hay 20 playlists, cada una con cuatro álbumes distintos con carátula, y el usuario recorre la pestaña de arriba abajo rápidamente
- **THEN** la lista responde sin detenerse, los mosaicos aparecen en cuanto están listos y ninguno parpadea entre el mosaico y el marcador una vez cargado
