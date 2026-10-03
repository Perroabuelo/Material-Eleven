# Nav Shell Specification

## Purpose

Gives the user a persistent, always-visible way to switch between the app's top-level screens, replacing the current model where each screen is a dead-end reachable only by exiting the previous one or knowing a specific physical-button shortcut.

## Requirements

### Requirement: Persistent navigation rail lists only available destinations
The system SHALL display a persistent navigation rail on every top-level screen, listing exactly the destinations that are implemented (Now Playing, Folders, Library, Settings). It SHALL NOT list a destination whose underlying screen or functionality does not exist.

#### Scenario: Rail matches implemented destinations
- **WHEN** the user views any top-level screen
- **THEN** the navigation rail shows entries for Now Playing, Folders, Library, and Settings only, with no entry for Playlists or a queue-management screen

#### Scenario: La biblioteca se alcanza desde cualquier pantalla
- **WHEN** el usuario está en cualquier pantalla de nivel superior y elige la entrada de biblioteca en el rail
- **THEN** la aplicación pasa a la pantalla de biblioteca, sin tener que volver antes a ninguna otra

#### Scenario: La biblioteca aparece aunque no se haya escaneado nunca
- **WHEN** el usuario no ha elegido todavía ninguna carpeta de escaneo
- **THEN** la entrada de biblioteca aparece igualmente en el rail, porque la pantalla existe y es donde se elige esa carpeta

### Requirement: Selecting a rail entry switches the active screen
The system SHALL switch to a destination's screen when the user selects its entry in the rail, without requiring a separate back/cancel action first.

#### Scenario: Switching from Folders to Settings via the rail
- **WHEN** the user is viewing Folders and selects the Settings entry in the rail
- **THEN** the app switches to the Settings screen

### Requirement: Rail indicates the active destination
The system SHALL visually distinguish the entry for the currently active screen from the other entries in the rail.

#### Scenario: Active entry is distinguishable
- **WHEN** the user is viewing the Folders screen
- **THEN** the Folders entry in the rail is visually marked as active and the other entries are not

### Requirement: Existing physical-button navigation still works
The system SHALL preserve the physical-button navigation behavior that existed before the navigation rail (for example, SELECT opening Settings from the folder browser, and the cancel button returning to the previous screen), alongside the new rail-based navigation. START SHALL NOT exit the app on any screen; its behavior is defined by `playback/screen-off`. The app SHALL be closed the same way as any other console application, from the PS button and its LiveArea.

#### Scenario: SELECT still opens Settings
- **WHEN** the user presses SELECT while viewing Folders
- **THEN** the app opens Settings, the same outcome as before this change

#### Scenario: START ya no cierra la aplicación
- **WHEN** el usuario pulsa START en Carpetas o en Biblioteca
- **THEN** la aplicación sigue abierta, en la misma pantalla, al volver a encender la pantalla

#### Scenario: El botón de volver sigue regresando
- **WHEN** el usuario está en Reproduciendo y pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a Carpetas, igual que antes de este cambio

### Requirement: On-screen legend of available physical-button actions
The system SHALL display, on every screen, a legend of the physical-button actions currently available on that screen. Each entry SHALL show the button that performs the action next to the action's name, and the action's name SHALL NOT repeat the button's name in words. The four symbol buttons (Cross, Circle, Square and Triangle) SHALL be shown as their symbol drawn in that button's PlayStation color (Cross blue, Circle red, Square pink, Triangle green), legible over the legend's background. The other buttons (L, R, SELECT, START) SHALL be shown as a neutral chip with the button's name. The entries for confirming and for going back SHALL show whichever symbol button the console assigns to confirm and to cancel. The legend SHALL NOT offer an action to exit the app.

#### Scenario: Legend reflects the current screen
- **WHEN** the user is viewing the Folders screen
- **THEN** the legend lists the actions available there (such as open/play and go to parent folder) and not actions that only apply to a different screen

#### Scenario: Cada acción muestra su botón
- **WHEN** el usuario está en Biblioteca con una lista de canciones a la vista
- **THEN** la leyenda muestra el símbolo del botón de confirmar junto a la acción de reproducir, el Triángulo verde junto a "Reescanear", el Cuadrado rosa junto a "Carpeta", y chips neutros con L y R junto a "Vistas", sin que ningún texto nombre el botón con palabras

#### Scenario: Consola que confirma con Cruz
- **WHEN** la consola tiene asignada la Cruz para confirmar y el usuario está en Carpetas
- **THEN** la leyenda muestra la Cruz azul junto a "Abrir / Reproducir" y el Círculo rojo junto a la acción de volver

#### Scenario: Consola que confirma con Círculo
- **WHEN** la consola tiene asignado el Círculo para confirmar y el usuario está en Carpetas
- **THEN** la leyenda muestra el Círculo rojo junto a "Abrir / Reproducir" y la Cruz azul junto a la acción de volver, y esos son los botones que hacen cada cosa

#### Scenario: La leyenda no ofrece salir
- **WHEN** el usuario mira la leyenda de cualquier pantalla
- **THEN** ninguna entrada ofrece salir de la aplicación

#### Scenario: Los íconos se ven en los dos idiomas
- **WHEN** el usuario cambia el idioma entre English y Español y recorre Carpetas, Biblioteca, Reproduciendo y Ajustes
- **THEN** en cada pantalla la leyenda muestra los mismos íconos de botones, con los textos en el idioma elegido y sin solaparse entre entradas
