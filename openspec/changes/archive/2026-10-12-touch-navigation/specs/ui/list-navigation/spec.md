## ADDED Requirements

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
Al apoyar el dedo sobre una fila de una lista, esa fila SHALL mostrarse resaltada. Al levantar el dedo, si no llegó a arrastrar, la fila SHALL quedar seleccionada y SHALL activarse de inmediato, con el mismo efecto que seleccionarla y pulsar Confirmar: reproducir una pista, entrar en un artista, un álbum o una carpeta, abrir una categoría de Ajustes o cambiar una opción. Si el dedo se mueve lo suficiente para considerarse un arrastre, el resaltado del toque SHALL desaparecer y la fila SHALL NOT activarse. Un toque que empieza en una fila y termina fuera del área de la lista SHALL NOT activar nada.

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
