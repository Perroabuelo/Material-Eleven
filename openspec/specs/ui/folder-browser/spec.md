# Folder Browser Specification

## Purpose

Lets the user navigate the device's file system to find and play audio files, presented in the app's shared visual language, without introducing a cross-folder index or search.

## Requirements

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

### Requirement: Directory navigation
The system SHALL let the user enter a subfolder and return to the parent folder, and SHALL NOT allow navigating above the currently configured root.

#### Scenario: Entering a subfolder
- **WHEN** the user selects a folder entry
- **THEN** the screen shows that folder's contents

#### Scenario: Returning to the parent folder
- **WHEN** the user is below the configured root and activates the "go to parent" action
- **THEN** the screen shows the parent folder's contents

#### Scenario: Root is a navigation boundary
- **WHEN** the user is viewing the configured root folder
- **THEN** no "go to parent" action is available

### Requirement: Opening a file
The system SHALL begin playback and show the Now Playing screen when the user selects a file with a recognized audio extension, and SHALL take no playback action when the user selects a file whose extension is not recognized.

#### Scenario: Opening a recognized audio file
- **WHEN** the user selects a file with a recognized audio extension
- **THEN** the app begins playback of that file and shows the Now Playing screen

#### Scenario: Selecting an unrecognized file
- **WHEN** the user selects a file whose extension is not recognized for playback
- **THEN** no playback begins

### Requirement: In-folder filename filter
The system SHALL let the user filter the current directory's already-loaded listing by typing part of a name, and SHALL present a text-entry surface when the user activates the filter. The filter SHALL apply only to entries already listed in the current folder and SHALL NOT trigger reading any other folder. The text-entry surface SHALL honor the console's own confirm and cancel button assignment, the same one the rest of the application honors.

#### Scenario: Filtering narrows the current listing
- **WHEN** the user types a search term while viewing a folder's contents
- **THEN** only entries in that folder whose names contain the term remain visible

#### Scenario: Filter does not reach into subfolders
- **WHEN** the user types a search term
- **THEN** entries inside subfolders of the current folder are not searched or shown as results

#### Scenario: Activar el filtro presenta la entrada de texto
- **WHEN** el usuario activa el filtro desde la lista de carpetas
- **THEN** aparece una superficie de entrada de texto sobre la pantalla, con el término vigente ya cargado si el usuario había filtrado antes

#### Scenario: El botón de aceptar es el mismo que en el resto de la aplicación
- **WHEN** el usuario confirma o descarta la entrada de texto
- **THEN** los botones que confirman y descartan son los que la consola tiene asignados, y coinciden con los que el usuario usa para abrir y para volver en el resto de las pantallas

### Requirement: El usuario nunca queda atrapado en el filtro
The system SHALL always leave the user a way out of the filter, both while the text entry is open and after a term has been applied. Abandoning the text entry SHALL return the user to the listing with the filter unchanged. When an applied filter leaves the listing with no entries to act on, the user SHALL be able to remove that filter using a physical button, without depending on touch and without depending on where the current folder sits relative to the device root.

#### Scenario: Descartar la entrada de texto devuelve el control
- **WHEN** el usuario descarta la entrada de texto sin confirmar un término
- **THEN** vuelve a la lista de carpetas con el filtro que tenía antes, y la pantalla responde de nuevo a los botones y al toque

#### Scenario: La entrada de texto no llega a mostrarse
- **WHEN** el usuario activa el filtro y la entrada de texto no llega a presentarse
- **THEN** la aplicación deja de esperarla por sí sola y vuelve a la lista de carpetas, en lugar de quedar retenida indefinidamente, y la reproducción en curso no se interrumpe

#### Scenario: Un filtro sin coincidencias en la raíz del dispositivo
- **WHEN** el usuario aplica en la raíz del dispositivo un término que no coincide con ninguna entrada, de modo que la lista queda vacía
- **THEN** puede quitar el filtro con un botón físico y recuperar la lista completa

#### Scenario: Quitar el filtro por botón por debajo de la raíz
- **WHEN** el usuario tiene un filtro aplicado en una carpeta que no es la raíz y usa el botón de volver
- **THEN** se quita el filtro y sigue viendo la misma carpeta, y solo un uso posterior de ese botón lo lleva a la carpeta superior

### Requirement: Docked mini-player while browsing
The system SHALL show a persistent mini-player, reflecting the currently playing track's title, artist, and basic transport controls, while the user browses folders, without interrupting playback.

#### Scenario: Mini-player reflects active playback while browsing
- **WHEN** a track is playing and the user navigates the folder browser
- **THEN** a mini-player showing that track's title, artist, and transport controls remains visible and playback continues uninterrupted

### Requirement: El listado de una carpeta nunca desborda su reserva
El sistema SHALL acotar el número de entradas que lee de una carpeta al tamaño de la memoria que tiene reservada para ellas. Cuando una carpeta contiene más entradas de las que esa reserva admite, el sistema SHALL mostrar las que caben y seguir funcionando con normalidad, y SHALL NOT escribir fuera de la reserva. Esto SHALL aplicar por igual a la construcción del listado y a la construcción de la cola de reproducción a partir de la carpeta.

#### Scenario: Carpeta con más entradas de las que caben
- **WHEN** el usuario entra en una carpeta que contiene más entradas de las que la aplicación puede listar
- **THEN** se muestran las entradas que caben, la navegación y la reproducción siguen respondiendo, y la aplicación no se cae ni corrompe lo que muestra

#### Scenario: Reproducir desde una carpeta con más entradas de las que caben
- **WHEN** el usuario reproduce un archivo de una carpeta que contiene más entradas de las que la aplicación puede listar
- **THEN** la reproducción comienza y la cola se construye con las pistas que caben, sin caídas
