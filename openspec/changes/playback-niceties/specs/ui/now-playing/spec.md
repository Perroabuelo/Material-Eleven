## ADDED Requirements

### Requirement: SELECT abre el menú del temporizador
En Reproduciendo, en cualquiera de sus vistas (normal, letras y visualizador), SELECT SHALL abrir sobre la pantalla un menú con el título "Temporizador" ("Sleep timer") y las opciones del temporizador para dormir (ver `playback/sleep-timer`), con la opción vigente marcada y seleccionada al abrirlo. En el menú, arriba y abajo SHALL mover la selección, el botón de confirmar SHALL elegir la opción seleccionada y cerrar el menú, y el botón de volver o SELECT SHALL cerrarlo sin cambiar nada. Tocar una opción SHALL elegirla y cerrar el menú, y tocar fuera del menú SHALL cerrarlo sin cambiar nada. Mientras el menú está abierto, los demás botones de Reproduciendo (gatillos, Triángulo, Cuadrado, izquierda, derecha) y los toques fuera del menú SHALL NOT actuar sobre la reproducción ni sobre la vista, y la música SHALL seguir sonando. START SHALL seguir apagando la pantalla, como define `playback/screen-off`.

#### Scenario: Abrir y elegir con botones
- **WHEN** el usuario pulsa SELECT en Reproduciendo, baja una fila y pulsa el botón de confirmar
- **THEN** el menú se cierra, el temporizador queda en "15 min" y la música sigue sonando

#### Scenario: Cerrar sin cambiar nada
- **WHEN** el usuario tiene un temporizador de "30 min", abre el menú con SELECT, baja dos filas y pulsa el botón de volver
- **THEN** el menú se cierra, el temporizador sigue en "30 min" y la aplicación sigue en Reproduciendo

#### Scenario: Elegir tocando
- **WHEN** el usuario abre el menú con SELECT y toca "Al terminar la pista"
- **THEN** el menú se cierra y el temporizador queda en el modo de fin de pista

#### Scenario: Los gatillos no actúan con el menú abierto
- **WHEN** el usuario abre el menú con SELECT y pulsa R
- **THEN** la pista en curso sigue sonando, sin pasar a la siguiente, y el menú sigue abierto

#### Scenario: Desde la vista de letras
- **WHEN** el usuario abre la vista de letras, pulsa SELECT y cierra el menú con el botón de volver
- **THEN** el menú se abre sobre la vista de letras, y al cerrarlo la vista de letras sigue abierta

### Requirement: La leyenda de Reproduciendo anuncia el temporizador
La leyenda de botones de Reproduciendo SHALL incluir, en las tres vistas, la entrada de SELECT con la acción "Temporizador" en español o "Sleep timer" en inglés, como un chip neutro con el nombre del botón. La entrada SHALL ir después de las de Triángulo y Cuadrado y antes de la de START. Con el menú del temporizador abierto, la leyenda SHALL mostrar solo las acciones del menú: elegir, con el botón de confirmar, y cerrar, con el botón de volver.

#### Scenario: Entrada del temporizador en los dos idiomas
- **WHEN** el usuario está en Reproduciendo, en la vista normal, con la interfaz en español y luego en inglés
- **THEN** la leyenda muestra el chip de SELECT junto a "Temporizador" y luego junto a "Sleep timer", sin solaparse con las demás entradas

#### Scenario: Leyenda con el menú abierto
- **WHEN** el usuario abre el menú del temporizador con SELECT
- **THEN** la leyenda muestra solo el botón de confirmar junto a la acción de elegir y el botón de volver junto a la de cerrar
