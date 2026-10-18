## MODIFIED Requirements

### Requirement: On-screen legend of available physical-button actions
The system SHALL display, on every screen, a legend of the physical-button actions currently available on that screen. Each entry SHALL show the button that performs the action next to the action's name, and the action's name SHALL NOT repeat the button's name in words. The four symbol buttons (Cross, Circle, Square and Triangle) SHALL be shown as their symbol drawn in that button's PlayStation color (Cross blue, Circle red, Square pink, Triangle green), legible over the legend's background. The other buttons (L, R, SELECT, START) SHALL be shown as a neutral chip with the button's name. Las direcciones del D-pad SHALL mostrarse como un chip neutro con una flecha dibujada, en vez de un nombre: el D-pad arriba con una flecha hacia arriba, el D-pad abajo con una flecha hacia abajo, y el par izquierda y derecha, cuando una acción usa los dos, como un solo chip con una flecha hacia cada lado. The entries for confirming and for going back SHALL show whichever symbol button the console assigns to confirm and to cancel. The legend SHALL NOT offer an action to exit the app.

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

#### Scenario: El D-pad abajo y el par izquierda y derecha se muestran como flechas
- **WHEN** el usuario está en Reproduciendo con la vista de visualizador abierta
- **THEN** la leyenda muestra, junto a la acción de visualizador, un chip neutro con una flecha hacia abajo, y junto a la acción de estilo, un solo chip neutro con una flecha hacia la izquierda y otra hacia la derecha, los dos del mismo alto que los chips de L y R y sin ningún texto que nombre los botones con palabras
