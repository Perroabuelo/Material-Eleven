## MODIFIED Requirements

### Requirement: Seek control
The system SHALL show the elapsed time and the track's total duration next to the progress control, and SHALL let the user seek by touching or dragging that control. El punto tocado SHALL corresponder a la misma fracción de la pista que ocupa sobre el ancho dibujado de la barra: el borde izquierdo es el inicio, el borde derecho es el final, y el relleno de la barra MUST coincidir con la posición del dedo. La zona táctil de la barra SHALL medir al menos el tamaño táctil mínimo de la app en alto.

Un toque sobre la barra la agarra. Mientras el dedo sigue apoyado, el relleno, un indicador en la posición del dedo y el tiempo transcurrido SHALL seguir la coordenada horizontal del dedo, aunque el dedo se salga de la barra en vertical, y la reproducción MUST NOT cambiar de posición. Al levantar el dedo, la reproducción SHALL saltar una sola vez a la posición del dedo en ese momento, acotada al inicio y al final de la pista. Mientras la barra está agarrada SHALL verse más gruesa y mostrar el indicador. Sin tocar, SHALL verse sin indicador, con el grosor de reposo.

La barra de la vista de letras SHALL comportarse igual. Mientras se arrastra esa barra, las letras MUST NOT moverse con el dedo; se reubican según la nueva posición al soltar.

#### Scenario: Seeking via touch
- **WHEN** con una pista de al menos tres minutos sonando en Reproduciendo, el usuario toca y suelta sin arrastrar el punto medio de la barra
- **THEN** la reproducción salta a la mitad de la pista (con un margen de un par de segundos), y el relleno llega hasta el punto tocado

#### Scenario: El borde derecho lleva al final
- **WHEN** con una pista sonando en Reproduciendo, el usuario toca y suelta el extremo derecho de la barra
- **THEN** el tiempo transcurrido queda a menos de un par de segundos de la duración total, y la pista termina y pasa a la siguiente (o se repite, si la repetición está encendida)

#### Scenario: El borde izquierdo lleva al inicio
- **WHEN** con una pista sonando a más de un minuto de su inicio, el usuario toca y suelta el extremo izquierdo de la barra
- **THEN** el tiempo transcurrido vuelve a 0:00 o 0:01

#### Scenario: Arrastrar muestra el destino y salta al soltar
- **WHEN** con una pista de al menos tres minutos sonando, el usuario apoya el dedo sobre el primer cuarto de la barra, lo arrastra hasta cerca del final sin levantarlo, y lo mantiene quieto un par de segundos
- **THEN** mientras arrastra, la barra se ve más gruesa, el indicador y el relleno siguen al dedo y el tiempo transcurrido muestra el tiempo bajo el dedo, pero el audio sigue sonando desde donde estaba
- **AND** al levantar el dedo, el audio salta a ese punto, y la barra vuelve al grosor de reposo y sin indicador

#### Scenario: El dedo se sale de la barra durante el arrastre
- **WHEN** el usuario agarra la barra, mueve el dedo hacia arriba sobre la carátula o hacia abajo sobre los controles mientras lo desliza a la derecha, y lo levanta ahí
- **THEN** el arrastre continúa hasta soltar, la reproducción salta a la posición horizontal del dedo al soltar, y no se activa ningún control de transporte

#### Scenario: Zona táctil más alta
- **WHEN** el usuario toca un punto unos 25 px por encima o por debajo de la línea de la barra, sin tocar los controles de transporte
- **THEN** el toque agarra la barra igual que si se hubiera tocado sobre la línea

#### Scenario: La barra de la vista de letras se comporta igual
- **WHEN** en la vista de letras de una pista con letras sincronizadas, el usuario arrastra la barra inferior hasta cerca del final y la suelta
- **THEN** mientras arrastra, las letras no se mueven y el tiempo transcurrido sigue al dedo
- **AND** al soltar, el audio salta a ese punto y las letras pasan a la línea que corresponde a la nueva posición
