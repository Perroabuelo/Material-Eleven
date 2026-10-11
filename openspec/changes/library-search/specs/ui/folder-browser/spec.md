## MODIFIED Requirements

### Requirement: In-folder filename filter
The system SHALL let the user filter the current directory's already-loaded listing by typing part of a name, and SHALL present a text-entry surface when the user activates the filter. The filter SHALL be activated both by pressing the Triangle button and by touching the filter box in the top bar. The filter SHALL apply only to entries already listed in the current folder and SHALL NOT trigger reading any other folder. The text-entry surface SHALL honor the console's own confirm and cancel button assignment, the same one the rest of the application honors.

La comparación SHALL NOT distinguir entre mayúsculas y minúsculas ni entre letras con y sin tilde o diéresis, y SHALL tratar la ñ como n, igual que la búsqueda de la biblioteca. El término que el usuario escribe, con sus tildes y eñes, SHALL llegar intacto al filtro, SHALL mostrarse así en la caja de filtro y SHALL volver a cargarse así en la entrada de texto.

#### Scenario: Filtering narrows the current listing
- **WHEN** the user types a search term while viewing a folder's contents
- **THEN** only entries in that folder whose names contain the term remain visible

#### Scenario: Filter does not reach into subfolders
- **WHEN** the user types a search term
- **THEN** entries inside subfolders of the current folder are not searched or shown as results

#### Scenario: Activar el filtro presenta la entrada de texto
- **WHEN** el usuario activa el filtro desde la lista de carpetas
- **THEN** aparece una superficie de entrada de texto sobre la pantalla, con el término vigente ya cargado si el usuario había filtrado antes

#### Scenario: Triángulo abre el filtro
- **WHEN** el usuario está en la lista de carpetas y pulsa Triángulo
- **THEN** aparece la entrada de texto del filtro, igual que al tocar la caja de filtro

#### Scenario: El botón de aceptar es el mismo que en el resto de la aplicación
- **WHEN** el usuario confirma o descarta la entrada de texto
- **THEN** los botones que confirman y descartan son los que la consola tiene asignados, y coinciden con los que el usuario usa para abrir y para volver en el resto de las pantallas

#### Scenario: Filtrar sin tildes encuentra nombres con tildes
- **WHEN** la carpeta contiene el archivo "Canción.flac" y el usuario filtra con "cancion"
- **THEN** el archivo queda visible; y filtrar con "CANCIÓN" también lo deja visible

#### Scenario: Un término con tildes no se desfigura
- **WHEN** el usuario escribe "acción" en la entrada de texto y confirma
- **THEN** la caja de filtro muestra "acción", los archivos cuyo nombre lo contiene quedan visibles, y al abrir de nuevo el filtro la entrada de texto muestra "acción"
