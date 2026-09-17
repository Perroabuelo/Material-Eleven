# Dynamic Accent Specification

## Purpose

Hace que el color de acento de toda la interfaz refleje la carátula del track en reproducción, imitando el espíritu de Material You sin depender de ninguna API que Vita no expone, y sin dejar de tener un color de acento legible cuando no hay carátula disponible.

## Requirements

### Requirement: El acento de toda la interfaz refleja la carátula del track en reproducción
El sistema SHALL derivar el color de acento de la interfaz a partir del tono dominante de la carátula embebida del track actualmente en reproducción, y SHALL aplicar ese mismo color de acento por igual en las tres pantallas de nivel superior (Now Playing, Folders, Settings) y en el nav rail, incluido su indicador de pantalla activa.

#### Scenario: Comienza a reproducirse un track con carátula embebida
- **WHEN** el usuario abre un archivo con carátula embebida y comienza la reproducción
- **THEN** el color de acento de la interfaz pasa a ser el derivado de esa carátula, visible tanto en Now Playing como en Folders, Settings y el nav rail

#### Scenario: El acento se mantiene al cambiar de pantalla
- **WHEN** un track con carátula está en reproducción y el usuario cambia de pantalla usando el nav rail
- **THEN** la pantalla de destino y el propio nav rail (incluido el indicador de la pantalla activa) muestran el mismo color de acento derivado de esa carátula, no el color fijo por defecto

#### Scenario: Cambia el track en reproducción
- **WHEN** el usuario abre un track distinto, con una carátula de tono dominante diferente al anterior
- **THEN** el color de acento de la interfaz se actualiza para reflejar la carátula del nuevo track

### Requirement: Color de acento fijo cuando no hay carátula disponible
El sistema SHALL usar el color de acento fijo por defecto cuando el track en reproducción no tiene carátula embebida, y SHALL usar ese mismo color fijo cuando no hay ningún track cargado.

#### Scenario: Track sin carátula embebida
- **WHEN** el track en reproducción no tiene carátula embebida (por ejemplo, un WAV o un archivo de tracker MOD/IT/S3M)
- **THEN** el color de acento de la interfaz es el color fijo por defecto

#### Scenario: Ningún track cargado
- **WHEN** no hay ningún track cargado en la sesión de audio
- **THEN** el color de acento de la interfaz es el color fijo por defecto

### Requirement: El color derivado se mantiene legible sobre el fondo oscuro fijo
El sistema SHALL ajustar el tono dominante extraído de la carátula a un rango de saturación y luminosidad que mantenga el texto y los íconos dibujados con ese acento legibles sobre el fondo oscuro fijo de la interfaz, independientemente de qué tan oscura, clara o desaturada sea la carátula de origen.

#### Scenario: Carátula predominantemente oscura
- **WHEN** la carátula del track en reproducción es predominantemente oscura o de bajo contraste
- **THEN** el color de acento resultante mantiene suficiente luminosidad y saturación para distinguirse del fondo y del texto secundario

#### Scenario: Carátula predominantemente desaturada (escala de grises)
- **WHEN** la carátula del track en reproducción es mayormente monocromática o de muy baja saturación
- **THEN** el color de acento resultante no queda indistinguible del gris de fondo ni del texto secundario existente
