## ADDED Requirements

### Requirement: Los diálogos del sistema se componen fuera de la escena de dibujo del fotograma
El sistema SHALL componer todo diálogo provisto por el sistema operativo fuera de la escena de dibujo del fotograma de la aplicación, de modo que el diálogo se presente en pantalla y progrese según la interacción del usuario. La aplicación SHALL NOT pedir la composición de un diálogo mientras tiene una escena de dibujo abierta.

#### Scenario: Un diálogo del sistema se presenta al pedirlo
- **WHEN** la aplicación abre un diálogo provisto por el sistema mientras sigue dibujando fotogramas
- **THEN** el diálogo aparece en pantalla sobre el contenido de la aplicación

#### Scenario: Un diálogo del sistema responde y termina
- **WHEN** el usuario interactúa con un diálogo del sistema que está en pantalla
- **THEN** el diálogo progresa y termina, y la aplicación recibe su resultado y recupera el control

### Requirement: Ninguna operación que retenga el lazo de fotogramas deja la aplicación sin salida
El sistema SHALL garantizar que toda operación que retenga el lazo de fotogramas —esperar un diálogo del sistema, o cualquier trabajo que impida a la pantalla anterior seguir atendiendo al usuario— termine por sí sola en un estado utilizable aunque la condición que esperaba no llegue a cumplirse. El sistema SHALL NOT dejar como única salida que el usuario cierre el proceso desde el sistema operativo.

#### Scenario: La condición esperada no llega a cumplirse
- **WHEN** una operación que retiene el lazo de fotogramas espera una condición que no se cumple
- **THEN** la operación termina por sí sola y devuelve el control a la pantalla anterior, que vuelve a dibujarse y a responder

#### Scenario: La aplicación sigue dibujando mientras espera
- **WHEN** la aplicación está esperando el resultado de una operación que retiene el lazo de fotogramas
- **THEN** la pantalla sigue actualizándose en lugar de quedar congelada, de modo que el usuario puede ver que la aplicación sigue viva
