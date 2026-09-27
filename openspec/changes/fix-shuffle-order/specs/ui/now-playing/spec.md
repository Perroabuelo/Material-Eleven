## MODIFIED Requirements

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
