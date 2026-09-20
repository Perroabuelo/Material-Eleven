## MODIFIED Requirements

### Requirement: Persistent navigation rail lists only available destinations
The system SHALL display a persistent navigation rail on every top-level screen, listing exactly the destinations that are implemented (Now Playing, Folders, Library, Settings). It SHALL NOT list a destination whose underlying screen or functionality does not exist.

#### Scenario: Rail matches implemented destinations
- **WHEN** the user views any top-level screen
- **THEN** the navigation rail shows entries for Now Playing, Folders, Library, and Settings only, with no entry for Playlists or a queue-management screen

#### Scenario: La biblioteca se alcanza desde cualquier pantalla
- **WHEN** el usuario está en cualquier pantalla de nivel superior y elige la entrada de biblioteca en el rail
- **THEN** la aplicación pasa a la pantalla de biblioteca, sin tener que volver antes a ninguna otra

#### Scenario: La biblioteca aparece aunque no se haya escaneado nunca
- **WHEN** el usuario no ha elegido todavía ninguna carpeta de escaneo
- **THEN** la entrada de biblioteca aparece igualmente en el rail, porque la pantalla existe y es donde se elige esa carpeta
