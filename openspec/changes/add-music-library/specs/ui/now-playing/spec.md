## MODIFIED Requirements

### Requirement: Read-only upcoming-tracks preview
The system SHALL show a short preview of the upcoming track(s) in the current queue's existing order, whatever the queue was built from, without offering controls to reorder, remove, or otherwise edit that order.

#### Scenario: Preview reflects playlist order
- **WHEN** the current queue has more than one remaining track
- **THEN** the screen shows the upcoming track(s) in playback order, with no control to reorder or remove them

#### Scenario: La previsualización refleja una cola venida de la biblioteca
- **WHEN** el usuario inició la reproducción desde una vista de biblioteca y quedan pistas por delante
- **THEN** la previsualización muestra las pistas siguientes de esa vista, y no las de la carpeta en la que está el archivo que suena

#### Scenario: La previsualización refleja una cola venida de una carpeta
- **WHEN** el usuario inició la reproducción desde el navegador de carpetas y quedan pistas por delante
- **THEN** la previsualización muestra las pistas siguientes de esa carpeta, igual que antes de existir la biblioteca
