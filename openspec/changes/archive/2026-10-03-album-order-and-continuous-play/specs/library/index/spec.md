## MODIFIED Requirements

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

## ADDED Requirements

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
