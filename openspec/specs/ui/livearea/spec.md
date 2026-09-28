# LiveArea Specification

## Purpose

Define cómo se presenta la app en el sistema de la PS Vita, fuera de su propia interfaz: la burbuja de la pantalla de inicio, la LiveArea y el splash de arranque, con la identidad visual de Material Eleven.

## Requirements

### Requirement: La app se llama "Material Eleven" en el sistema
El sistema SHALL mostrar la app con el nombre "Material Eleven" en todos los lugares donde la consola muestra su nombre: bajo la burbuja de la pantalla de inicio y en la LiveArea.

#### Scenario: Nombre bajo la burbuja
- **WHEN** se instala el `.vpk` en la consola y se mira la pantalla de inicio
- **THEN** la burbuja de la app lleva el nombre "Material Eleven", y no "Eleven Music Player"

### Requirement: Ícono propio legible con el recorte circular
La burbuja SHALL mostrar el ícono de Material Eleven: una nota musical clara sobre el naranja de acento fijo de la app. La nota SHALL quedar completa dentro del recorte circular que la consola aplica a la burbuja.

#### Scenario: Ícono en la pantalla de inicio
- **WHEN** se mira la burbuja de la app en la pantalla de inicio, sobre el wallpaper por defecto y sobre uno oscuro
- **THEN** se ve la nota musical blanca sobre fondo naranja, entera dentro del círculo, y no la púa rosa anterior

### Requirement: LiveArea con fondo y gate propios
La LiveArea SHALL mostrar un fondo y un gate con la paleta y la tipografía de la app, en reemplazo del degradado y del banner heredados de ElevenMPV. El gate SHALL mostrar el ícono y el logotipo MATERIAL / Eleven. Ningún elemento del fondo SHALL quedar tapado por los controles que dibuja el sistema de un modo que lo haga ilegible.

#### Scenario: Abrir la LiveArea
- **WHEN** se toca la burbuja de la app en la pantalla de inicio
- **THEN** la LiveArea muestra el fondo oscuro con las ondas concéntricas y el gate con el ícono y el logotipo, sin el banner rosa "Eleven", y el botón de inicio del sistema sigue viéndose y funcionando

### Requirement: Splash de arranque
Al iniciar la app, el sistema SHALL mostrar el splash de Material Eleven mientras la app carga, hasta que la app dibuja su primera pantalla.

#### Scenario: Iniciar la app
- **WHEN** se inicia la app desde la LiveArea
- **THEN** antes de la primera pantalla de la app aparece el splash con el logotipo y los chips de formatos, y no una pantalla en blanco o negra

### Requirement: Las imágenes cumplen el formato que exige la consola
Cada imagen del sistema que lleva el `.vpk` SHALL ser un PNG indexado de 8 bits con su tamaño exacto: ícono de 128x128, fondo de 840x500, gate de 280x158 y splash de 960x544.

#### Scenario: Verificación de las imágenes versionadas
- **WHEN** se lee el encabezado PNG (IHDR) de las cuatro imágenes de `sce_sys/`
- **THEN** las cuatro son PNG de tipo paleta (color type 3) con profundidad de 8 bits y el tamaño indicado

#### Scenario: Instalación en consola
- **WHEN** se instala el `.vpk` compilado con las imágenes nuevas usando VitaShell
- **THEN** la instalación termina sin errores, y la burbuja y la LiveArea muestran las imágenes nuevas

### Requirement: Actualizar sobre una instalación previa conserva los datos
Instalar el `.vpk` sobre una instalación anterior de la app SHALL reemplazar el nombre, el ícono, la LiveArea y el splash, y SHALL conservar la biblioteca, las carátulas y los ajustes del usuario.

#### Scenario: Instalar encima de la v3.1.0
- **WHEN** la consola tiene instalada la v3.1.0 con una biblioteca escaneada y ajustes cambiados, y se instala encima el `.vpk` nuevo
- **THEN** la burbuja y la LiveArea muestran el arte y el nombre nuevos, y al abrir la app la biblioteca y los ajustes siguen como estaban
