## MODIFIED Requirements

### Requirement: La repetición se detiene en el borde y solo una pulsación nueva da la vuelta
Una pulsación nueva de ↑ sobre la primera fila SHALL llevar a la última, y una de ↓ sobre la última SHALL llevar a la primera, como hasta ahora. En el modo selección de la biblioteca, esa vuelta SHALL pasar por la casilla Todo del encabezado, como define `library/multi-select`. Los pasos repetidos por mantener el botón, en cambio, SHALL detenerse en la primera o en la última fila y SHALL NOT dar la vuelta ni pasar por la casilla Todo. Un salto de página que se pasaría del borde SHALL dejar la selección en esa fila del borde.

#### Scenario: Mantener ↓ se detiene en la última fila
- **WHEN** el usuario mantiene ↓ en una lista de 12 entradas de Carpetas hasta pasado el final y sigue manteniéndolo 2 s más
- **THEN** la selección queda en la última entrada y no vuelve a la primera

#### Scenario: Una pulsación nueva en el borde da la vuelta
- **WHEN** la selección quedó en la última entrada tras mantener ↓, y el usuario suelta el botón y pulsa ↓ una vez
- **THEN** la selección pasa a la primera entrada

#### Scenario: El salto de página no se pasa del borde
- **WHEN** el usuario mantiene ↓ en la vista Canciones hasta que la lista salta por páginas y la última página tiene menos filas que una página completa
- **THEN** la selección termina en la última canción, sin dar la vuelta

#### Scenario: Mantener ↑ en modo selección no sube al encabezado
- **WHEN** el usuario, en modo selección en la vista Canciones con la décima fila como actual, mantiene ↑ durante 3 s
- **THEN** la fila actual queda en la primera canción, y no en la casilla Todo; y una pulsación nueva de ↑ lleva a la casilla Todo

### Requirement: Un toque sobre una fila la elige
Al apoyar el dedo sobre una fila de una lista, esa fila SHALL mostrarse resaltada. Al levantar el dedo, si no llegó a arrastrar, la fila SHALL quedar seleccionada y SHALL activarse de inmediato, con el mismo efecto que seleccionarla y pulsar Confirmar: reproducir una pista, entrar en un artista, un álbum o una carpeta, abrir una categoría de Ajustes o cambiar una opción. En el modo selección de la biblioteca, el toque SHALL marcar o desmarcar la fila, como define `library/multi-select`, en vez de activarla. Si el dedo se mueve lo suficiente para considerarse un arrastre, el resaltado del toque SHALL desaparecer y la fila SHALL NOT activarse. Un toque que empieza en una fila y termina fuera del área de la lista SHALL NOT activar nada.

#### Scenario: Tocar una pista la reproduce
- **WHEN** el usuario, en la vista Álbumes, entra en un álbum y toca la tercera pista
- **THEN** la pista empieza a sonar, la cola es la que define la Biblioteca para ese álbum, y al volver con el botón de volver la tercera pista aparece seleccionada

#### Scenario: Tocar un álbum entra en él
- **WHEN** el usuario toca un álbum en la vista Álbumes
- **THEN** la pantalla muestra las pistas de ese álbum, igual que al seleccionarlo y pulsar Confirmar

#### Scenario: Tocar una carpeta entra en ella
- **WHEN** el usuario toca una carpeta en la lista de Carpetas
- **THEN** la pantalla muestra el contenido de esa carpeta

#### Scenario: Tocar un archivo no reconocido no reproduce
- **WHEN** el usuario toca en Carpetas un archivo cuya extensión no se reconoce para reproducción
- **THEN** no empieza ninguna reproducción

#### Scenario: El resaltado aparece al apoyar el dedo
- **WHEN** el usuario apoya el dedo sobre una fila de la Biblioteca y lo mantiene quieto sin levantarlo
- **THEN** esa fila se ve resaltada mientras el dedo sigue apoyado, y nada empieza a sonar todavía

#### Scenario: Un arrastre no activa la fila donde empezó
- **WHEN** el usuario apoya el dedo sobre una pista, lo arrastra unos 100 píxeles y lo levanta
- **THEN** la lista se desplazó, el resaltado del toque desapareció al empezar el arrastre, y no empieza a sonar ninguna pista

#### Scenario: Toque y arrastre distinguidos en PC
- **WHEN** un test en PC alimenta el reconocedor de gestos con un toque que se mueve menos que el umbral y se levanta, y con otro que lo supera
- **THEN** el primero se informa como toque en su posición de inicio y el segundo como arrastre, sin informar toque

#### Scenario: Tocar en modo selección marca
- **WHEN** el usuario, en modo selección en la vista Álbumes, toca un álbum
- **THEN** el álbum queda marcado y la pantalla sigue mostrando la vista Álbumes

## ADDED Requirements

### Requirement: Mantener el dedo sobre una fila de la biblioteca
En las listas de la biblioteca donde `library/multi-select` ofrece el modo selección, mantener el dedo quieto sobre una fila, sin llegar a arrastrar, durante entre 400 y 700 ms SHALL contar como un toque mantenido, distinto de tocar y de arrastrar. El toque mantenido SHALL actuar en cuanto se cumple el tiempo, con el dedo todavía apoyado: fuera del modo selección, entra en él con esa fila marcada; dentro del modo, marca o desmarca esa fila. Levantar el dedo después SHALL NOT activar la fila ni repetir la acción, y arrastrar después SHALL desplazar la lista como cualquier arrastre.

Un toque que se convierte en arrastre antes de cumplirse el tiempo SHALL NOT contar como toque mantenido. El toque que detiene una lista en movimiento SHALL NOT contar como toque mantenido aunque el dedo siga apoyado. En la pestaña Playlists, mantener el dedo sobre una fila SHALL NOT hacer nada, y levantarlo después SHALL NOT activarla. En Carpetas, en elegir carpeta y en Ajustes, mantener el dedo SHALL comportarse como hasta ahora: levantarlo sin haber arrastrado activa la fila.

#### Scenario: Mantener el dedo entra en el modo
- **WHEN** el usuario, en la vista Canciones, apoya el dedo sobre la segunda fila visible y lo mantiene quieto un segundo
- **THEN** antes de levantar el dedo la vista ya está en modo selección con esa canción marcada; y al levantarlo no empieza a sonar nada

#### Scenario: Mantener el dedo dentro del modo
- **WHEN** en modo selección el usuario mantiene el dedo un segundo sobre una fila marcada y lo levanta
- **THEN** la fila queda desmarcada una sola vez

#### Scenario: Un arrastre lento no es un toque mantenido
- **WHEN** el usuario apoya el dedo sobre una fila y, antes de 400 ms, lo arrastra más allá del umbral de arrastre y lo sigue moviendo
- **THEN** la lista se desplaza y la vista no entra en modo selección

#### Scenario: Detener una lista en movimiento
- **WHEN** la lista se mueve después de un deslizamiento y el usuario apoya el dedo sobre ella y lo mantiene quieto un segundo
- **THEN** la lista se detiene y la vista no entra en modo selección

#### Scenario: Mantener el dedo en la pestaña Playlists
- **WHEN** el usuario mantiene el dedo un segundo sobre una playlist en la pestaña Playlists y lo levanta
- **THEN** no se entra en la playlist ni en ningún modo

#### Scenario: Carpetas no cambia
- **WHEN** el usuario mantiene el dedo un segundo sobre una carpeta en Carpetas y lo levanta sin arrastrar
- **THEN** la pantalla entra en esa carpeta, como antes de este cambio

#### Scenario: Toque mantenido probado en PC
- **WHEN** un test en PC alimenta el reconocedor de gestos con un dedo quieto durante más tiempo que el umbral y luego lo levanta, con otro que se levanta antes del umbral y con otro que arrastra antes del umbral
- **THEN** el primero informa un toque mantenido una sola vez y ningún toque al levantarse, el segundo informa un toque y el tercero informa arrastre sin toque mantenido
