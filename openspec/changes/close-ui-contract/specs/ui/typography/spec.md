## Purpose

Fija cómo se dibuja todo el texto de la aplicación para que sea legible en la pantalla física de la consola, se vea igual sin importar por dónde navegó el usuario, y no deje caracteres en blanco cuando los nombres de archivo o los tags vienen en escrituras que la tipografía propia no cubre.

## ADDED Requirements

### Requirement: Texto nítido e independiente del orden de navegación
El sistema SHALL dibujar cada glifo rasterizado al tamaño en que se muestra, sin reescalar a otro tamaño un glifo rasterizado para uno distinto. La definición con que se ve un texto SHALL NOT depender de qué pantalla se abrió primero ni de qué contenido se dibujó antes en la misma sesión.

#### Scenario: El mismo carácter aparece en dos tamaños distintos
- **WHEN** una misma letra aparece a la vez en un texto grande y en uno pequeño de la interfaz
- **THEN** ambas se ven con el mismo nivel de definición de borde, sin que la mayor luzca difusa o con halo

#### Scenario: El orden de navegación no altera la nitidez
- **WHEN** el usuario llega a Now Playing pasando primero por Folders, y en otra sesión llega a Now Playing con otro track y otra ruta de navegación
- **THEN** el título del track se ve con la misma definición en ambos casos

#### Scenario: El título del track es el texto más grande y no el más borroso
- **WHEN** se reproduce un track cuyo título comparte letras con los nombres de archivo ya mostrados en el navegador
- **THEN** el título se dibuja con bordes definidos, no como una ampliación de un texto menor

### Requirement: Escala tipográfica legible en la pantalla de la consola
El sistema SHALL dibujar todo el texto a un tamaño cuyo equivalente físico sobre la pantalla de la consola no sea menor que el tamaño mínimo de etiqueta del lenguaje de diseño que la interfaz imita, y SHALL tomar cada texto de un conjunto cerrado de tamaños nominales en vez de elegir un tamaño por sitio de dibujo.

#### Scenario: Ningún texto por debajo del mínimo
- **WHEN** el usuario mira cualquiera de las tres pantallas
- **THEN** ningún texto queda por debajo del tamaño mínimo, incluidos el reloj, el porcentaje de batería, los badges de formato y los hints de botones físicos

#### Scenario: El mismo rol de texto mide lo mismo en todas las pantallas
- **WHEN** un mismo rol de texto, como el nombre de una entrada de lista, aparece en dos pantallas distintas
- **THEN** se dibuja al mismo tamaño en ambas

#### Scenario: La densidad cede ante la legibilidad
- **WHEN** el alto disponible de una lista no alcanza para mostrar la misma cantidad de entradas que antes al tamaño legible
- **THEN** se muestran menos entradas, y no se reduce el tamaño del texto para que quepan más

### Requirement: Línea base estable, independiente del contenido de la cadena
El sistema SHALL alinear verticalmente el texto a partir de las métricas de la tipografía al tamaño dibujado, de modo que la posición de la línea base no dependa de qué caracteres contiene la cadena concreta.

#### Scenario: Filas con y sin caracteres descendentes
- **WHEN** dos filas consecutivas de una lista muestran nombres, uno con caracteres descendentes y otro sin ellos
- **THEN** ambos textos comparten la misma línea base dentro de su fila

#### Scenario: Un campo que cambia de contenido en el mismo sitio
- **WHEN** el contenido de un campo se actualiza a otro con distinta altura de caracteres, como el tiempo transcurrido al avanzar la reproducción
- **THEN** el texto no se desplaza verticalmente entre una actualización y la siguiente

### Requirement: El texto que no cabe se recorta de forma visible
El sistema SHALL recortar el texto que excede el ancho disponible de su contenedor e indicar que quedó recortado, y SHALL NOT dibujar texto fuera de los límites de su contenedor ni superpuesto a otro elemento de la interfaz.

#### Scenario: Nombre de archivo más largo que su fila
- **WHEN** una entrada de la lista tiene un nombre más largo que el ancho disponible de su fila
- **THEN** el nombre se muestra recortado con una indicación de continuación, sin invadir el badge de formato ni el borde de la fila

#### Scenario: Título o artista más largo que su panel
- **WHEN** el track en reproducción tiene un título o un artista más largo que el panel que lo muestra
- **THEN** el texto se recorta dentro del panel y no se dibuja sobre los elementos vecinos

### Requirement: Cobertura de glifos para contenido no latino
El sistema SHALL dibujar los caracteres de nombres de archivo y de tags que la tipografía propia de la interfaz no cubre, recurriendo a las tipografías que la consola provee, y SHALL NOT omitir en silencio un carácter que la consola puede representar.

#### Scenario: Nombre de archivo en una escritura no latina
- **WHEN** la carpeta actual contiene archivos cuyos nombres están en japonés, chino o cirílico
- **THEN** sus nombres se muestran con sus caracteres dibujados, no en blanco

#### Scenario: Una misma cadena mezcla escrituras
- **WHEN** un título combina caracteres latinos y no latinos en la misma línea
- **THEN** la línea se dibuja completa, con cada carácter representado y sin huecos entre tramos

#### Scenario: Sesión larga sobre contenido no latino
- **WHEN** el usuario navega durante una sesión prolongada por contenido con gran cantidad de caracteres no latinos distintos
- **THEN** los caracteres se siguen dibujando durante toda la sesión, sin que el sistema empiece a omitirlos a medida que avanza
