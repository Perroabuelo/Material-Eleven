## MODIFIED Requirements

### Requirement: Consolidated master-detail settings screen
The system SHALL present all settings categories (storage/device, sort order, metadata, dynamic normalizer, equalizer, language) as a single list, with the currently selected category's controls shown alongside it, rather than as separate full-screen menus reached by descending through submenus.

#### Scenario: Selecting a category shows its controls without navigating away
- **WHEN** the user selects a category from the category list
- **THEN** that category's controls appear in the detail area of the same screen

#### Scenario: Language category is listed
- **WHEN** the user opens Settings on the console
- **THEN** the category list shows a language category after the equalizer, and selecting it shows the options System, English and Español as a single-choice list with the current choice marked

## ADDED Requirements

### Requirement: Updating the config keeps saved settings
When the app starts with a `config.cfg` written by an earlier version, the system SHALL keep every setting that file already stores and SHALL give any setting the file lacks its default value, instead of resetting all settings to their defaults.

#### Scenario: Upgrading from the previous version
- **WHEN** the user has changed several settings on the previous version (for example, sort order Z-A, the MP3 metadata toggle off and the Jazz equalizer preset), installs this version over it and opens the app
- **THEN** those settings keep their values, and the language setting is System
