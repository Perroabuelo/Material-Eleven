## MODIFIED Requirements

### Requirement: Existing physical-button navigation still works
The system SHALL preserve the physical-button navigation behavior that existed before the navigation rail (for example, SELECT opening Settings from the folder browser), alongside the new rail-based navigation. En Reproduciendo, el botón de volver de la consola SHALL llevar a la pantalla donde el usuario eligió la canción que suena (Carpetas o Biblioteca), y SHALL hacerlo aunque el usuario haya llegado a Reproduciendo desde otra pantalla con el nav rail. Al volver a la Biblioteca, la pantalla SHALL mostrar la misma pestaña, el mismo artista o álbum abierto y la misma fila seleccionada que tenía cuando se eligió la canción. Pasar a la pista siguiente o a la anterior SHALL NOT cambiar adónde lleva el botón de volver. START SHALL NOT exit the app on any screen; its behavior is defined by `playback/screen-off`. The app SHALL be closed the same way as any other console application, from the PS button and its LiveArea.

#### Scenario: SELECT still opens Settings
- **WHEN** the user presses SELECT while viewing Folders
- **THEN** the app opens Settings, the same outcome as before this change

#### Scenario: START ya no cierra la aplicación
- **WHEN** el usuario pulsa START en Carpetas o en Biblioteca
- **THEN** la aplicación sigue abierta, en la misma pantalla, al volver a encender la pantalla

#### Scenario: El botón de volver sigue regresando
- **WHEN** el usuario reproduce un archivo desde Carpetas y, en Reproduciendo, pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a Carpetas, en la misma carpeta y con el mismo archivo seleccionado, igual que antes de este cambio

#### Scenario: Volver a la Biblioteca cuando la canción se eligió ahí
- **WHEN** el usuario abre un álbum en la vista Álbumes de la Biblioteca, reproduce su tercera pista y, en Reproduciendo, pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a la Biblioteca, en la vista Álbumes, dentro de ese mismo álbum y con la tercera pista seleccionada

#### Scenario: El origen se mantiene al entrar por el nav rail
- **WHEN** el usuario reproduce una canción desde la Biblioteca, va a Ajustes con el nav rail, vuelve a Reproduciendo con el nav rail y pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a la Biblioteca, y no a Ajustes ni a Carpetas

#### Scenario: Cambiar de pista no cambia el origen
- **WHEN** el usuario reproduce una canción desde la Biblioteca, pasa a la pista siguiente con R y pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a la Biblioteca

#### Scenario: Una nueva elección cambia el origen
- **WHEN** el usuario reproduce una canción desde la Biblioteca, va a Carpetas con el nav rail, reproduce un archivo ahí y pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a Carpetas
