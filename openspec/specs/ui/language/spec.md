# Language Specification

## Purpose

Define en qué idiomas se muestra la interfaz, cómo se elige el idioma activo y qué calidad deben tener sus textos, para que la app se pueda usar en inglés o en español sin mezclar idiomas en pantalla.

## Requirements

### Requirement: La interfaz está disponible en inglés y en español
El sistema SHALL ofrecer la interfaz completa en exactamente dos idiomas, English y Español. Cada texto visible de la interfaz SHALL existir en ambos idiomas.

#### Scenario: Recorrido completo en cada idioma
- **WHEN** en la consola se elige English en Ajustes > Idioma y se recorren Carpetas, Biblioteca (sus cuatro vistas, la pantalla sin carpeta elegida y un escaneo), Reproduciendo y Ajustes (todas sus categorías), y después se repite el recorrido con Español
- **THEN** en cada recorrido todos los títulos, etiquetas, hints de botones, contadores y avisos aparecen en el idioma elegido, y ninguno aparece vacío

### Requirement: El idioma inicial sigue al del sistema
Mientras la preferencia de idioma sea "Sistema", el sistema SHALL mostrar la interfaz en Español si el idioma de la consola es español, y en English con cualquier otro idioma de la consola. "Sistema" SHALL ser el valor por defecto de una instalación nueva.

#### Scenario: Consola en español
- **WHEN** la consola tiene el idioma del sistema en español y se abre la app por primera vez, sin `config.cfg`
- **THEN** la interfaz aparece en español

#### Scenario: Consola en otro idioma
- **WHEN** la consola tiene el idioma del sistema en inglés, o en cualquier otro idioma que no sea español (por ejemplo, francés o japonés), y se abre la app por primera vez, sin `config.cfg`
- **THEN** la interfaz aparece en inglés

### Requirement: El inglés es el respaldo
El sistema SHALL usar English como idioma de respaldo: si el valor de idioma guardado no corresponde a ninguna opción conocida, la interfaz SHALL mostrarse según la regla del idioma del sistema. Nunca SHALL mostrarse un texto vacío en lugar de una traducción.

#### Scenario: Valor de idioma inválido en la config
- **WHEN** se edita `config.cfg` para poner un valor de idioma fuera de rango, con la consola en inglés, y se abre la app
- **THEN** la app arranca normalmente, la interfaz aparece en inglés y Ajustes > Idioma muestra "System" seleccionado

### Requirement: El cambio de idioma se aplica al instante
El sistema SHALL aplicar un idioma nuevo en cuanto el usuario lo elige, sin reiniciar la app y sin interrumpir la reproducción. Todas las pantallas SHALL mostrarse en el idioma nuevo desde ese momento.

#### Scenario: Cambiar de idioma mientras suena música
- **WHEN** hay una pista sonando y el usuario elige otro idioma en Ajustes > Idioma
- **THEN** la pantalla de Ajustes se redibuja en el idioma nuevo en el mismo momento, la música sigue sin cortes, y al ir a Carpetas, Biblioteca y Reproduciendo esas pantallas también aparecen en el idioma nuevo

### Requirement: Los nombres de los idiomas se muestran en su propio idioma
El sistema SHALL mostrar las opciones de idioma como "English" y "Español", sin importar cuál esté activo, para que un usuario pueda reconocer el suyo aunque no entienda el idioma activo. La opción "Sistema" SHALL mostrarse en el idioma activo.

#### Scenario: Volver al propio idioma
- **WHEN** la interfaz está en inglés y el usuario abre Settings > Language
- **THEN** las opciones se leen "System", "English" y "Español"

### Requirement: El español es neutro y tiene ortografía completa
Los textos en español SHALL escribirse en español neutro: sin voseo, con tuteo en las frases dirigidas al usuario y con infinitivo en los hints de botones. SHALL llevar todos los acentos y la ñ que les correspondan.

#### Scenario: Revisión de los textos en español
- **WHEN** con la interfaz en español se recorren las mismas pantallas que en el recorrido completo
- **THEN** no aparece ninguna forma de voseo (por ejemplo, "Elegí" o "elegiste"), y palabras como "Atrás", "Triángulo", "Álbumes", "Categoría", "Continuación", "carátulas" y "dañado" aparecen con su acento o su ñ

### Requirement: Cada pantalla muestra un solo idioma
El sistema SHALL mostrar cada texto de la interfaz en el idioma activo, sin mezclar textos de otro idioma. Quedan exceptuados los nombres propios, que se muestran iguales en ambos idiomas: nombres de formatos, de presets de EQ, de dispositivos de almacenamiento y de los idiomas mismos.

#### Scenario: Normalizador en español
- **WHEN** la interfaz está en español y el usuario mira la categoría Normalizador en la lista de Ajustes
- **THEN** el valor de la categoría aparece en español, y no como "Off" u "On"
