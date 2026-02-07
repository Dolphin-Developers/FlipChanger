# Changelog

All notable changes to FlipChanger are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

---

## [Unreleased]

### Changed

- Code comments: architecture overview, section dividers, key logic notes
- App description finalized for catalog (fap_description)
- Error messages: actionable text ("Press Back", "Restart app", "Loading. Press Back.")

---

## [1.2.0] - 2025-02-10

### Added

- **Disc Number** field (0=unset, 1–999) for multi-disc sets
- **Album Artist** field for compilations and DJ sets
- Add/Edit form fields: Artist, Album Artist, Album, Disc #, Year, Genre, Notes, Tracks
- Slot details view shows album artist, disc number, and notes when set
- App manifest metadata for App Store: `fap_version`, `fap_author`, `fap_description`

### Changed

- CD data model extended with `album_artist` and `disc_number`
- JSON storage format includes new fields (backward compatible)

---

## [1.1.0] - 2025-02-07

### Added

- **Multi-Changer support**: Name, Location, Total Slots per changer
- Changer list at top menu; header shows current changer name
- Changer admin: Add, Edit, Delete (block delete if last)
- Per-changer slot databases (`flipchanger_<id>.json`)
- Migration from legacy single-file `flipchanger_data.json`
- Splash screen on launch (~1.2s or key to skip)
- Main menu scrollable (5 visible); 6 items including Changers, Help

### Changed

- Changer registry stored in `flipchanger_changers.json`
- Last-used changer persisted across sessions

---

## [1.0.3] - 2025-02

### Added

- Full-screen layout: 5 slots, 4 fields, 5 tracks visible
- Help menu with navigation instructions
- Wrap-around scroll for slot list
- Long-press Up/Down to skip 10 slots
- Custom 10×10 app icon

---

## [1.0.2] - 2025-01

### Added

- Long-press Back support throughout app
- Track title and duration parsing on load (save fix)

### Fixed

- Slot list wrap-around navigation
- Help menu accessibility

---

## [1.0.0] - 2024

### Added

- Core CD tracking for changers (3–200 slots)
- Slot browser and slot details view
- Add/Edit CD: artist, album, year, genre, notes
- Track management: add, delete, edit title and duration
- JSON storage on SD card
- SD card-based caching (10 slots in RAM)
- Settings: slot count configuration (3–200)
- Statistics: albums, tracks, play time (from cached slots)
