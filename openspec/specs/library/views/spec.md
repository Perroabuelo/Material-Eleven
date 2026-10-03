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

#### Scenario: Reproducir desde un álbum
- **WHEN** el usuario abre un álbum en la biblioteca y reproduce una de sus pistas
- **THEN** la pista siguiente es la siguiente del álbum, aunque los archivos del álbum estén repartidos en carpetas distintas

#### Scenario: Reproducir desde el navegador de carpetas
- **WHEN** el usuario reproduce un archivo desde el navegador de carpetas
- **THEN** la cola sigue siendo la carpeta que estaba viendo, igual que antes de existir la biblioteca

#### Scenario: La cola sobrevive al cambio de pantalla
- **WHEN** el usuario reproduce desde una vista de biblioteca y luego se va a otra pantalla usando el nav rail
- **THEN** la reproducción continúa y la pista siguiente sigue saliendo de la cola con la que empezó

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
El sistema SHALL ofrecer desde la propia pantalla de biblioteca las acciones de reescanear la carpeta vigente y de elegir una carpeta distinta, sin que el usuario tenga que buscarlas en otra pantalla.

#### Scenario: Pedir un reescaneo
- **WHEN** el usuario añadió música y quiere verla en la biblioteca
- **THEN** puede lanzar el reescaneo desde la pantalla de biblioteca

#### Scenario: Cambiar de carpeta
- **WHEN** el usuario quiere apuntar la biblioteca a otra carpeta
- **THEN** puede elegirla desde la pantalla de biblioteca

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
El sistema SHALL mostrar en las vistas de biblioteca la carátula del álbum de cada entrada cuando exista, y un marcador por defecto coherente con el resto de la interfaz cuando no exista. La presentación de una lista SHALL NOT quedar retenida esperando a que sus carátulas estén disponibles.

#### Scenario: Álbum con carátula
- **WHEN** el usuario mira la vista de álbumes y alguno de ellos tiene carátula
- **THEN** esa entrada muestra su carátula

#### Scenario: Álbum sin carátula
- **WHEN** el usuario mira una entrada cuya carátula no está disponible
- **THEN** esa entrada muestra el marcador por defecto, y la lista se ve completa igual

#### Scenario: Recorrer una lista larga
- **WHEN** el usuario recorre rápidamente una lista larga de la biblioteca
- **THEN** la lista responde al desplazamiento sin quedarse detenida esperando carátulas

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
