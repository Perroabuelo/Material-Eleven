# Library Views Specification

## Purpose

Define qué ve el usuario cuando abre la biblioteca y qué puede hacer con ella: las vistas por las que recorre su colección sin navegar carpetas, cómo se presentan las pistas cuyos tags no alcanzan para agruparlas, qué ocurre mientras el índice todavía se está construyendo, y el hecho de que reproducir desde una vista convierte esa vista en la cola de reproducción.

## Requirements

### Requirement: La biblioteca presenta la colección por canciones, artistas, álbumes y añadidos recientemente
El sistema SHALL ofrecer en la pantalla de biblioteca cuatro formas de recorrer la colección indexada —todas las canciones, agrupada por artista, agrupada por álbum, y ordenada por lo añadido más recientemente— y SHALL permitir al usuario cambiar entre ellas sin salir de la pantalla.

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

### Requirement: Reproducir desde una vista convierte esa vista en la cola
El sistema SHALL tomar como cola de reproducción la lista de pistas de la vista desde la que el usuario inició la reproducción, en el orden en que esa vista las muestra. Las acciones de pista siguiente y anterior SHALL recorrer esa cola, y no la carpeta en la que se encuentre el archivo reproducido.

Una búsqueda aplicada SHALL NOT cambiar la cola. Si el usuario reproduce una pista desde una lista filtrada, la cola SHALL ser la misma vista sin filtrar, en su orden completo, y la reproducción SHALL empezar por la pista elegida. Es el mismo resultado que elegir esa pista sin haber buscado.

La única excepción es reproducir desde dentro de un álbum o de un artista con el ajuste "Al terminar un álbum o artista" en "Seguir con el siguiente". En ese caso la cola SHALL ser toda la biblioteca:
- Dentro de un álbum: agrupada por álbum, con los álbumes en el orden de la vista Álbumes y las pistas de cada uno en el orden del disco.
- Dentro de un artista: agrupada por artista, con los artistas en el orden de la vista Artistas y las pistas de cada uno en el orden que muestra ese artista.

La reproducción SHALL empezar por la pista elegida. Con el ajuste en "Repetirlo", la cola SHALL ser solo la vista abierta, como hasta ahora. En las vistas Canciones y Añadidos recientemente el ajuste SHALL NOT cambiar nada.

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

### Requirement: La biblioteca dice en qué estado está
El sistema SHALL distinguir en la pantalla de biblioteca los estados en que puede encontrarse —sin carpeta elegida, escaneando, vacía tras un escaneo, y con contenido— y SHALL ofrecer en cada uno la acción que corresponde, en vez de presentar una lista vacía sin explicación.

#### Scenario: Sin carpeta elegida
- **WHEN** el usuario abre la biblioteca sin haber elegido nunca una carpeta
- **THEN** la pantalla se lo dice y le ofrece elegirla

#### Scenario: Escaneo en curso
- **WHEN** hay un escaneo en curso
- **THEN** la pantalla indica que está escaneando y cuánto lleva avanzado, y ofrece abandonarlo

#### Scenario: Carpeta sin música
- **WHEN** el escaneo termina y la carpeta elegida no contenía ninguna pista reproducible
- **THEN** la pantalla se lo dice y le ofrece elegir otra carpeta, en vez de mostrar una lista vacía sin explicación

#### Scenario: Biblioteca truncada
- **WHEN** el escaneo terminó habiendo alcanzado el máximo de pistas indexables
- **THEN** la pantalla indica que la biblioteca quedó truncada

### Requirement: Reescanear y cambiar de carpeta se piden desde la biblioteca
El sistema SHALL ofrecer desde la propia pantalla de biblioteca las acciones de reescanear la carpeta vigente y de elegir una carpeta distinta, sin que el usuario tenga que buscarlas en otra pantalla. Las dos acciones SHALL reunirse en un menú que se abre con el botón Cuadrado. El menú SHALL recorrerse con arriba y abajo, SHALL ejecutar la opción marcada con el botón de confirmar o al tocarla, y SHALL cerrarse sin hacer nada con el botón de volver o al tocar fuera de él. Mientras el menú está abierto, la lista de debajo SHALL NOT recibir los botones. Reescanear o elegir otra carpeta SHALL quitar cualquier búsqueda aplicada en la biblioteca.

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

### Requirement: La biblioteca no reemplaza al navegador de carpetas
El sistema SHALL conservar el navegador de carpetas como camino independiente y completo hacia la reproducción, con el mismo comportamiento que tenía antes de existir la biblioteca. El uso de uno SHALL NOT condicionar ni limitar el uso del otro.

#### Scenario: Ambos caminos siguen disponibles
- **WHEN** el usuario tiene una biblioteca escaneada
- **THEN** sigue pudiendo navegar sus carpetas y reproducir desde ahí archivos que no están en la biblioteca

#### Scenario: Reproducir algo fuera de la carpeta escaneada
- **WHEN** el usuario reproduce desde el navegador de carpetas un archivo que está fuera de la carpeta de escaneo
- **THEN** la reproducción funciona con normalidad, y la biblioteca no se altera

### Requirement: Mini reproductor acoplado en la biblioteca
El sistema SHALL mostrar en la pantalla de biblioteca un mini reproductor persistente con el título y el artista de la pista en curso y sus controles básicos de transporte, sin interrumpir la reproducción. La biblioteca y el navegador de carpetas son dos caminos hacia la reproducción, y el transporte SHALL alcanzarse desde los dos por igual.

#### Scenario: El mini reproductor sigue a la reproducción en la biblioteca
- **WHEN** hay una pista sonando y el usuario recorre la biblioteca
- **THEN** un mini reproductor con su título, su artista y sus controles de transporte permanece visible, y la reproducción sigue sin cortarse

#### Scenario: Sin nada cargado no hay mini reproductor
- **WHEN** el usuario abre la biblioteca y no hay ninguna pista cargada
- **THEN** la pantalla no muestra mini reproductor, y la lista dispone de ese espacio

### Requirement: Las listas de la biblioteca muestran la carátula del álbum
El sistema SHALL mostrar en las vistas de biblioteca la carátula del álbum de cada entrada cuando exista, y un marcador por defecto coherente con el resto de la interfaz cuando no exista. En la vista Artistas, cada artista SHALL mostrar la carátula de la primera de sus pistas, en el orden que muestra ese artista, que tenga carátula disponible. La entrada "Desconocido" de la vista Artistas SHALL mostrar el marcador por defecto. La presentación de una lista SHALL NOT quedar retenida esperando a que sus carátulas estén disponibles.

#### Scenario: Álbum con carátula
- **WHEN** el usuario mira la vista de álbumes y alguno de ellos tiene carátula
- **THEN** esa entrada muestra su carátula

#### Scenario: Álbum sin carátula
- **WHEN** el usuario mira una entrada cuya carátula no está disponible
- **THEN** esa entrada muestra el marcador por defecto, y la lista se ve completa igual

#### Scenario: Recorrer una lista larga
- **WHEN** el usuario recorre rápidamente una lista larga de la biblioteca
- **THEN** la lista responde al desplazamiento sin quedarse detenida esperando carátulas

#### Scenario: Artista con carátula
- **WHEN** el usuario mira la vista Artistas y un artista tiene al menos un álbum con carátula
- **THEN** la fila de ese artista muestra una de esas carátulas, la misma cada vez que vuelve a la vista

#### Scenario: Artista sin ninguna carátula
- **WHEN** ninguna pista de un artista tiene carátula disponible
- **THEN** la fila de ese artista muestra el marcador por defecto

#### Scenario: El cubo de artistas desconocidos
- **WHEN** el usuario mira la entrada "Desconocido" de la vista Artistas
- **THEN** esa fila muestra el marcador por defecto

### Requirement: Las filas de canciones muestran el badge de formato
El sistema SHALL mostrar en cada fila de la biblioteca que representa una canción el badge de formato de su archivo, con la misma presentación que en el navegador de carpetas: el nombre del formato legible, en el color de su familia de formatos, sobre un fondo tenue de ese mismo color. Esto SHALL aplicar a la vista de canciones, a la de añadidos recientemente y a las pistas dentro de un artista o de un álbum. Las filas que representan un artista o un álbum SHALL NOT mostrar badge de formato. El badge SHALL NOT tapar el título ni el artista: si no caben, son ellos los que se recortan antes del badge.

#### Scenario: Canciones de distintos formatos
- **WHEN** el usuario abre la vista de canciones con pistas FLAC, MP3 y MOD en su biblioteca
- **THEN** cada fila muestra el badge de su formato, "FLAC" en verde, "MP3" en morado y "MOD" en ámbar, como esos mismos archivos en Carpetas

#### Scenario: Pistas dentro de un álbum
- **WHEN** el usuario entra en un álbum desde la vista de álbumes
- **THEN** cada pista del álbum muestra el badge de su formato

#### Scenario: Las filas de artista y de álbum no llevan badge
- **WHEN** el usuario mira la vista de artistas o la de álbumes
- **THEN** ninguna de sus filas muestra badge de formato

#### Scenario: Título largo
- **WHEN** una canción tiene un título más largo que el ancho disponible de la fila
- **THEN** el título se recorta antes del badge, y el badge se ve completo

### Requirement: Las pistas de un álbum o de un artista siguen el orden del disco
Dentro de un álbum, el sistema SHALL ordenar las pistas por número de disco, luego por número de pista y luego por título. Una pista sin número de disco SHALL ordenarse como disco 1. Las pistas sin número de pista SHALL ir después de las numeradas de su disco, ordenadas por título. Dentro de un artista, el sistema SHALL agrupar las pistas por álbum, con los álbumes en orden alfabético sin distinguir mayúsculas y las pistas sin álbum al final, y SHALL ordenar cada álbum como se ordena dentro de él. Entre dos pistas que empatan en todo lo anterior, la ruta SHALL decidir, para que el orden sea estable. Las vistas Canciones y Añadidos recientemente SHALL conservar su orden actual.

#### Scenario: Álbum con números de pista
- **WHEN** el usuario abre un álbum cuyas pistas traen los números 1 a 10 y sus títulos no están en orden alfabético
- **THEN** las pistas se muestran y suenan del 1 al 10

#### Scenario: Álbum de dos discos
- **WHEN** el usuario abre un álbum con las pistas 1 a 8 del disco 1 y 1 a 6 del disco 2
- **THEN** se muestran primero las 8 del disco 1 y después las 6 del disco 2, cada grupo en su orden

#### Scenario: Pistas sin número
- **WHEN** un álbum tiene pistas numeradas y otras sin número
- **THEN** las numeradas van primero en su orden y las sin número van al final, por título; lo verifica un test en PC sobre el comparador

#### Scenario: Dentro de un artista
- **WHEN** el usuario abre un artista con dos álbumes, "Beta" y "Alfa", y algunas pistas sin álbum
- **THEN** se muestran las pistas de "Alfa" en su orden, después las de "Beta" en su orden, y al final las que no traen álbum

#### Scenario: La vista Canciones no cambia
- **WHEN** el usuario mira la vista Canciones
- **THEN** las pistas siguen ordenadas por título, aunque traigan número de pista

### Requirement: La biblioteca permite buscar en la lista que se ve
El sistema SHALL permitir al usuario reducir la lista que la biblioteca muestra a las entradas que contienen un término escrito. La búsqueda SHALL abrirse tanto con el botón Triángulo como tocando una caja de búsqueda en la barra superior de la biblioteca, y SHALL presentar la entrada de texto del sistema, con el término vigente ya cargado si la lista tenía uno. La entrada de texto SHALL respetar la asignación de confirmar y cancelar de la consola.

Lo que se compara depende de la lista:
- En Canciones y en Añadidos recientemente, una pista coincide cuando su título, su artista o su álbum contiene el término. El título que se compara es el que muestra la fila: el del tag, o el nombre del archivo si no lo trae.
- En Artistas y en Álbumes, un grupo coincide cuando su nombre contiene el término. El cubo de los desconocidos se compara por la etiqueta que muestra.
- Dentro de un artista o de un álbum, una pista coincide cuando su título contiene el término.

La comparación SHALL NOT distinguir entre mayúsculas y minúsculas ni entre letras con y sin tilde o diéresis, y SHALL tratar la ñ como n. Las entradas que coinciden SHALL conservar el orden que tienen en la lista completa.

El término SHALL pertenecer a la lista en la que se escribió:
- Cambiar de pestaña SHALL borrar los términos de la pestaña que se deja.
- Entrar en un artista o en un álbum SHALL empezar sin término, y volver de él SHALL devolver la lista exterior con el término que tenía.
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

### Requirement: El usuario nunca queda atrapado en la búsqueda de la biblioteca
El sistema SHALL dejar siempre al usuario una salida de la búsqueda, tanto mientras la entrada de texto está abierta como después de aplicar un término. Descartar la entrada de texto SHALL devolver la lista con el término que tenía antes. Confirmar un término vacío SHALL quitar la búsqueda. Si la entrada de texto no llega a presentarse, la aplicación SHALL dejar de esperarla por sí sola y volver a la lista, sin interrumpir la reproducción. Mientras la entrada de texto está abierta, mantener L, R y START juntos SHALL abandonarla.

El botón de volver SHALL deshacer lo más reciente primero: con un término aplicado lo quita y la lista sigue en el mismo nivel. Sin término, sale del artista o del álbum como hasta ahora.

Cuando un término no deja ninguna entrada, la pantalla SHALL decir que no hay resultados para ese término, en vez de mostrar una lista vacía sin explicación. En ese estado el botón de volver SHALL seguir quitando el término.

#### Scenario: Descartar la entrada de texto
- **WHEN** el usuario tiene "rock" aplicado, abre la búsqueda y descarta la entrada de texto
- **THEN** vuelve a la lista filtrada con "rock", y la pantalla responde de nuevo a los botones y al toque

#### Scenario: Confirmar un término vacío
- **WHEN** el usuario tiene un término aplicado, abre la búsqueda, borra el texto y confirma
- **THEN** la lista vuelve a mostrarse completa

#### Scenario: Volver quita primero la búsqueda
- **WHEN** el usuario está dentro de un álbum con un término aplicado y pulsa el botón de volver
- **THEN** se quita el término y sigue viendo el álbum completo; y solo un segundo uso del botón lo lleva a la vista Álbumes

#### Scenario: Búsqueda sin resultados
- **WHEN** el usuario confirma un término que no coincide con ninguna entrada de la lista
- **THEN** la pantalla dice que no hay resultados para ese término, la leyenda ofrece quitar la búsqueda con el botón de volver, y pulsarlo devuelve la lista completa

#### Scenario: Abandonar la entrada de texto con L, R y START
- **WHEN** la entrada de texto está abierta y el usuario mantiene L, R y START juntos
- **THEN** la entrada se cierra, la lista conserva el término que tenía antes y la reproducción en curso sigue sonando
