## MODIFIED Requirements

### Requirement: Directory listing with format badges
The system SHALL list the current directory's folders and files, showing a format badge on entries with a recognized audio extension (flac, it, mod, mp3, ogg, opus, s3m, wav, xm), matching the extensions currently recognized for playback. The badge SHALL show the format's name as legible text in its format family's color over a faint background of that same color, without an outline: lossless formats (FLAC, WAV) in green, lossy formats (MP3, OGG, OPUS) in purple, and tracker formats (IT, MOD, S3M, XM) in amber.

#### Scenario: Recognized audio file is badged
- **WHEN** the current directory contains a file with a recognized audio extension
- **THEN** its row shows a badge naming that format

#### Scenario: Unrecognized file has no format badge
- **WHEN** the current directory contains a file whose extension is not recognized for playback
- **THEN** its row shows no format badge

#### Scenario: El nombre del formato se lee dentro del badge
- **WHEN** el usuario navega una carpeta con archivos de cada uno de los nueve formatos reconocidos
- **THEN** cada badge muestra legible el nombre de su formato (FLAC, WAV, MP3, OGG, OPUS, IT, MOD, S3M o XM), sin borde, con texto verde para FLAC y WAV, morado para MP3, OGG y OPUS, y ámbar para IT, MOD, S3M y XM, cada uno sobre un fondo tenue de su mismo color

#### Scenario: El badge se lee también en la fila seleccionada
- **WHEN** el usuario selecciona la fila de un archivo de audio reconocido
- **THEN** el nombre del formato sigue siendo legible sobre el resaltado de la fila
