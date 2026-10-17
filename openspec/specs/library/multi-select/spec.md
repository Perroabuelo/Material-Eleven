# Library Multi-select Specification

## Purpose

Define el modo selección de las listas de la biblioteca: cómo se marcan varias filas a la vez para actuar sobre todas con una sola acción, dónde está disponible, cómo se entra y se sale, el encabezado con la casilla Todo, qué hacen los botones y el toque mientras dura, y cómo convive con la búsqueda.

## Requirements

### Requirement: Dónde hay modo selección
El modo selección SHALL estar disponible en las listas de canciones de la biblioteca (Canciones, Añadidos recientemente y dentro de un artista o de un álbum), en las vistas Artistas y Álbumes, y dentro de una playlist. La pestaña Playlists SHALL NOT ofrecer el modo selección. Con la lista vacía, o con la biblioteca sin contenido, el modo SHALL NOT estar disponible.

#### Scenario: Seleccionar en la vista Álbumes
- **WHEN** el usuario marca un álbum en la vista Álbumes, pulsa Cuadrado y elige "Seleccionar"
- **THEN** la vista entra en modo selección, con una casilla en cada álbum

#### Scenario: Sin modo selección en la pestaña Playlists
- **WHEN** el usuario marca una playlist en la pestaña Playlists y pulsa Cuadrado
- **THEN** el menú no ofrece "Seleccionar"

### Requirement: Entrar en el modo selección
El usuario SHALL poder entrar en el modo selección de dos formas:
- eligiendo "Seleccionar" en el menú de Cuadrado, que entra con ninguna fila marcada y deja la marca de la lista donde estaba;
- manteniendo el dedo sobre una fila, como define `ui/list-navigation`, que entra con esa fila ya marcada y la deja como fila actual.

Entrar en el modo SHALL NOT reproducir nada, entrar en ningún grupo ni cambiar la búsqueda aplicada.

#### Scenario: Entrar desde el menú
- **WHEN** el usuario está en la vista Canciones con la quinta fila marcada, pulsa Cuadrado y elige "Seleccionar"
- **THEN** la vista entra en modo selección sin ninguna fila marcada, el contador del encabezado dice 0 y la quinta fila sigue siendo la actual

#### Scenario: Entrar manteniendo el dedo
- **WHEN** el usuario mantiene el dedo quieto sobre la tercera canción visible durante un segundo y lo levanta
- **THEN** la vista entra en modo selección con esa canción marcada, el contador dice 1 y no empieza a sonar ninguna canción

### Requirement: Marcar y desmarcar filas
En el modo selección, el botón de confirmar sobre una fila, tocarla o mantener el dedo sobre ella SHALL marcarla si no lo estaba y desmarcarla si lo estaba. Ninguna de esas acciones SHALL reproducir una canción ni entrar en un artista, un álbum o una playlist. Cada fila SHALL mostrar una casilla que dice si está marcada. Arriba y abajo SHALL mover la fila actual sin cambiar las marcas. Arrastrar la lista SHALL seguir desplazándola, como fuera del modo.

#### Scenario: Marcar tres canciones con botones
- **WHEN** el usuario está en modo selección en la vista Canciones y pulsa confirmar sobre la primera, baja dos filas, pulsa confirmar, baja una y pulsa confirmar
- **THEN** la primera, la tercera y la cuarta canción muestran su casilla marcada, el contador dice 3 y no suena nada

#### Scenario: Desmarcar con un toque
- **WHEN** en modo selección hay dos canciones marcadas y el usuario toca una de ellas
- **THEN** esa canción queda desmarcada y el contador dice 1

#### Scenario: Confirmar sobre un álbum no entra en él
- **WHEN** el usuario está en modo selección en la vista Álbumes y pulsa confirmar sobre un álbum
- **THEN** el álbum queda marcado y la lista sigue mostrando los álbumes

### Requirement: Encabezado del modo selección
Mientras dura el modo, la barra superior de la biblioteca SHALL reemplazarse por un encabezado fijo que no se desplaza con la lista. El encabezado SHALL contener:
- un control para salir del modo, que tiene el mismo efecto que el botón de volver sin búsqueda aplicada;
- el contador de filas marcadas, que incluye las que la búsqueda oculta;
- la caja de búsqueda, con el término aplicado si lo hay;
- la casilla Todo.

La casilla Todo SHALL calcularse sobre las filas visibles y SHALL tener tres estados: vacía si no hay ninguna visible marcada, parcial si hay algunas y llena si están todas. Activarla SHALL desmarcar todas las filas visibles si estaba llena, y SHALL marcar todas las filas visibles en los otros dos casos. Las marcas de filas que la búsqueda oculta SHALL NOT cambiar al activarla. La casilla SHALL activarse con el botón de confirmar cuando es la posición actual, o al tocarla.

Arriba y abajo SHALL recorrer la casilla Todo como una posición más, antes de la primera fila: una pulsación nueva de arriba sobre la primera fila SHALL llevar a la casilla, y una de arriba sobre la casilla SHALL llevar a la última fila. Abajo sobre la última fila SHALL llevar a la casilla, y abajo sobre la casilla a la primera fila. Mantener el botón SHALL detenerse en la primera o en la última fila, como define `ui/list-navigation`, sin pasar por la casilla. Si no hay filas visibles, la casilla SHALL mostrarse deshabilitada y SHALL NOT poder activarse ni alcanzarse.

#### Scenario: Marcar todo lo visible
- **WHEN** el usuario está en modo selección en la vista Canciones, sin búsqueda y sin nada marcado, sube desde la primera fila hasta la casilla Todo y pulsa confirmar
- **THEN** todas las canciones de la vista quedan marcadas, la casilla se ve llena y el contador dice cuántas canciones tiene la vista

#### Scenario: La casilla llena desmarca lo visible
- **WHEN** todas las filas visibles están marcadas y el usuario toca la casilla Todo
- **THEN** ninguna fila visible queda marcada y la casilla se ve vacía

#### Scenario: Casilla parcial
- **WHEN** en una vista de 10 filas visibles hay 4 marcadas
- **THEN** la casilla Todo se ve en estado parcial, y activarla marca las 10

#### Scenario: Todo no toca lo oculto
- **WHEN** el usuario marcó dos canciones buscando "queen", busca "abba" y activa la casilla Todo dos veces
- **THEN** tras la primera activación el contador suma los resultados de "abba" a las dos de "queen", y tras la segunda vuelve a decir 2

#### Scenario: El recorrido pasa por la casilla
- **WHEN** en modo selección la fila actual es la primera y el usuario pulsa arriba, y luego pulsa arriba otra vez
- **THEN** tras la primera pulsación la posición actual es la casilla Todo, y tras la segunda es la última fila

#### Scenario: Búsqueda sin resultados deshabilita la casilla
- **WHEN** en modo selección el usuario aplica un término que no deja ninguna fila
- **THEN** la casilla Todo se ve deshabilitada y tocarla no cambia el contador

### Requirement: Botones y controles durante el modo
Mientras dura el modo selección:
- Triángulo y tocar la caja de búsqueda SHALL abrir la búsqueda, como fuera del modo, y aplicar un término SHALL conservar todas las marcas.
- El botón de volver SHALL quitar el término si hay uno aplicado, conservando las marcas. Sin término, SHALL salir del modo y descartar todas las marcas.
- Cuadrado SHALL abrir el menú de acciones del modo que define `library/views`.
- L, R y SELECT SHALL NOT hacer nada, y las pestañas de vista SHALL NOT mostrarse ni responder al toque.
- Los controles del mini reproductor, START y el nav rail SHALL seguir funcionando. Ir a otra pantalla con el nav rail SHALL salir del modo y descartar las marcas.

La leyenda SHALL mostrar solo las acciones del modo: marcar con el botón de confirmar, salir o quitar la búsqueda con el botón de volver, buscar con Triángulo y las acciones con Cuadrado.

#### Scenario: Buscar, marcar y seguir marcando
- **WHEN** el usuario, en modo selección en la vista Canciones, busca "queen" y marca dos resultados, busca "abba" y marca tres resultados
- **THEN** el contador dice 5, la lista muestra los resultados de "abba" y las dos canciones de "queen" siguen marcadas aunque no se vean

#### Scenario: Volver quita primero la búsqueda
- **WHEN** en modo selección hay un término aplicado y tres filas marcadas, y el usuario pulsa el botón de volver
- **THEN** la lista se muestra completa, el modo sigue activo y las tres filas siguen marcadas

#### Scenario: Volver sin búsqueda sale del modo
- **WHEN** en modo selección, sin término, hay tres filas marcadas y el usuario pulsa el botón de volver
- **THEN** la vista sale del modo, las casillas desaparecen y, al volver a entrar con "Seleccionar", el contador dice 0

#### Scenario: L y R no cambian de vista
- **WHEN** en modo selección en la vista Canciones el usuario pulsa R
- **THEN** la vista sigue siendo Canciones, en modo selección y con las mismas marcas

#### Scenario: SELECT no lleva a Ajustes
- **WHEN** en modo selección el usuario pulsa SELECT
- **THEN** la pantalla sigue en la biblioteca, en modo selección

#### Scenario: El nav rail descarta la selección
- **WHEN** en modo selección hay filas marcadas, el usuario va a Ajustes con el nav rail y vuelve a la biblioteca
- **THEN** la biblioteca ya no está en modo selección y no hay filas marcadas

#### Scenario: Leyenda del modo
- **WHEN** el usuario entra en modo selección
- **THEN** la leyenda muestra el botón de confirmar junto a "Marcar", el de volver junto a "Salir", Triángulo junto a "Buscar" y Cuadrado junto a "Acciones", y no muestra L y R

### Requirement: Las acciones actúan sobre todo lo marcado
Las acciones del modo SHALL actuar sobre todas las filas marcadas, también las que la búsqueda oculta en ese momento. Una acción que termina bien SHALL salir del modo y descartar las marcas. Una acción que el usuario cancela (cerrar el selector de playlists, descartar el nombre de una playlist nueva o cancelar una confirmación) o que no se completa (nombre vacío o repetido) SHALL dejar el modo activo con las mismas marcas.

#### Scenario: Añadir lo marcado aunque no se vea
- **WHEN** el usuario marca dos canciones buscando "queen", aplica un término que no deja resultados y añade lo marcado a una playlist
- **THEN** las dos canciones de "queen" se añaden a esa playlist

#### Scenario: Cancelar el selector conserva las marcas
- **WHEN** en modo selección hay cuatro filas marcadas, el usuario abre "Añadir a playlist…" y cierra el selector con el botón de volver
- **THEN** la vista sigue en modo selección con las cuatro filas marcadas

#### Scenario: Terminar una acción sale del modo
- **WHEN** el usuario añade a una playlist las cuatro filas que tenía marcadas
- **THEN** aparece el aviso de lo añadido, la vista sale del modo y la lista conserva su búsqueda y su fila actual
