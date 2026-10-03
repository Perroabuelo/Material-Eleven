## MODIFIED Requirements

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
