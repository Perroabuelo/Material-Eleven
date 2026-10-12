## MODIFIED Requirements

### Requirement: Existing physical-button navigation still works
The system SHALL preserve the physical-button navigation behavior that existed before the navigation rail (for example, SELECT opening Settings from the folder browser), alongside the new rail-based navigation. En Reproduciendo, el botón de volver de la consola SHALL llevar a la pantalla donde el usuario eligió la canción que suena (Carpetas o Biblioteca), y SHALL hacerlo aunque el usuario haya llegado a Reproduciendo desde otra pantalla con el nav rail. Al volver a la Biblioteca, la pantalla SHALL mostrar la misma pestaña, el mismo artista o álbum abierto, la misma búsqueda aplicada y la misma fila seleccionada que tenía cuando se eligió la canción. Pasar a la pista siguiente o a la anterior SHALL NOT cambiar adónde lleva el botón de volver. START SHALL NOT exit the app on any screen; its behavior is defined by `playback/screen-off`. The app SHALL be closed the same way as any other console application, from the PS button and its LiveArea.

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

#### Scenario: Volver a la Biblioteca con una búsqueda aplicada
- **WHEN** el usuario filtra la vista Canciones con un término, reproduce el segundo resultado y, en Reproduciendo, pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a la Biblioteca, en la vista Canciones, con el mismo término aplicado y con el segundo resultado seleccionado

#### Scenario: El origen se mantiene al entrar por el nav rail
- **WHEN** el usuario reproduce una canción desde la Biblioteca, va a Ajustes con el nav rail, vuelve a Reproduciendo con el nav rail y pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a la Biblioteca, y no a Ajustes ni a Carpetas

#### Scenario: Cambiar de pista no cambia el origen
- **WHEN** el usuario reproduce una canción desde la Biblioteca, pasa a la pista siguiente con R y pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a la Biblioteca

#### Scenario: Una nueva elección cambia el origen
- **WHEN** el usuario reproduce una canción desde la Biblioteca, va a Carpetas con el nav rail, reproduce un archivo ahí y pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a Carpetas

### Requirement: On-screen legend of available physical-button actions
The system SHALL display, on every screen, a legend of the physical-button actions currently available on that screen. Each entry SHALL show the button that performs the action next to the action's name, and the action's name SHALL NOT repeat the button's name in words. The four symbol buttons (Cross, Circle, Square and Triangle) SHALL be shown as their symbol drawn in that button's PlayStation color (Cross blue, Circle red, Square pink, Triangle green), legible over the legend's background. The other buttons (L, R, SELECT, START) SHALL be shown as a neutral chip with the button's name. El D-pad arriba SHALL mostrarse como un chip neutro con una flecha hacia arriba dibujada, en vez de un nombre. The entries for confirming and for going back SHALL show whichever symbol button the console assigns to confirm and to cancel. The legend SHALL NOT offer an action to exit the app.

#### Scenario: Legend reflects the current screen
- **WHEN** the user is viewing the Folders screen
- **THEN** the legend lists the actions available there (such as open/play, go to parent folder and search) and not actions that only apply to a different screen

#### Scenario: Cada acción muestra su botón
- **WHEN** el usuario está en Biblioteca con una lista de canciones a la vista
- **THEN** la leyenda muestra el símbolo del botón de confirmar junto a la acción de reproducir, el Triángulo verde junto a "Buscar", el Cuadrado rosa junto a "Opciones", y chips neutros con L y R junto a "Vistas", sin que ningún texto nombre el botón con palabras

#### Scenario: Buscar aparece en Carpetas
- **WHEN** el usuario está en Carpetas
- **THEN** la leyenda muestra el Triángulo verde junto a "Buscar"

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

#### Scenario: El D-pad arriba se muestra como flecha
- **WHEN** el usuario está en Reproduciendo
- **THEN** la leyenda muestra, junto a la acción de letras, un chip neutro con una flecha hacia arriba, del mismo alto que los chips de L y R, y ningún texto que nombre el botón con palabras
