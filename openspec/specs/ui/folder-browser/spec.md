# Folder Browser Specification

## Purpose

Lets the user navigate the device's file system to find and play audio files, presented in the app's shared visual language, without introducing a cross-folder index or search.

## Requirements

### Requirement: Directory listing with format badges
The system SHALL list the current directory's folders and files, showing a format badge on entries with a recognized audio extension (flac, it, mod, mp3, ogg, opus, s3m, wav, xm), matching the extensions currently recognized for playback.

#### Scenario: Recognized audio file is badged
- **WHEN** the current directory contains a file with a recognized audio extension
- **THEN** its row shows a badge naming that format

#### Scenario: Unrecognized file has no format badge
- **WHEN** the current directory contains a file whose extension is not recognized for playback
- **THEN** its row shows no format badge

### Requirement: Directory navigation
The system SHALL let the user enter a subfolder and return to the parent folder, and SHALL NOT allow navigating above the currently configured root.

#### Scenario: Entering a subfolder
- **WHEN** the user selects a folder entry
- **THEN** the screen shows that folder's contents

#### Scenario: Returning to the parent folder
- **WHEN** the user is below the configured root and activates the "go to parent" action
- **THEN** the screen shows the parent folder's contents

#### Scenario: Root is a navigation boundary
- **WHEN** the user is viewing the configured root folder
- **THEN** no "go to parent" action is available

### Requirement: Opening a file
The system SHALL begin playback and show the Now Playing screen when the user selects a file with a recognized audio extension, and SHALL take no playback action when the user selects a file whose extension is not recognized.

#### Scenario: Opening a recognized audio file
- **WHEN** the user selects a file with a recognized audio extension
- **THEN** the app begins playback of that file and shows the Now Playing screen

#### Scenario: Selecting an unrecognized file
- **WHEN** the user selects a file whose extension is not recognized for playback
- **THEN** no playback begins

### Requirement: In-folder filename filter
The system SHALL let the user filter the current directory's already-loaded listing by typing part of a name. The filter SHALL apply only to entries already listed in the current folder and SHALL NOT trigger reading any other folder.

#### Scenario: Filtering narrows the current listing
- **WHEN** the user types a search term while viewing a folder's contents
- **THEN** only entries in that folder whose names contain the term remain visible

#### Scenario: Filter does not reach into subfolders
- **WHEN** the user types a search term
- **THEN** entries inside subfolders of the current folder are not searched or shown as results

### Requirement: Docked mini-player while browsing
The system SHALL show a persistent mini-player, reflecting the currently playing track's title, artist, and basic transport controls, while the user browses folders, without interrupting playback.

#### Scenario: Mini-player reflects active playback while browsing
- **WHEN** a track is playing and the user navigates the folder browser
- **THEN** a mini-player showing that track's title, artist, and transport controls remains visible and playback continues uninterrupted
