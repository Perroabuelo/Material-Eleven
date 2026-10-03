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
The system SHALL show elapsed and remaining time and allow the user to seek to a position by touching the progress control, matching current seek behavior.

#### Scenario: Seeking via touch
- **WHEN** the user touches a point along the progress control
- **THEN** playback position updates to correspond to that point

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
