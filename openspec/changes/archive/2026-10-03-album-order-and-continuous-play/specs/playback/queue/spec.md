## MODIFIED Requirements

### Requirement: Origen y orden natural de la cola
La cola SHALL recibir su contenido de quien inicia la reproducción —una carpeta del navegador o la biblioteca—, conservando como orden natural de la cola el orden en que el origen entrega sus pistas. La biblioteca entrega la vista desde la que se reprodujo o, cuando el ajuste "Al terminar un álbum o artista" lo pide, toda la biblioteca agrupada por álbum o por artista (ver `library/views`). Rellenar la cola SHALL reemplazarla por completo, sin dejar pistas de la anterior.

#### Scenario: Reproducción iniciada desde una carpeta
- **WHEN** el usuario reproduce un archivo desde el navegador de carpetas
- **THEN** la cola contiene los archivos reproducibles de esa carpeta, en el orden en que la carpeta los muestra

#### Scenario: Reproducción iniciada desde una vista de biblioteca
- **WHEN** el usuario reproduce una pista desde una vista de biblioteca con el ajuste "Al terminar un álbum o artista" en "Repetirlo"
- **THEN** la cola contiene las pistas de esa vista, en el orden de la vista, y no las de la carpeta donde está el archivo

#### Scenario: Reproducción continua desde un álbum
- **WHEN** el usuario reproduce una pista desde dentro de un álbum con el ajuste en "Seguir con el siguiente"
- **THEN** la cola contiene todas las pistas de la biblioteca, agrupadas por álbum en el orden de la vista Álbumes, y la reproducción empieza por la pista elegida

#### Scenario: Una cola nueva sustituye a la anterior
- **WHEN** el usuario inicia la reproducción desde otra carpeta o desde otra vista
- **THEN** la cola pasa a contener únicamente las pistas del nuevo origen
