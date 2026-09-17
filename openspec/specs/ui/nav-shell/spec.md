# Nav Shell Specification

## Purpose

Gives the user a persistent, always-visible way to switch between the app's top-level screens, replacing the current model where each screen is a dead-end reachable only by exiting the previous one or knowing a specific physical-button shortcut.

## Requirements

### Requirement: Persistent navigation rail lists only available destinations
The system SHALL display a persistent navigation rail on every top-level screen, listing exactly the destinations that are implemented (Now Playing, Folders, Settings). It SHALL NOT list a destination whose underlying screen or functionality does not exist.

#### Scenario: Rail matches implemented destinations
- **WHEN** the user views any top-level screen
- **THEN** the navigation rail shows entries for Now Playing, Folders, and Settings only, with no entry for Library, Playlists, or a queue-management screen

### Requirement: Selecting a rail entry switches the active screen
The system SHALL switch to a destination's screen when the user selects its entry in the rail, without requiring a separate back/cancel action first.

#### Scenario: Switching from Folders to Settings via the rail
- **WHEN** the user is viewing Folders and selects the Settings entry in the rail
- **THEN** the app switches to the Settings screen

### Requirement: Rail indicates the active destination
The system SHALL visually distinguish the entry for the currently active screen from the other entries in the rail.

#### Scenario: Active entry is distinguishable
- **WHEN** the user is viewing the Folders screen
- **THEN** the Folders entry in the rail is visually marked as active and the other entries are not

### Requirement: Existing physical-button navigation still works
The system SHALL preserve all physical-button navigation behavior that existed before this change (for example, SELECT opening Settings from the folder browser, START exiting the app, and the cancel button returning to the previous screen), alongside the new rail-based navigation.

#### Scenario: SELECT still opens Settings
- **WHEN** the user presses SELECT while viewing Folders
- **THEN** the app opens Settings, the same outcome as before this change

### Requirement: On-screen legend of available physical-button actions
The system SHALL display, on every screen, a legend of the physical-button actions currently available on that screen.

#### Scenario: Legend reflects the current screen
- **WHEN** the user is viewing the Folders screen
- **THEN** the legend lists the actions available there (such as open/play and go to parent folder) and not actions that only apply to a different screen
