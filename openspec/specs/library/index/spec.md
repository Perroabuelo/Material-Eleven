# Library Index Specification

## Purpose

Define de dónde sale la biblioteca de música y qué garantiza: qué carpeta se escanea, qué se indexa de cada pista, cómo se resuelve la ausencia de tags, cuándo se rehace el índice y cómo se comporta el escaneo frente a colecciones grandes, archivos ilegibles y música que el usuario añadió o borró desde la consola o el PC.

## Requirements

### Requirement: Carpeta de escaneo elegida por el usuario y recordada
El sistema SHALL permitir al usuario elegir una carpeta de escaneo, SHALL recordarla entre sesiones, y SHALL admitir exactamente una carpeta a la vez. Cuando la carpeta recordada ya no existe, el sistema SHALL informarlo y ofrecer elegir otra, en vez de fallar o escanear una ruta distinta por su cuenta.

#### Scenario: Primera vez, sin carpeta elegida
- **WHEN** el usuario abre la biblioteca sin haber elegido nunca una carpeta
- **THEN** la pantalla le ofrece elegir una carpeta, y no escanea nada por su cuenta

#### Scenario: La carpeta elegida sobrevive al reinicio
- **WHEN** el usuario elige una carpeta, cierra la aplicación y vuelve a abrirla
- **THEN** la biblioteca sigue asociada a esa misma carpeta, sin volver a preguntar

#### Scenario: Elegir una carpeta distinta reemplaza a la anterior
- **WHEN** el usuario elige una carpeta distinta de la que tenía
- **THEN** la biblioteca pasa a reflejar solo la carpeta nueva, y ninguna pista de la anterior permanece en el índice

#### Scenario: La carpeta recordada ya no existe
- **WHEN** el usuario abre la biblioteca y la carpeta recordada ya no está disponible, por ejemplo porque retiró la tarjeta de memoria
- **THEN** la aplicación se lo informa y le ofrece elegir otra carpeta, sin borrar el índice que ya tenía y sin escanear otra ruta

### Requirement: El escaneo recorre la carpeta completa e indexa solo lo reproducible
El sistema SHALL recorrer recursivamente la carpeta de escaneo, incluidas sus subcarpetas a cualquier profundidad, y SHALL incorporar al índice únicamente los archivos con una extensión reconocida para reproducción, las mismas que la aplicación reconoce al abrir un archivo desde el navegador de carpetas.

#### Scenario: Música repartida en subcarpetas anidadas
- **WHEN** la carpeta elegida contiene música dentro de subcarpetas, y esas a su vez dentro de otras
- **THEN** todas esas pistas quedan en el índice, sin que el usuario tenga que entrar a ninguna carpeta

#### Scenario: Los archivos no reproducibles no entran
- **WHEN** la carpeta elegida contiene archivos que la aplicación no puede reproducir, junto a los que sí
- **THEN** el índice contiene solo los reproducibles, y el usuario no ve los demás en ninguna vista de biblioteca

#### Scenario: El escaneo no altera dónde está parado el navegador de carpetas
- **WHEN** el usuario escanea una carpeta distinta de aquella en la que tiene parado el navegador de carpetas
- **THEN** el navegador de carpetas sigue mostrando la misma carpeta que mostraba antes del escaneo

### Requirement: Cada pista indexada lleva identidad, ubicación y tags
El sistema SHALL guardar para cada pista indexada su ruta, su extensión, su tamaño, su fecha de modificación, y su título, artista, álbum, número de pista y número de disco cuando el formato los provea. El sistema SHALL derivar el título de la pista del formato de origen: de los tags embebidos cuando existen, del nombre interno del módulo en los formatos de tracker, y del nombre de archivo cuando no hay ninguna de las dos cosas. Un número de pista o de disco escrito como "n/total" SHALL guardarse como n. Un número ausente, vacío o que no empieza por un dígito SHALL guardarse como ausente.

#### Scenario: Pista con tags completos
- **WHEN** se indexa un archivo cuyos tags embebidos traen título, artista y álbum
- **THEN** la pista aparece en la biblioteca con ese título, ese artista y ese álbum

#### Scenario: Archivo con tags vacíos o ausentes
- **WHEN** se indexa un archivo de un formato que admite tags pero que no los trae
- **THEN** la pista aparece con su nombre de archivo como título

#### Scenario: Módulo de tracker
- **WHEN** se indexa un módulo de tracker que lleva un nombre interno
- **THEN** la pista aparece con ese nombre interno como título, y no con su nombre de archivo

#### Scenario: Formato sin metadatos de ninguna clase
- **WHEN** se indexa un archivo de un formato que no admite metadatos embebidos
- **THEN** la pista aparece con su nombre de archivo como título

#### Scenario: Número de pista escrito como "n/total"
- **WHEN** se lee el número de pista "3/12" o "03", o el número de disco "2/2", de un tag de FLAC, OGG, OPUS o MP3
- **THEN** la pista queda guardada con el número de pista 3, o con el disco 2, en cada caso; lo verifica un test en PC sobre la función que interpreta el texto del tag

#### Scenario: Número de pista inválido
- **WHEN** el texto del número de pista está vacío, es "abc" o es "/12"
- **THEN** la pista queda guardada sin número de pista; lo verifica un test en PC

#### Scenario: Número de pista de un MP3 con ID3v1.1
- **WHEN** se indexa un MP3 que solo trae ID3v1.1 con el número de pista 7
- **THEN** la pista queda guardada con el número de pista 7

### Requirement: La ausencia de artista o de álbum se agrupa bajo "Desconocido"
El sistema SHALL agrupar en una entrada única todas las pistas sin artista, y en otra entrada única todas las pistas sin álbum. Esa entrada SHALL mostrarse con la etiqueta del idioma activo ("Desconocido" en español, "Unknown" en inglés), y SHALL ordenarse siempre al final de su lista, con independencia del criterio de orden vigente. Cambiar de idioma SHALL cambiar la etiqueta sin obligar a reescanear, y un artista o un álbum que de verdad se llame "Desconocido" o "Unknown" SHALL seguir siendo una entrada aparte.

#### Scenario: Pistas sin artista
- **WHEN** la biblioteca contiene pistas cuyo formato no provee artista, o cuyos tags no lo traen
- **THEN** todas ellas quedan reunidas bajo una sola entrada "Desconocido" en la vista de artistas, o "Unknown" si la interfaz está en inglés

#### Scenario: "Desconocido" no se intercala entre los nombres reales
- **WHEN** el usuario mira la vista de artistas o la de álbumes y existe la entrada "Desconocido"
- **THEN** esa entrada aparece al final de la lista, después de todos los nombres reales

#### Scenario: Cambiar de idioma con la biblioteca ya escaneada
- **WHEN** la biblioteca ya está escaneada y tiene pistas sin artista, y el usuario cambia el idioma en Ajustes y vuelve a la vista de artistas
- **THEN** la entrada aparece con la etiqueta del idioma nuevo, al final de la lista, con las mismas pistas, y sin que se haya iniciado un escaneo

### Requirement: El índice sobrevive entre sesiones sin reescanear
El sistema SHALL persistir el índice, y SHALL presentar la biblioteca al abrir la aplicación sin volver a recorrer la carpeta de escaneo. Cuando el índice persistido no puede leerse o está incompleto, el sistema SHALL tratarlo como biblioteca no construida y ofrecer escanear, en vez de presentar datos parciales como si fueran la colección.

#### Scenario: Reabrir la aplicación
- **WHEN** el usuario escanea su carpeta, cierra la aplicación y vuelve a abrirla
- **THEN** la biblioteca se muestra completa de inmediato, sin repetir el escaneo

#### Scenario: Índice ilegible
- **WHEN** el índice persistido no puede leerse o quedó incompleto por un corte de energía durante un escaneo
- **THEN** la aplicación se lo informa al usuario y le ofrece escanear de nuevo, y no muestra una biblioteca parcial como si estuviera completa

### Requirement: Reescaneo manual que refleja lo añadido y lo borrado
El sistema SHALL ofrecer al usuario reescanear la carpeta cuando él lo pida, y el índice resultante SHALL reflejar tanto las pistas añadidas desde el último escaneo como la desaparición de las que ya no están. El sistema SHALL NOT reescanear por su cuenta.

#### Scenario: El usuario añadió música
- **WHEN** el usuario copia canciones nuevas dentro de la carpeta de escaneo y pide reescanear
- **THEN** esas canciones aparecen en la biblioteca

#### Scenario: El usuario borró música
- **WHEN** el usuario borra archivos de la carpeta de escaneo y pide reescanear
- **THEN** esas pistas dejan de aparecer en la biblioteca

#### Scenario: No se reescanea solo
- **WHEN** el usuario añade música a la carpeta y abre la biblioteca sin pedir reescaneo
- **THEN** la biblioteca muestra lo que tenía indexado, sin ponerse a recorrer la carpeta por su cuenta

### Requirement: Un escaneo en curso puede abandonarse y deja la biblioteca en un estado coherente
El sistema SHALL permitir al usuario abandonar un escaneo en curso, y SHALL dejar entonces la biblioteca en un estado coherente: o bien el índice anterior intacto, o bien un índice parcial identificado como tal. El sistema SHALL NOT quedar retenido en un escaneo sin forma de salir.

#### Scenario: Abandonar un escaneo
- **WHEN** el usuario abandona un escaneo que está en curso
- **THEN** vuelve a tener el control de la aplicación, y la biblioteca queda o como estaba antes o marcada como incompleta

#### Scenario: La reproducción no se interrumpe
- **WHEN** hay una pista sonando y el usuario lanza un escaneo
- **THEN** la reproducción continúa sin cortes durante todo el escaneo

### Requirement: El escaneo tolera lo que no puede leer
El sistema SHALL continuar el escaneo cuando encuentra un archivo o una carpeta que no puede leer, y SHALL dejar fuera del índice únicamente lo que no pudo leerse. Un archivo ilegible SHALL NOT interrumpir el escaneo ni invalidar lo ya indexado.

#### Scenario: Archivo ilegible a mitad del recorrido
- **WHEN** el escaneo encuentra un archivo con extensión reconocida que no puede abrirse o cuya cabecera está dañada
- **THEN** el escaneo continúa con el resto, y la biblioteca contiene todas las demás pistas

#### Scenario: Carpeta que no puede abrirse
- **WHEN** el escaneo encuentra una subcarpeta que no puede abrirse
- **THEN** el escaneo continúa con las demás carpetas en vez de detenerse

### Requirement: La biblioteca tiene un techo declarado y no lo desborda
El sistema SHALL tener un número máximo de pistas indexables, y cuando una colección lo supere SHALL indexar hasta ese máximo, informar al usuario de que la biblioteca quedó truncada, y seguir funcionando con normalidad. El escaneo SHALL NOT escribir fuera de la memoria que tiene reservada, sea cual sea el número de archivos o de entradas de una carpeta.

#### Scenario: Colección más grande que el techo
- **WHEN** el usuario escanea una carpeta con más pistas reproducibles de las que la biblioteca admite
- **THEN** la biblioteca contiene pistas hasta su máximo, el usuario es informado de que quedó truncada, y la aplicación sigue respondiendo con normalidad

#### Scenario: Una sola carpeta con muchísimas entradas
- **WHEN** el escaneo atraviesa una carpeta que contiene muchos más archivos de los que cabrían en una reserva de tamaño fijo
- **THEN** el escaneo la procesa sin corromper memoria y sin caerse

### Requirement: Las carátulas se extraen una vez por álbum y se conservan
El sistema SHALL obtener la carátula de una pista de su imagen embebida cuando el formato la provea, SHALL extraerla una sola vez por álbum en lugar de una vez por pista, y SHALL conservarla entre sesiones para no repetir la extracción en cada arranque. Las pistas sin carátula disponible SHALL quedar identificadas como tales, sin bloquear ni reintentar indefinidamente.

#### Scenario: Varias pistas del mismo álbum
- **WHEN** se indexa un álbum de varias pistas que llevan todas la misma carátula embebida
- **THEN** la carátula se extrae una sola vez para ese álbum, y todas sus pistas la muestran

#### Scenario: Pista sin carátula embebida
- **WHEN** se indexa una pista cuyo formato no lleva carátula, o que no la trae
- **THEN** la biblioteca la muestra con su marcador por defecto, y no reintenta extraerla en cada arranque

#### Scenario: Las carátulas sobreviven al reinicio
- **WHEN** el usuario cierra la aplicación y vuelve a abrirla después de un escaneo con carátulas
- **THEN** las carátulas se muestran sin volver a abrir los archivos de audio

### Requirement: Un índice de la versión anterior se conserva al actualizar
Cuando la aplicación encuentra un índice persistido por la versión anterior, que no guarda el número de pista ni el de disco, SHALL cargarlo en vez de descartarlo. La biblioteca SHALL mostrarse completa con los títulos, artistas y álbumes que ese índice ya tenía, y todas sus pistas SHALL quedar marcadas como pendientes de releer tags, para que la pasada de tags, reanudable como siempre, complete los números de pista. El sistema SHALL NOT iniciar esa pasada por su cuenta. Un índice de una versión que no es ni la actual ni la anterior SHALL seguir descartándose.

#### Scenario: Primer arranque después de actualizar
- **WHEN** el usuario tenía la biblioteca escaneada en la versión anterior, instala esta versión y abre la Biblioteca
- **THEN** la biblioteca se muestra completa con sus títulos, artistas y álbumes, el contador indica que todas las pistas tienen tags pendientes y no se inicia ningún escaneo

#### Scenario: Completar los números de pista
- **WHEN** después de actualizar el usuario pulsa Triángulo para reescanear y deja terminar la pasada de tags
- **THEN** el contador deja de indicar tags pendientes y los álbumes se ordenan por número de pista

#### Scenario: Abandonar y retomar la relectura
- **WHEN** el usuario abandona la pasada de tags a la mitad, cierra la aplicación, la vuelve a abrir y reescanea
- **THEN** la pasada continúa por las pistas que faltaban, y las que ya se habían releído conservan su número de pista

#### Scenario: Lectura de filas de las dos versiones
- **WHEN** se lee una fila del índice de la versión anterior y una de la versión actual
- **THEN** la de la versión anterior da una pista sin número de pista ni de disco y marcada como pendiente, y la de la versión actual da los mismos campos que se escribieron; lo verifica un test en PC
