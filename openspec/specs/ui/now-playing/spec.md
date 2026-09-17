# Now Playing Specification

## Purpose

Presents the currently playing track and its transport controls in the app's shared visual language, while preserving every existing playback behavior and not implying queue-management capability the app does not yet have.

## Requirements

### Requirement: Track identity and format display
The system SHALL display the current track's title and artist when embedded metadata provides them, falling back to the filename when it does not (matching existing fallback behavior), plus a badge naming the file's format derived from its extension (for example FLAC, MP3, OPUS, WAV).

#### Scenario: Metadata available
- **WHEN** a track with embedded title and artist metadata is playing
- **THEN** the screen shows that title and artist, and a badge naming the file's format

#### Scenario: Metadata unavailable
- **WHEN** a track without embedded metadata is playing
- **THEN** the screen shows the filename in place of title and artist, matching current fallback behavior

### Requirement: Playback transport controls
The system SHALL provide play/pause, previous, next, shuffle-toggle, and repeat-toggle controls, each preserving its current effect on playback order and state.

#### Scenario: Toggling shuffle
- **WHEN** the user activates the shuffle control
- **THEN** subsequent "next" actions pick a random remaining track, matching current shuffle behavior

#### Scenario: Toggling repeat
- **WHEN** the user activates the repeat control
- **THEN** the current track repeats when it ends, matching current repeat behavior

### Requirement: Seek control
The system SHALL show elapsed and remaining time and allow the user to seek to a position by touching the progress control, matching current seek behavior.

#### Scenario: Seeking via touch
- **WHEN** the user touches a point along the progress control
- **THEN** playback position updates to correspond to that point

### Requirement: Read-only upcoming-tracks preview
The system SHALL show a short preview of the upcoming track(s) in the current playlist's existing order, without offering controls to reorder, remove, or otherwise edit that order.

#### Scenario: Preview reflects playlist order
- **WHEN** the current folder's playlist has more than one remaining track
- **THEN** the screen shows the upcoming track(s) in playback order, with no control to reorder or remove them

### Requirement: No dead-end queue-management control
The system SHALL NOT present any control that claims to open a full queue-management view (reordering, removing individual tracks, clearing the queue), since that capability does not exist.

#### Scenario: No control opens a nonexistent screen
- **WHEN** the user views Now Playing
- **THEN** no button or control is shown that would open a queue-management screen
