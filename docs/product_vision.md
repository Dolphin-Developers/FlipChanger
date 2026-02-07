# FlipChanger - Product Vision Document

## 1. Executive Summary

FlipChanger is an open-source application for the Flipper Zero device that helps users track physical CDs loaded into a CD changer system (supporting 3-200 discs). The application stores metadata similar to MP3 ID3 tags (artist, album, track listings) and associates this information with specific disc slots/positions. FlipChanger serves as both a practical utility and a learning project to understand the full application development lifecycle for Flipper Zero, including compliance with the official app store.

## 2. Purpose & Goals

### Primary Purpose
Enable Flipper Zero users to manage and track their physical CD collection when using a CD changer, storing metadata directly on the device for quick reference without needing a computer or mobile app.

### Learning Objectives
- Understand Flipper Zero application architecture
- Learn the Flipper Zero app store submission process
- Gain experience with repository management, version control, and open-source practices
- Explore Flipper Zero's infrared remote capabilities and existing IR database integration
- Build a foundation for future, more complex Flipper Zero applications

## 3. Target Audience

- **Primary Users**: Flipper Zero enthusiasts who own physical CD changers and want to track their CD collections
- **Secondary Users**: Developers/learners interested in Flipper Zero app development examples
- **Use Cases**:
  - Quick reference: "What CD is in slot 15?"
  - Collection management: Adding new CDs to the changer
  - Metadata lookup: Viewing track listings without ejecting discs

## 4. Key Features

### Core Features (v1.0)

| Feature | Description | Priority |
|---------|-------------|----------|
| **Slot Management** | Track which CDs are in which slots (positions 1-200). Support for 3-disc minimum, 200-disc maximum changers. | High |
| **Metadata Storage** | Store CD metadata per slot: Artist, Album, Year, Genre, Track listing (title, duration), Notes/Description | High |
| **View & Browse** | Navigate slots, view slot status (empty/occupied), display full metadata for loaded CDs | High |
| **Add/Edit CDs** | User-friendly interface to add or edit CD information for any slot | High |
| **Data Persistence** | Save all data to SD card or internal storage, survive device reboots | High |
| **Slot Status Indicators** | Quick visual indication of which slots are filled vs. empty | Medium |

### Enhanced Features (Future)

| Feature | Description | Priority |
|---------|-------------|----------|
| **Multi-Changer Support** | Multiple Changers (Name, Location, Slots); select at top menu; persist last-used; per-Changer data. See [CHANGERS_DESIGN.md](CHANGERS_DESIGN.md) | High |
| **Splash Screen** | Brief FlipChanger logo on app launch | Medium |
| **IR Remote Integration** | Utilize Flipper Zero's infrared capabilities to control CD changer (play, pause, eject, select slot) using existing IR database | Medium |
| **Import/Export** | Import metadata from JSON/CSV files (via SD card or Flipper Lab), export for backup | Medium |
| **Search Function** | Search CDs by artist, album, or track name across all slots | Low |
| **Statistics** | View collection stats: total discs, by artist, by genre, etc. | Low |

## 5. Technical Specifications

### Platform & Language
- **Primary**: C (native Flipper Zero application) - recommended for performance and full feature access
- **Alternative Consideration**: JavaScript (via Flipper's mJS engine) - easier for iteration and learning

### Storage Architecture
- **Data Format**: JSON files stored on SD card or internal storage
- **Storage Location**: App-specific data folder (compliant with Flipper Zero app guidelines)
- **File Structure** (current → planned):
  - `flipchanger_data.json` - Current single-changer database (legacy)
  - **Planned**: `flipchanger_changers.json` - Changer registry + last_used_id
  - **Planned**: `flipchanger_<id>.json` - Per-Changer slots database

### User Interface Constraints
- **Display**: 128x64 pixel monochrome LCD
- **Navigation**: D-pad (up/down/left/right) + Back button + OK/Select
- **Design Principles**: 
  - Simple, clear menu hierarchy
  - Minimal text, maximize information density
  - Scrollable lists for slots
  - Form-based data entry

### Data Model

```json
{
  "version": "1.0",
  "changer_capacity": 100,
  "slots": [
    {
      "slot_number": 1,
      "occupied": true,
      "cd": {
        "artist": "Artist Name",
        "album": "Album Title",
        "year": 2023,
        "genre": "Rock",
        "tracks": [
          {"number": 1, "title": "Track One", "duration": "3:45"},
          {"number": 2, "title": "Track Two", "duration": "4:12"}
        ],
        "notes": "Optional notes"
      }
    },
    {
      "slot_number": 2,
      "occupied": false,
      "cd": null
    }
  ]
}
```

### Infrared Integration (Optional)
- Leverage existing Flipper Zero IR database for CD changer remotes
- Potential commands: Play, Pause, Stop, Eject, Select Slot (1-N), Next/Previous Track
- Research phase: Identify common CD changer IR codes in Flipper database

## 6. App Store Compliance

### Requirements Checklist
- [ ] Follow Flipper Zero app manifest format (FAM file or equivalent)
- [ ] Proper app metadata (name, description, version, author)
- [ ] Comply with Flipper branding/trademark guidelines
- [ ] Code follows Flipper Zero coding standards
- [ ] App icon/assets meet specifications
- [ ] Documentation: README with build instructions
- [ ] License file (open-source license - suggest MIT or Apache 2.0)

### Resources
- Flipper Zero Developer Documentation
- App Publishing Guidelines
- Compliance Documentation

## 7. Success Metrics

### Functional Requirements
- ✅ Users can list all slots (1-200)
- ✅ Users can add/edit CD metadata for any slot
- ✅ Data persists across device reboots
- ✅ App runs without crashes on Flipper Zero hardware
- ✅ Acceptable performance (< 2 second load times, smooth navigation)

### Learning Goals
- ✅ Repository successfully published to GitHub
- ✅ App accepted in Flipper Zero app store
- ✅ Experience with version control, issues, pull requests
- ✅ Understanding of Flipper Zero app architecture
- ✅ Foundation for future app development

## 8. Out of Scope (v1.0)

- Automatic CD detection via hardware sensors
- Cover art/image display (display constraints)
- Audio playback capabilities
- Network/cloud sync features
- Complex IR automation beyond basic remote control
- Multi-language support (English only for v1)

## 9. Development Phases

### Phase 1: Foundation
- Set up repository structure
- Create product vision document
- Research Flipper Zero app development setup
- Design data model

### Phase 2: Core Features
- Implement storage system (read/write JSON)
- Build basic UI navigation
- Slot listing and status display
- View metadata functionality

### Phase 3: Data Management
- Add CD functionality
- Edit CD functionality
- Delete/clear slot functionality
- Data validation

### Phase 4: Polish & Compliance
- UI/UX improvements
- Error handling
- App store compliance checks
- Documentation completion

### Phase 5: Optional Enhancements
- IR remote integration
- Import/export functionality
- Search features

## 10. Open Source Considerations

- **License**: MIT License (permissive, allows commercial use, easy contribution)
- **Contributing**: Clear CONTRIBUTING.md with guidelines
- **Code of Conduct**: Consider adding if project grows
- **Issue Templates**: Bug reports, feature requests
- **Documentation**: README, API docs, user guide

## 11. Implementation Status

### ✅ Completed
- Development environment setup (uFBT installed and working)
- Core application structure (C/C++ with proper manifest)
- Main menu and navigation system
- Slot browser (supports 1-200 slots)
- Slot details view
- Memory optimization (SD card caching system)

### 🚧 In Progress
- JSON storage implementation (structure ready)
- Add/Edit CD interface

### 📋 Planned
- Track management
- Settings menu
- Statistics view
- IR integration

## 12. Risks & Mitigations

| Risk | Mitigation | Status |
|------|------------|--------|
| Limited Flipper Zero development experience | Start simple, iterate, use examples from official repo | ✅ Mitigated - Learning through hello world app |
| Storage limitations | Optimize JSON structure, implement efficient file handling | ✅ Mitigated - SD card caching implemented |
| Memory constraints | Only cache 10 slots in RAM, store rest on SD card | ✅ Solved - App runs without crashes |
| UI complexity on small display | Prioritize core features, keep UI simple | ✅ Mitigated - Simple, scrollable lists |
| IR integration complexity | Research existing IR database first, mark as optional | 📋 Planned |
| App store rejection | Follow guidelines closely, review similar accepted apps | 📋 Future |

## 12. Future Vision

Beyond v1.0, FlipChanger could evolve into:
- **Multi-Changer support** (Next phase): Multiple Changers (Name, Location, Slots); select at top menu; persist last-used; per-Changer slots database. See [CHANGERS_DESIGN.md](CHANGERS_DESIGN.md).
- Barcode/QR code scanning for CD lookup
- Integration with music databases (Discogs, MusicBrainz)
- Companion mobile/web app for easier data entry
- Community-driven IR code contributions

---

**Document Version**: 1.0  
**Last Updated**: 2024  
**Author**: FlipChanger Development Team
