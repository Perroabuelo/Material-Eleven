# Now Playing Specification

## Purpose

Presents the currently playing track and its transport controls in the app's shared visual language, while preserving every existing playback behavior and not implying queue-management capability the app does not yet have.

## Requirements

### Requirement: Track identity and format display
The system SHALL display the current track's title and artist when embedded metadata provides them, falling back to the filename when it does not (matching existing fallback behavior), plus a badge naming the file's format derived from its extension (for example FLAC, MP3, OPUS, WAV). The badge SHALL show the format's name as legible text in its format family's color over a faint background of that same color, without an outline, the same way the folder browser shows it.

#### Scenario: Metadata available
- **WHEN** a track with embedded title and artist metadata is playing
- **THEN** the screen shows that title and artist, and a badge naming the file's format

#### Scenario: Metadata unavailable
- **WHEN** a track without embedded metadata is playing
- **THEN** the screen shows the filename in place of title and artist, matching current fallback behavior

#### Scenario: El badge de Reproduciendo se lee
- **WHEN** suena un archivo FLAC, y luego uno MP3, y luego uno XM
- **THEN** el badge muestra legible "FLAC" en verde, "MP3" en morado y "XM" en ámbar, igual que esos mismos archivos en Carpetas

### Requirement: Playback transport controls
The system SHALL provide play/pause, previous, next, shuffle-toggle, and repeat-toggle controls. Los controles de barajado y de repetición SHALL ser independientes entre sí: activar uno MUST NOT cambiar el estado del otro. El barajado gobierna el orden en que se recorre la cola; la repetición gobierna qué ocurre cuando la pista termina por sí sola. Cualquier avance o retroceso de pista SHALL respetar el orden vigente, venga del transporte en pantalla, de los gatillos de la consola o del mini reproductor de otra pantalla.

#### Scenario: Toggling shuffle
- **WHEN** el usuario activa el control de barajado
- **THEN** el control queda marcado como activo y las próximas pistas mostradas pasan a ser las del orden barajado, sin cortar la pista en curso

#### Scenario: Toggling repeat
- **WHEN** el usuario activa el control de repetición y la pista en curso termina
- **THEN** esa misma pista vuelve a sonar

#### Scenario: Los dos controles son independientes
- **WHEN** el usuario activa el barajado teniendo la repetición encendida
- **THEN** la repetición sigue encendida, y ambos controles aparecen marcados como activos

#### Scenario: Barajado y repetición a la vez
- **WHEN** el barajado y la repetición están encendidos y el usuario pide la pista siguiente
- **THEN** suena la siguiente pista del orden barajado, en vez de repetirse la actual

#### Scenario: Un salto manual respeta el barajado
- **WHEN** el barajado está encendido y el usuario pide la pista siguiente desde el transporte, desde los gatillos o desde el mini reproductor
- **THEN** suena la siguiente pista del orden barajado, igual que si la pista hubiera terminado por sí sola

### Requirement: Seek control
The system SHALL show the elapsed time and the track's total duration next to the progress control, and SHALL let the user seek by touching or dragging that control. El punto tocado SHALL corresponder a la misma fracción de la pista que ocupa sobre el ancho dibujado de la barra: el borde izquierdo es el inicio, el borde derecho es el final, y el relleno de la barra MUST coincidir con la posición del dedo. La zona táctil de la barra SHALL medir al menos el tamaño táctil mínimo de la app en alto.

Un toque sobre la barra la agarra. Mientras el dedo sigue apoyado, el relleno, un indicador en la posición del dedo y el tiempo transcurrido SHALL seguir la coordenada horizontal del dedo, aunque el dedo se salga de la barra en vertical, y la reproducción MUST NOT cambiar de posición. Al levantar el dedo, la reproducción SHALL saltar una sola vez a la posición del dedo en ese momento, acotada al inicio y al final de la pista. Mientras la barra está agarrada SHALL verse más gruesa y mostrar el indicador. Sin tocar, SHALL verse sin indicador, con el grosor de reposo.

La barra de la vista de letras SHALL comportarse igual. Mientras se arrastra esa barra, las letras MUST NOT moverse con el dedo; se reubican según la nueva posición al soltar.

#### Scenario: Seeking via touch
- **WHEN** con una pista de al menos tres minutos sonando en Reproduciendo, el usuario toca y suelta sin arrastrar el punto medio de la barra
- **THEN** la reproducción salta a la mitad de la pista (con un margen de un par de segundos), y el relleno llega hasta el punto tocado

#### Scenario: El borde derecho lleva al final
- **WHEN** con una pista sonando en Reproduciendo, el usuario toca y suelta el extremo derecho de la barra
- **THEN** el tiempo transcurrido queda a menos de un par de segundos de la duración total, y la pista termina y pasa a la siguiente (o se repite, si la repetición está encendida)

#### Scenario: El borde izquierdo lleva al inicio
- **WHEN** con una pista sonando a más de un minuto de su inicio, el usuario toca y suelta el extremo izquierdo de la barra
- **THEN** el tiempo transcurrido vuelve a 0:00 o 0:01

#### Scenario: Arrastrar muestra el destino y salta al soltar
- **WHEN** con una pista de al menos tres minutos sonando, el usuario apoya el dedo sobre el primer cuarto de la barra, lo arrastra hasta cerca del final sin levantarlo, y lo mantiene quieto un par de segundos
- **THEN** mientras arrastra, la barra se ve más gruesa, el indicador y el relleno siguen al dedo y el tiempo transcurrido muestra el tiempo bajo el dedo, pero el audio sigue sonando desde donde estaba
- **AND** al levantar el dedo, el audio salta a ese punto, y la barra vuelve al grosor de reposo y sin indicador

#### Scenario: El dedo se sale de la barra durante el arrastre
- **WHEN** el usuario agarra la barra, mueve el dedo hacia arriba sobre la carátula o hacia abajo sobre los controles mientras lo desliza a la derecha, y lo levanta ahí
- **THEN** el arrastre continúa hasta soltar, la reproducción salta a la posición horizontal del dedo al soltar, y no se activa ningún control de transporte

#### Scenario: Zona táctil más alta
- **WHEN** el usuario toca un punto unos 25 px por encima o por debajo de la línea de la barra, sin tocar los controles de transporte
- **THEN** el toque agarra la barra igual que si se hubiera tocado sobre la línea

#### Scenario: La barra de la vista de letras se comporta igual
- **WHEN** en la vista de letras de una pista con letras sincronizadas, el usuario arrastra la barra inferior hasta cerca del final y la suelta
- **THEN** mientras arrastra, las letras no se mueven y el tiempo transcurrido sigue al dedo
- **AND** al soltar, el audio salta a ese punto y las letras pasan a la línea que corresponde a la nueva posición

### Requirement: Read-only upcoming-tracks preview
The system SHALL show a short preview of the upcoming track(s) in the order in which they will actually play, whatever the queue was built from and whichever playback order is active, without offering controls to reorder, remove, or otherwise edit that order. La previsualización MUST NOT anunciar un final de la reproducción que la cola no tiene: mientras queden pistas que vayan a sonar, SHALL mostrarlas.

#### Scenario: Preview reflects playlist order
- **WHEN** la cola tiene más de una pista por delante
- **THEN** la pantalla muestra las próximas pistas en el orden en que sonarán, sin ningún control para reordenarlas ni quitarlas

#### Scenario: La previsualización refleja el orden barajado
- **WHEN** el barajado está encendido y quedan pistas por delante
- **THEN** la previsualización muestra las próximas pistas del orden barajado, y no las vecinas de la pista en curso dentro del orden natural

#### Scenario: Encender el barajado cambia lo anunciado en el acto
- **WHEN** el usuario enciende el barajado mientras suena una pista
- **THEN** la previsualización pasa a mostrar las pistas del nuevo orden sin esperar a que la pista en curso termine

#### Scenario: La previsualización refleja una cola venida de la biblioteca
- **WHEN** el usuario inició la reproducción desde una vista de biblioteca y quedan pistas por delante
- **THEN** la previsualización muestra las pistas siguientes de esa vista, y no las de la carpeta en la que está el archivo que suena

#### Scenario: La previsualización refleja una cola venida de una carpeta
- **WHEN** el usuario inició la reproducción desde el navegador de carpetas y quedan pistas por delante
- **THEN** la previsualización muestra las pistas siguientes de esa carpeta, igual que antes de existir la biblioteca

#### Scenario: La última pista del orden no se anuncia como final
- **WHEN** suena la última pista del orden vigente y la reproducción va a continuar por el principio
- **THEN** la previsualización muestra las pistas por las que va a continuar, en vez de afirmar que no quedan más

### Requirement: No dead-end queue-management control
The system SHALL NOT present any control that claims to open a full queue-management view (reordering, removing individual tracks, clearing the queue), since that capability does not exist.

#### Scenario: No control opens a nonexistent screen
- **WHEN** the user views Now Playing
- **THEN** no button or control is shown that would open a queue-management screen

### Requirement: La previsualización muestra la carátula de las próximas pistas
Cada pista de la previsualización de próximas pistas SHALL mostrar la carátula de su álbum cuando la cola vino de la biblioteca y la biblioteca tiene esa carátula guardada. Cuando la cola vino del navegador de carpetas, o la carátula no está disponible, SHALL mostrar el marcador por defecto que muestra hoy. Obtener esas carátulas SHALL NOT retener la pantalla ni el cambio de pista: una carátula que todavía no llegó se muestra con el marcador y aparece en cuanto está disponible.

#### Scenario: Cola de biblioteca con carátulas
- **WHEN** el usuario reproduce desde un álbum de la biblioteca que tiene carátula y quedan pistas por delante
- **THEN** cada pista de la previsualización muestra la carátula de su álbum

#### Scenario: La previsualización cruza a otro álbum
- **WHEN** el ajuste "Al terminar un álbum o artista" está en "Seguir con el siguiente" y suena la penúltima pista de un álbum
- **THEN** la previsualización muestra la última pista con la carátula de ese álbum, y la pista que sigue con la carátula del álbum siguiente

#### Scenario: Cola de carpeta
- **WHEN** el usuario reproduce desde el navegador de carpetas
- **THEN** la previsualización muestra el marcador por defecto en cada pista, igual que antes de este cambio

#### Scenario: Pista sin carátula
- **WHEN** una próxima pista de una cola de biblioteca no tiene carátula disponible
- **THEN** esa pista muestra el marcador por defecto

### Requirement: Vista de letras con toggle
Reproduciendo SHALL ofrecer una vista de letras a pantalla completa, que se abre y se cierra con D-pad arriba. Mientras está abierta, SHALL mostrar en lugar de la carátula grande, el transporte en pantalla y la previsualización de próximas pistas: una cabecera con la carátula en pequeño, el título, el artista y el badge de formato; la letra en el centro; y abajo la barra de progreso con el tiempo transcurrido y el total. El nav rail y la leyenda de botones SHALL seguir visibles. Los botones físicos SHALL hacer en la vista de letras lo mismo que en la vista normal (pausa, pista anterior y siguiente, barajado, repetición, apagar pantalla y volver). La vista elegida SHALL mantenerse al cambiar de pista y al salir y volver a Reproduciendo durante la misma sesión, y la app SHALL arrancar siempre en la vista normal.

#### Scenario: Abrir y cerrar las letras
- **WHEN** el usuario está en Reproduciendo con una pista sonando, pulsa D-pad arriba, y después vuelve a pulsarlo
- **THEN** la primera pulsación muestra la vista de letras con la cabecera, la letra y la barra de progreso, sin cortar la música, y la segunda vuelve a la vista normal tal como estaba

#### Scenario: Los botones siguen funcionando en la vista de letras
- **WHEN** el usuario, con la vista de letras abierta, pulsa el botón de confirmar, luego R, luego Triángulo
- **THEN** la música se pausa, al pulsar R suena la pista siguiente con la vista de letras todavía abierta y mostrando la letra de esa pista, y Triángulo cambia el barajado como en la vista normal

#### Scenario: Volver desde la vista de letras
- **WHEN** el usuario reproduce un archivo desde Carpetas, abre la vista de letras y pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a Carpetas, igual que desde la vista normal

#### Scenario: La vista se recuerda en la sesión
- **WHEN** el usuario abre la vista de letras, va a Ajustes con el nav rail y vuelve a Reproduciendo con el nav rail
- **THEN** Reproduciendo aparece con la vista de letras abierta

#### Scenario: Buscar con la barra de la vista de letras
- **WHEN** el usuario, con la vista de letras abierta, toca la barra de progreso en su punto medio
- **THEN** la canción salta aproximadamente a la mitad de su duración, y al tocar el extremo derecho de la barra salta a los últimos segundos

### Requirement: La letra sincronizada sigue a la canción
Con una letra sincronizada, la vista de letras SHALL resaltar la línea que está sonando (la última cuya marca de tiempo ya pasó) y mostrarla centrada en vertical, con las líneas anteriores y siguientes atenuadas alrededor. Al cambiar la línea que suena, la vista SHALL desplazarse hasta la nueva de forma continua, sin saltos bruscos. Antes de la primera marca, ninguna línea SHALL aparecer resaltada y la primera línea SHALL aparecer centrada. Al pausar, SHALL quedarse en la línea que sonaba; al buscar otro punto de la canción, SHALL pasar a la línea de ese punto. Una línea que no cabe en el ancho de la vista SHALL partirse en varios renglones, sin recortarse.

#### Scenario: La línea resaltada sigue al canto
- **WHEN** el usuario reproduce `01 - i-dle - TOMBOY.flac` con la vista de letras abierta y escucha los primeros veinte segundos
- **THEN** la línea resaltada va cambiando al mismo tiempo que el canto ("Ah-ah-ah-ah-ah", "Yeah, I'm tomboy", "Look at you (you), 넌 못 감당해 날 (날)"…), cada una centrada al resaltarse

#### Scenario: Antes de la primera línea
- **WHEN** el usuario reproduce una pista cuya primera marca está en 00:03 y mira la vista de letras durante los primeros dos segundos
- **THEN** ninguna línea está resaltada y la primera línea aparece centrada

#### Scenario: Buscar mueve la letra
- **WHEN** el usuario, con la vista de letras abierta, toca la barra de progreso cerca del final de la canción
- **THEN** la letra pasa a mostrar resaltada la línea que corresponde a ese punto

#### Scenario: Una línea con texto coreano
- **WHEN** la línea que suena mezcla latín y hangul, como "Look at my toe, 나의 ex 이름 tattoo"
- **THEN** se ve completa y nítida, con todos sus caracteres dibujados

#### Scenario: Una línea más ancha que la vista
- **WHEN** el usuario reproduce `04 linea larga.wav` de las pruebas con la vista de letras abierta
- **THEN** la línea se muestra completa en varios renglones, sin puntos suspensivos ni texto fuera de la vista

### Requirement: La letra se desplaza con el tacto
En la vista de letras, el usuario SHALL poder arrastrar la letra con el dedo para recorrerla, sin pasar de la primera ni de la última línea. Con una letra sincronizada, arrastrar SHALL dejar de seguir a la canción mientras dura el gesto; tres segundos después de soltar, la vista SHALL volver sola, de forma continua, a la línea que está sonando. Arrastrar SHALL NOT cambiar el punto de la canción.

#### Scenario: Recorrer la letra y volver solo
- **WHEN** el usuario, con una letra sincronizada sonando, arrastra la letra hacia arriba hasta ver líneas de más adelante y suelta
- **THEN** la vista se queda donde la dejó, la música sigue sin cambios, y unos tres segundos después la vista vuelve sola a la línea que está sonando

#### Scenario: El arrastre tiene tope
- **WHEN** el usuario arrastra la letra hacia abajo más allá del principio
- **THEN** la primera línea no baja más del centro de la vista

### Requirement: La letra sin tiempos se muestra estática
Con una letra sin tiempos, la vista de letras SHALL mostrar la letra completa desde el principio, sin resaltar ninguna línea, conservando las líneas vacías entre estrofas, y con los encabezados de sección entre corchetes en un tono atenuado. El usuario SHALL poder desplazarla con el tacto, y la vista SHALL NOT moverse sola.

#### Scenario: Letra de Genius embebida
- **WHEN** el usuario reproduce el MP3 de *Golden* de *KPop Demon Hunters* y abre la vista de letras
- **THEN** la letra aparece desde su primera línea, con `[Verse: Rumi, Zoey, Mira, All]` atenuado, y al arrastrarla se desplaza y se queda donde el usuario la deja

### Requirement: Aviso cuando la pista no tiene letra
Si la pista que suena no tiene letra, la vista de letras SHALL mostrar en el centro el aviso "Sin letra" en español o "No lyrics" en inglés, y el resto de la vista (cabecera y barra de progreso) SHALL seguir funcionando.

#### Scenario: Pista sin letra
- **WHEN** el usuario reproduce un archivo de `ux0:/pruebas-eleven/tracker/` y abre la vista de letras, primero con la interfaz en español y después en inglés
- **THEN** la vista muestra "Sin letra" y luego "No lyrics", con la cabecera de la pista y la barra de progreso avanzando

#### Scenario: De una pista con letra a una sin letra
- **WHEN** el usuario, con la vista de letras abierta en una pista con letra, pasa con R a una pista sin letra
- **THEN** la vista muestra el aviso de que no hay letra, sin restos de la letra anterior

### Requirement: La leyenda de Reproduciendo anuncia las letras
La leyenda de botones de Reproduciendo SHALL incluir la entrada de D-pad arriba con la acción "Letras" en español o "Lyrics" en inglés, en las dos vistas.

#### Scenario: Entrada de letras en la leyenda
- **WHEN** el usuario está en Reproduciendo, en la vista normal y luego en la de letras
- **THEN** en ambas la leyenda muestra el chip de D-pad arriba junto a "Letras", sin solaparse con las demás entradas
