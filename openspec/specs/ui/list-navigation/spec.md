# List Navigation Specification

## Purpose

Define cómo se recorre la selección de las listas de la aplicación (Biblioteca, Carpetas, elegir carpeta y Ajustes) con la cruceta, incluida la repetición al mantener un botón, para que recorrer listas largas no exija una pulsación por fila.

## Requirements

### Requirement: Mantener ↑ o ↓ repite el paso en las listas
En la lista de la Biblioteca, en la del navegador de Carpetas, en la de elegir carpeta de la biblioteca y en los ítems de Ajustes, el sistema SHALL mover la selección un paso en cuanto el usuario pulsa ↑ o ↓, y, si el usuario mantiene el botón, SHALL seguir moviéndola sola en la misma dirección. Entre el primer paso y el segundo SHALL haber una pausa de entre 300 y 500 ms; después los pasos SHALL sucederse a un ritmo de entre 10 y 20 por segundo mientras el botón siga mantenido. Al soltar el botón la selección SHALL detenerse en la fila a la que haya llegado.

#### Scenario: Una pulsación corta mueve una sola fila
- **WHEN** el usuario está en la vista Canciones de la Biblioteca con la primera fila seleccionada y pulsa ↓ y lo suelta enseguida (menos de 300 ms)
- **THEN** queda seleccionada la segunda fila, y no la tercera

#### Scenario: Mantener ↓ recorre la lista sola
- **WHEN** el usuario, en una lista de Carpetas con al menos 30 entradas y la primera seleccionada, mantiene ↓ durante 1,5 s y lo suelta
- **THEN** la selección avanza varias filas sin más pulsaciones (entre 10 y 25), y deja de moverse en cuanto se suelta el botón

#### Scenario: Mantener ↑ en Ajustes
- **WHEN** el usuario, en Ajustes, en una categoría con varios ítems y el último seleccionado, mantiene ↑
- **THEN** la selección sube por los ítems sin más pulsaciones

### Requirement: Mantener el botón un rato largo salta por páginas
Si el usuario mantiene ↑ o ↓ en una de esas listas durante más de unos 2 s seguidos, el sistema SHALL pasar a mover la selección una página de filas por paso, donde una página es la cantidad de filas que esa lista muestra a la vez. El salto SHALL volver a ser de una fila en cuanto el usuario suelta el botón y lo vuelve a pulsar.

#### Scenario: Cruzar una lista larga
- **WHEN** el usuario, en la vista Canciones de una biblioteca de al menos 500 canciones y con la primera seleccionada, mantiene ↓ durante 5 s
- **THEN** la selección termina mucho más abajo que el número de pasos dados (más de 100 filas), y el contador de la lista lo refleja

#### Scenario: Soltar devuelve el paso de una fila
- **WHEN** el usuario, tras mantener ↓ hasta que la lista salta por páginas, suelta el botón y pulsa ↓ una vez
- **THEN** la selección avanza exactamente una fila

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

### Requirement: La repetición solo afecta a ↑ y ↓ en las listas
El sistema SHALL NOT repetir ninguna otra acción por mantener un botón: ←, →, L, R, confirmar, volver, △, □, SELECT y START SHALL actuar una sola vez por pulsación, en todas las pantallas. Fuera de las listas, ↑ y ↓ SHALL actuar una sola vez por pulsación; en Reproduciendo, mantener ↑ SHALL alternar la vista de letras una sola vez.

#### Scenario: Mantener ↑ en Reproduciendo
- **WHEN** el usuario, en Reproduciendo, mantiene ↑ durante 2 s
- **THEN** la vista de letras se abre una vez y queda abierta, sin abrirse y cerrarse repetidamente

#### Scenario: Mantener R en la Biblioteca
- **WHEN** el usuario, en la Biblioteca, mantiene R durante 2 s
- **THEN** pasa una sola vez a la vista siguiente

#### Scenario: Mantener confirmar sobre una canción
- **WHEN** el usuario mantiene el botón de confirmar sobre una canción de la Biblioteca durante 2 s
- **THEN** la canción empieza a sonar una sola vez, sin reiniciarse

### Requirement: Las listas se desplazan con el dedo
Las listas de la Biblioteca (en cualquiera de sus vistas y dentro de un artista o un álbum), la lista de Carpetas, la de elegir carpeta de la biblioteca y las listas de Ajustes que no caben en pantalla SHALL desplazarse siguiendo al dedo mientras el usuario lo arrastra en vertical sobre ellas. El desplazamiento SHALL ser continuo, sin saltos de página, y SHALL detenerse en el principio y en el final de la lista sin dejar espacio vacío más allá de ellos. Desplazar la lista SHALL NOT cambiar la fila seleccionada. Una lista que cabe entera en su área SHALL NOT desplazarse.

#### Scenario: Arrastrar recorre la lista
- **WHEN** el usuario abre la vista Canciones con al menos 30 pistas, apoya el dedo sobre la tercera fila visible y lo arrastra hacia arriba unos 200 píxeles sin levantarlo
- **THEN** la lista sube acompañando al dedo, se ven filas que antes estaban debajo, y ninguna pista empieza a sonar

#### Scenario: La selección no cambia al desplazar
- **WHEN** el usuario tiene seleccionada la primera fila de Carpetas, en una carpeta con al menos 20 entradas, y arrastra la lista hasta que esa fila sale de la pantalla
- **THEN** al pulsar Confirmar se abre la primera entrada, la misma que estaba seleccionada antes de arrastrar

#### Scenario: Los extremos detienen el desplazamiento
- **WHEN** el usuario arrastra hacia abajo la lista estando ya al principio, y después la lleva hasta el final y sigue arrastrando hacia arriba
- **THEN** la primera fila no baja de su posición inicial, y la última no sube más allá del borde inferior del área de la lista

#### Scenario: Una lista corta no se mueve
- **WHEN** el usuario arrastra en vertical sobre una lista que cabe entera, como una carpeta con 3 entradas o la lista de categorías de Ajustes
- **THEN** la lista queda quieta

### Requirement: La lista sigue moviéndose después de soltar con impulso
Cuando el usuario levanta el dedo mientras la lista se desplaza con velocidad, la lista SHALL seguir desplazándose en el mismo sentido y SHALL frenar gradualmente hasta detenerse sola. El movimiento SHALL detenerse al llegar al principio o al final de la lista. Apoyar el dedo sobre una lista en movimiento SHALL detenerla en el acto, y ese toque SHALL NOT activar ninguna fila. Pulsar un botón de dirección mientras la lista se mueve SHALL detener el movimiento.

#### Scenario: Un deslizamiento rápido recorre muchas filas
- **WHEN** el usuario, en la vista Canciones con al menos 200 pistas, desliza el dedo rápido hacia arriba y lo levanta
- **THEN** la lista sigue subiendo después de levantar el dedo, va frenando y se detiene sola varias pantallas más abajo

#### Scenario: Tocar una lista en movimiento la detiene
- **WHEN** la lista se está moviendo después de un deslizamiento y el usuario apoya el dedo sobre ella y lo levanta sin arrastrar
- **THEN** la lista se detiene y no empieza a sonar ninguna pista

#### Scenario: El impulso se detiene en el final
- **WHEN** el usuario desliza con fuerza hacia el final de una lista de unas 15 filas
- **THEN** la lista se detiene mostrando la última fila al borde inferior de su área, sin pasarse ni rebotar más allá

#### Scenario: Frenado probado en PC
- **WHEN** un test en PC suelta el desplazamiento con una velocidad inicial y lo hace avanzar fotograma a fotograma
- **THEN** la velocidad disminuye en cada fotograma, el desplazamiento se detiene en un número finito de fotogramas y nunca sale del rango de la lista

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

### Requirement: Los controles de las pantallas se pueden tocar
Todo control que hoy se acciona con un botón en Biblioteca, Carpetas y Ajustes y que se dibuja en pantalla SHALL poder accionarse también con un toque, con un área activa que cumpla el mínimo táctil de `ui/rendering`. Esto incluye: las pestañas de vista de la Biblioteca, las acciones de sus estados vacíos (elegir carpeta, escanear, elegir otra carpeta), la caja de búsqueda y las entradas del menú de carpeta de la Biblioteca, la caja de búsqueda de Carpetas, y las categorías y opciones de Ajustes. Cada toque SHALL tener el mismo efecto que la acción equivalente con botones.

#### Scenario: Tocar una pestaña cambia de vista
- **WHEN** el usuario está en la vista Canciones y toca la pestaña de Artistas en la barra superior
- **THEN** la Biblioteca pasa a la vista Artistas, igual que al pulsar R

#### Scenario: Tocar la acción de un estado vacío
- **WHEN** el usuario abre la Biblioteca sin carpeta elegida y toca la acción de elegir carpeta
- **THEN** se abre la elección de carpeta, igual que al pulsar Confirmar en ese estado

#### Scenario: Tocar una categoría de Ajustes
- **WHEN** el usuario toca la categoría de idioma en la lista de categorías de Ajustes
- **THEN** esa categoría queda marcada y sus opciones aparecen en el área de detalle

#### Scenario: Tocar una opción de Ajustes
- **WHEN** el usuario, con la categoría de idioma abierta, toca la opción English
- **THEN** el idioma cambia a English, igual que al seleccionarla y pulsar Confirmar, y el cambio se guarda

#### Scenario: Tocar una entrada del menú de carpeta
- **WHEN** el usuario abre en la Biblioteca el menú de carpeta y toca "Reescanear"
- **THEN** empieza el reescaneo, igual que al elegir esa entrada con botones

### Requirement: Los botones y el toque conviven sobre la misma lista
Con los botones de dirección, la vista de cada lista SHALL seguir mostrando la fila seleccionada, como antes de este cambio. Si el usuario desplazó la lista con el dedo y la fila seleccionada quedó fuera de la vista, el siguiente botón de dirección SHALL mover la selección desde esa fila, no desde lo que se ve, y la vista SHALL volver a mostrarla. Un toque que activa una fila SHALL dejar esa fila como seleccionada, para que los botones sigan desde ahí. Al volver a una lista (desde Reproduciendo con el botón de volver, o con el nav rail), la fila seleccionada SHALL verse en pantalla.

#### Scenario: Volver a la selección con un botón
- **WHEN** el usuario selecciona la quinta pista con los botones, desplaza la lista con el dedo hasta que la quinta pista sale de la vista y pulsa ↓
- **THEN** la sexta pista queda seleccionada y la vista vuelve a mostrarla

#### Scenario: Los botones siguen desde lo tocado
- **WHEN** el usuario toca un álbum para entrar, vuelve con el botón de volver y pulsa ↓
- **THEN** queda seleccionado el álbum siguiente al que tocó

#### Scenario: La fila elegida se ve al volver de Reproduciendo
- **WHEN** el usuario desplaza la vista Canciones con el dedo hasta la pista 120, la toca, y en Reproduciendo pulsa el botón de volver
- **THEN** la Biblioteca muestra la vista Canciones con la pista 120 seleccionada y visible

#### Scenario: Botón durante el impulso
- **WHEN** la lista se mueve después de un deslizamiento y el usuario pulsa ↑
- **THEN** la lista deja de moverse y la vista muestra la fila que quedó seleccionada

### Requirement: Cada toque lo recibe un solo elemento
Un toque SHALL pertenecer al elemento sobre el que empezó: el nav rail, el mini reproductor, la barra superior o la lista. Un arrastre que empieza en la lista SHALL NOT accionar el nav rail, el mini reproductor ni la barra superior aunque el dedo pase sobre ellos o se levante encima. Un toque que empieza sobre el nav rail, el mini reproductor o la barra superior SHALL NOT desplazar ni activar la lista. Las filas que se desplazan SHALL recortarse en el borde de la barra superior y en el del mini reproductor (o de la leyenda de botones cuando no hay mini reproductor), sin dibujarse encima de ellos, y una parte de fila tapada así SHALL NOT responder al toque.

#### Scenario: Arrastrar sobre el mini reproductor no lo acciona
- **WHEN** suena una pista, el usuario apoya el dedo sobre una fila de la Biblioteca, lo arrastra hacia abajo hasta el botón de pausa del mini reproductor y lo levanta ahí
- **THEN** la lista se desplazó y la reproducción sigue sonando, sin pausarse

#### Scenario: Tocar el mini reproductor no toca la fila de debajo
- **WHEN** suena una pista y el usuario toca el botón de siguiente del mini reproductor en Carpetas, con una fila de la lista desplazada justo detrás
- **THEN** pasa a la pista siguiente y no se abre la entrada de la lista que quedaba detrás

#### Scenario: Las filas no tapan la barra superior
- **WHEN** el usuario desplaza despacio la lista de la Biblioteca hacia arriba
- **THEN** las filas desaparecen al llegar al borde de la barra superior, y el título, las pestañas y la caja de búsqueda se siguen viendo completos

#### Scenario: Arrastrar desde el rail no mueve la lista
- **WHEN** el usuario apoya el dedo sobre el nav rail y lo arrastra en vertical hacia la lista
- **THEN** la lista no se desplaza

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
