# List Navigation Specification

## Purpose

Define cómo se recorre la selección de las listas de la aplicación (Biblioteca, Carpetas, elegir carpeta y Ajustes) con la cruceta, incluida la repetición al mantener un botón, para que recorrer listas largas no exija una pulsación por fila.

## Requirements

### Requirement: Mantener ↑ o ↓ repite el paso en las listas
En la lista de la Biblioteca, en la del navegador de Carpetas, en la de elegir carpeta de la biblioteca y en los ítems de Ajustes, el sistema SHALL mover la selección un paso en cuanto el usuario pulsa ↑ o ↓, y, si el usuario mantiene el botón, SHALL seguir moviéndola sola en la misma dirección. Entre el primer paso y el segundo SHALL haber una pausa de entre 300 y 500 ms; después los pasos SHALL sucederse a un ritmo de entre 10 y 20 por segundo mientras el botón siga mantenido. Al soltar el botón la selección SHALL detenerse en la fila a la que haya llegado.

#### Scenario: Una pulsación corta mueve una sola fila
- **WHEN** el usuario está en la vista Canciones de la Biblioteca con la primera fila seleccionada y pulsa ↓ y lo suelta enseguida (menos de 300 ms)
- **THEN** queda seleccionada la segunda fila, y no la tercera

#### Scenario: Mantener ↓ recorre la lista sola
- **WHEN** el usuario, en una lista de Carpetas con al menos 30 entradas y la primera seleccionada, mantiene ↓ durante 1,5 s y lo suelta
- **THEN** la selección avanza varias filas sin más pulsaciones (entre 10 y 25), y deja de moverse en cuanto se suelta el botón

#### Scenario: Mantener ↑ en Ajustes
- **WHEN** el usuario, en Ajustes, en una categoría con varios ítems y el último seleccionado, mantiene ↑
- **THEN** la selección sube por los ítems sin más pulsaciones

### Requirement: Mantener el botón un rato largo salta por páginas
Si el usuario mantiene ↑ o ↓ en una de esas listas durante más de unos 2 s seguidos, el sistema SHALL pasar a mover la selección una página de filas por paso, donde una página es la cantidad de filas que esa lista muestra a la vez. El salto SHALL volver a ser de una fila en cuanto el usuario suelta el botón y lo vuelve a pulsar.

#### Scenario: Cruzar una lista larga
- **WHEN** el usuario, en la vista Canciones de una biblioteca de al menos 500 canciones y con la primera seleccionada, mantiene ↓ durante 5 s
- **THEN** la selección termina mucho más abajo que el número de pasos dados (más de 100 filas), y el contador de la lista lo refleja

#### Scenario: Soltar devuelve el paso de una fila
- **WHEN** el usuario, tras mantener ↓ hasta que la lista salta por páginas, suelta el botón y pulsa ↓ una vez
- **THEN** la selección avanza exactamente una fila

### Requirement: La repetición se detiene en el borde y solo una pulsación nueva da la vuelta
Una pulsación nueva de ↑ sobre la primera fila SHALL llevar a la última, y una de ↓ sobre la última SHALL llevar a la primera, como hasta ahora. Los pasos repetidos por mantener el botón, en cambio, SHALL detenerse en la primera o en la última fila y SHALL NOT dar la vuelta. Un salto de página que se pasaría del borde SHALL dejar la selección en esa fila del borde.

#### Scenario: Mantener ↓ se detiene en la última fila
- **WHEN** el usuario mantiene ↓ en una lista de 12 entradas de Carpetas hasta pasado el final y sigue manteniéndolo 2 s más
- **THEN** la selección queda en la última entrada y no vuelve a la primera

#### Scenario: Una pulsación nueva en el borde da la vuelta
- **WHEN** la selección quedó en la última entrada tras mantener ↓, y el usuario suelta el botón y pulsa ↓ una vez
- **THEN** la selección pasa a la primera entrada

#### Scenario: El salto de página no se pasa del borde
- **WHEN** el usuario mantiene ↓ en la vista Canciones hasta que la lista salta por páginas y la última página tiene menos filas que una página completa
- **THEN** la selección termina en la última canción, sin dar la vuelta

### Requirement: La repetición solo afecta a ↑ y ↓ en las listas
El sistema SHALL NOT repetir ninguna otra acción por mantener un botón: ←, →, L, R, confirmar, volver, △, □, SELECT y START SHALL actuar una sola vez por pulsación, en todas las pantallas. Fuera de las listas, ↑ y ↓ SHALL actuar una sola vez por pulsación; en Reproduciendo, mantener ↑ SHALL alternar la vista de letras una sola vez.

#### Scenario: Mantener ↑ en Reproduciendo
- **WHEN** el usuario, en Reproduciendo, mantiene ↑ durante 2 s
- **THEN** la vista de letras se abre una vez y queda abierta, sin abrirse y cerrarse repetidamente

#### Scenario: Mantener R en la Biblioteca
- **WHEN** el usuario, en la Biblioteca, mantiene R durante 2 s
- **THEN** pasa una sola vez a la vista siguiente

#### Scenario: Mantener confirmar sobre una canción
- **WHEN** el usuario mantiene el botón de confirmar sobre una canción de la Biblioteca durante 2 s
- **THEN** la canción empieza a sonar una sola vez, sin reiniciarse
