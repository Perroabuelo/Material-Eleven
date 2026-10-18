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
La leyenda de botones de Reproduciendo SHALL incluir la entrada de SELECT con la acción "Dormir" en español o "Sleep" en inglés, como un chip neutro con el nombre del botón. La entrada SHALL ir después de las de Triángulo y Cuadrado y antes de la de START. En las vistas normal y de letras la entrada SHALL verse en los dos idiomas. En la vista de visualizador, donde la entrada de estilo ocupa su lugar, la entrada de SELECT MAY quedar fuera junto con las que la siguen, según la regla de que lo que no cabe se cae desde el final. Con el menú del temporizador abierto, la leyenda SHALL mostrar solo las acciones del menú: elegir, con el botón de confirmar, y cerrar, con el botón de volver.

#### Scenario: Entrada del temporizador en los dos idiomas
- **WHEN** el usuario está en Reproduciendo, en la vista normal y luego en la de letras, con la interfaz en español y luego en inglés
- **THEN** en las dos vistas la leyenda muestra el chip de SELECT junto a "Dormir" y luego junto a "Sleep", sin solaparse con las demás entradas

#### Scenario: La leyenda del visualizador se corta sin solaparse
- **WHEN** el usuario abre la vista de visualizador, con la interfaz en español y luego en inglés
- **THEN** la leyenda muestra sus entradas en orden hasta donde caben, sin que ninguna se solape con otra ni con el borde de la barra

#### Scenario: Leyenda con el menú abierto
- **WHEN** el usuario abre el menú del temporizador con SELECT
- **THEN** la leyenda muestra solo el botón de confirmar junto a la acción de elegir y el botón de volver junto a la de cerrar

## MODIFIED Requirements

### Requirement: La leyenda de Reproduciendo anuncia las letras
La leyenda de botones de Reproduciendo SHALL incluir, en las tres vistas (normal, letras y visualizador), una sola entrada con los chips de D-pad arriba y D-pad abajo y la acción "Vistas" en español o "Views" en inglés. En la vista de visualizador SHALL incluir además, justo después, la entrada de D-pad izquierda y derecha con la acción "Estilo" en español o "Style" en inglés. La entrada de L y R SHALL mostrar la acción "<< >>" en los dos idiomas. Si las entradas no caben en la leyenda, SHALL quedar fuera las del final, sin que ninguna se solape con otra.

#### Scenario: Entrada de letras en la leyenda
- **WHEN** el usuario está en Reproduciendo, en la vista normal y luego en la de letras
- **THEN** en ambas la leyenda muestra el chip de D-pad arriba, junto al de D-pad abajo, en una sola entrada con "Vistas", sin solaparse con las demás entradas

#### Scenario: Entrada de visualizador en la leyenda
- **WHEN** el usuario está en Reproduciendo, en la vista normal, luego en la de letras y luego en la de visualizador
- **THEN** en las tres la leyenda muestra el chip de D-pad abajo, junto al de D-pad arriba, en una sola entrada con "Vistas", sin solaparse con las demás entradas

#### Scenario: Entrada de estilo solo en el visualizador
- **WHEN** el usuario está en Reproduciendo, primero en la vista normal y después en la de visualizador, con la interfaz en inglés
- **THEN** en la vista normal la leyenda no muestra ninguna entrada de estilo, y en la de visualizador muestra el chip de D-pad izquierda y derecha junto a "Style"

#### Scenario: L y R con flechas
- **WHEN** el usuario está en Reproduciendo, con la interfaz en español y luego en inglés
- **THEN** la leyenda muestra los chips de L y R junto a "<< >>" en los dos idiomas
