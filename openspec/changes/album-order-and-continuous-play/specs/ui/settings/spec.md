## MODIFIED Requirements

### Requirement: Consolidated master-detail settings screen
The system SHALL present all settings categories (storage/device, sort order, metadata, dynamic normalizer, equalizer, language, playback) as a single list, with the currently selected category's controls shown alongside it, rather than as separate full-screen menus reached by descending through submenus.

#### Scenario: Selecting a category shows its controls without navigating away
- **WHEN** the user selects a category from the category list
- **THEN** that category's controls appear in the detail area of the same screen

#### Scenario: Language category is listed
- **WHEN** the user opens Settings on the console
- **THEN** the category list shows a language category after the equalizer, and selecting it shows the options System, English and Español as a single-choice list with the current choice marked

#### Scenario: Playback category is listed
- **WHEN** the user opens Settings on the console
- **THEN** the category list shows a playback category after the language category, every category fits on screen without overlapping the button legend, and selecting it shows the setting for what happens when an album or artist ends

### Requirement: Updating the config keeps saved settings
When the app starts with a `config.cfg` written by an earlier version, the system SHALL keep every setting that file already stores and SHALL give any setting the file lacks its default value, instead of resetting all settings to their defaults.

#### Scenario: Upgrading from the previous version
- **WHEN** the user has changed several settings on the previous version (for example, sort order Z-A, the MP3 metadata toggle off and the Jazz equalizer preset), installs this version over it and opens the app
- **THEN** those settings keep their values, and the language setting is System

#### Scenario: Upgrading from the version before the playback setting
- **WHEN** the user has changed several settings on the version that had the language setting but not the playback one (for example, the language set to English and the Pop equalizer preset), installs this version over it and opens the app
- **THEN** those settings keep their values, and "When an album or artist ends" is set to "Repeat it"

## ADDED Requirements

### Requirement: Qué hacer al terminar un álbum o un artista
La categoría de reproducción SHALL ofrecer el ajuste "Al terminar un álbum o artista" ("When an album or artist ends") como una elección única entre "Repetirlo" ("Repeat it"), que es el valor predeterminado, y "Seguir con el siguiente" ("Play the next one"). La lista de categorías SHALL mostrar el valor vigente. El efecto de cada valor sobre la cola está definido en `library/views`. Cambiar el ajuste SHALL aplicarse a la próxima reproducción que el usuario inicie desde la biblioteca, y SHALL NOT cambiar la cola de lo que ya está sonando.

#### Scenario: Valor predeterminado
- **WHEN** el usuario abre Ajustes por primera vez después de instalar esta versión
- **THEN** la categoría de reproducción muestra "Repetirlo" como valor vigente

#### Scenario: Cambiar el ajuste
- **WHEN** el usuario elige "Seguir con el siguiente", reinicia la aplicación y vuelve a Ajustes
- **THEN** la categoría de reproducción sigue mostrando "Seguir con el siguiente"

#### Scenario: El cambio no altera lo que suena
- **WHEN** suena un álbum con el ajuste en "Repetirlo", el usuario cambia a "Seguir con el siguiente" y vuelve a Reproduciendo
- **THEN** "A continuación" sigue anunciando las pistas de ese álbum, y la cola nueva se usa recién al reproducir otra vez desde la biblioteca

#### Scenario: El ajuste en los dos idiomas
- **WHEN** el usuario cambia el idioma entre English y Español y mira la categoría de reproducción
- **THEN** el ajuste y sus dos opciones aparecen en el idioma elegido
