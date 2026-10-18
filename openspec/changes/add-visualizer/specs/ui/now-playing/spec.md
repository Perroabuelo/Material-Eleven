## MODIFIED Requirements

### Requirement: Vista de letras con toggle
Reproduciendo SHALL ofrecer una vista de letras a pantalla completa, que se abre y se cierra con D-pad arriba. D-pad arriba SHALL abrir también la vista de letras cuando la que está abierta es la de visualizador (ver `ui/visualizer`). Mientras está abierta, SHALL mostrar en lugar de la carátula grande, el transporte en pantalla y la previsualización de próximas pistas: una cabecera con la carátula en pequeño, el título, el artista y el badge de formato; la letra en el centro; y abajo la barra de progreso con el tiempo transcurrido y el total. El nav rail y la leyenda de botones SHALL seguir visibles. Los botones físicos SHALL hacer en la vista de letras lo mismo que en la vista normal (pausa, pista anterior y siguiente, barajado, repetición, apagar pantalla y volver). La vista elegida SHALL mantenerse al cambiar de pista y al salir y volver a Reproduciendo durante la misma sesión, y la app SHALL arrancar siempre en la vista normal.

#### Scenario: Abrir y cerrar las letras
- **WHEN** el usuario está en Reproduciendo con una pista sonando, pulsa D-pad arriba, y después vuelve a pulsarlo
- **THEN** la primera pulsación muestra la vista de letras con la cabecera, la letra y la barra de progreso, sin cortar la música, y la segunda vuelve a la vista normal tal como estaba

#### Scenario: Abrir las letras desde el visualizador
- **WHEN** el usuario, con la vista de visualizador abierta, pulsa D-pad arriba
- **THEN** se muestra la vista de letras de la pista que suena, sin pasar por la vista normal

#### Scenario: Los botones siguen funcionando en la vista de letras
- **WHEN** el usuario, con la vista de letras abierta, pulsa el botón de confirmar, luego R, luego Triángulo
- **THEN** la música se pausa, al pulsar R suena la pista siguiente con la vista de letras todavía abierta y mostrando la letra de esa pista, y Triángulo cambia el barajado como en la vista normal

#### Scenario: Volver desde la vista de letras
- **WHEN** el usuario reproduce un archivo desde Carpetas, abre la vista de letras y pulsa el botón de volver de la consola
- **THEN** la aplicación vuelve a Carpetas, igual que desde la vista normal

#### Scenario: La vista se recuerda en la sesión
- **WHEN** el usuario abre la vista de letras, va a Ajustes con el nav rail y vuelve a Reproduciendo con el nav rail
- **THEN** Reproduciendo aparece con la vista de letras abierta

#### Scenario: Buscar con la barra de la vista de letras
- **WHEN** el usuario, con la vista de letras abierta, toca la barra de progreso en su punto medio
- **THEN** la canción salta aproximadamente a la mitad de su duración, y al tocar el extremo derecho de la barra salta a los últimos segundos

### Requirement: La leyenda de Reproduciendo anuncia las letras
La leyenda de botones de Reproduciendo SHALL incluir, en las tres vistas (normal, letras y visualizador), la entrada de D-pad arriba con la acción "Letras" en español o "Lyrics" en inglés, y la entrada de D-pad abajo con la acción "Visualizador" en español o "Visualizer" en inglés. En la vista de visualizador SHALL incluir además la entrada de D-pad izquierda y derecha con la acción "Estilo" en español o "Style" en inglés. Si las entradas no caben en la leyenda, SHALL quedar fuera las del final, sin que ninguna se solape con otra.

#### Scenario: Entrada de letras en la leyenda
- **WHEN** el usuario está en Reproduciendo, en la vista normal y luego en la de letras
- **THEN** en ambas la leyenda muestra el chip de D-pad arriba junto a "Letras", sin solaparse con las demás entradas

#### Scenario: Entrada de visualizador en la leyenda
- **WHEN** el usuario está en Reproduciendo, en la vista normal, luego en la de letras y luego en la de visualizador
- **THEN** en las tres la leyenda muestra el chip de D-pad abajo junto a "Visualizador", sin solaparse con las demás entradas

#### Scenario: Entrada de estilo solo en el visualizador
- **WHEN** el usuario está en Reproduciendo, primero en la vista normal y después en la de visualizador, con la interfaz en inglés
- **THEN** en la vista normal la leyenda no muestra ninguna entrada de estilo, y en la de visualizador muestra el chip de D-pad izquierda y derecha junto a "Style"
