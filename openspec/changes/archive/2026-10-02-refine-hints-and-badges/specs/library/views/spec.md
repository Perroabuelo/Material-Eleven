## ADDED Requirements

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
