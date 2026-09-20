## MODIFIED Requirements

### Requirement: In-folder filename filter
The system SHALL let the user filter the current directory's already-loaded listing by typing part of a name, and SHALL present a text-entry surface when the user activates the filter. The filter SHALL apply only to entries already listed in the current folder and SHALL NOT trigger reading any other folder. While the text-entry surface is open the system SHALL keep the folder listing visible behind it, and SHALL honor the console's own confirm and cancel button assignment, the same one the rest of the application honors.

#### Scenario: Filtering narrows the current listing
- **WHEN** the user types a search term while viewing a folder's contents
- **THEN** only entries in that folder whose names contain the term remain visible

#### Scenario: Filter does not reach into subfolders
- **WHEN** the user types a search term
- **THEN** entries inside subfolders of the current folder are not searched or shown as results

#### Scenario: Activar el filtro presenta la entrada de texto
- **WHEN** el usuario activa el filtro desde la lista de carpetas
- **THEN** aparece una superficie de entrada de texto sobre la pantalla, con el término vigente ya cargado si el usuario había filtrado antes

#### Scenario: La lista sigue visible detrás de la entrada de texto
- **WHEN** la entrada de texto está abierta
- **THEN** la lista de la carpeta sigue dibujándose detrás, en vez de quedar reemplazada por un color plano

#### Scenario: El botón de aceptar es el mismo que en el resto de la aplicación
- **WHEN** el usuario confirma o descarta la entrada de texto
- **THEN** los botones que confirman y descartan son los que la consola tiene asignados, y coinciden con los que el usuario usa para abrir y para volver en el resto de las pantallas

## ADDED Requirements

### Requirement: El usuario nunca queda atrapado en el filtro
The system SHALL always leave the user a way out of the filter, both while the text entry is open and after a term has been applied. Abandoning the text entry SHALL return the user to the listing with the filter unchanged. When an applied filter leaves the listing with no entries to act on, the user SHALL be able to remove that filter using a physical button, without depending on touch and without depending on where the current folder sits relative to the device root.

#### Scenario: Descartar la entrada de texto devuelve el control
- **WHEN** el usuario descarta la entrada de texto sin confirmar un término
- **THEN** vuelve a la lista de carpetas con el filtro que tenía antes, y la pantalla responde de nuevo a los botones y al toque

#### Scenario: La entrada de texto no llega a mostrarse
- **WHEN** el usuario activa el filtro y la entrada de texto no llega a presentarse o deja de progresar
- **THEN** la aplicación vuelve por sí sola a la lista de carpetas en un estado utilizable, en lugar de quedar retenida hasta que el usuario cierre el proceso desde el sistema

#### Scenario: Un filtro sin coincidencias en la raíz del dispositivo
- **WHEN** el usuario aplica en la raíz del dispositivo un término que no coincide con ninguna entrada, de modo que la lista queda vacía
- **THEN** puede quitar el filtro con un botón físico y recuperar la lista completa

#### Scenario: Quitar el filtro por botón por debajo de la raíz
- **WHEN** el usuario tiene un filtro aplicado en una carpeta que no es la raíz y usa el botón de volver
- **THEN** se quita el filtro y sigue viendo la misma carpeta, y solo un uso posterior de ese botón lo lleva a la carpeta superior
