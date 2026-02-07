# FlipChanger Development Progress

This document tracks the development progress of the FlipChanger project.

**Last Updated**: February 10, 2025

---

## Overall Status

**Current Phase**: On-device testing gate (v1.2.0)  
**Build Status**: ✅ Successful  
**Device Testing**: ⏳ Pending – v1.2.0 deployed; physical testing required  
**Memory Status**: ✅ Optimized (SD card caching)  
**Stability**: ✅ Stable (critical crashes fixed)

> **Phase gate**: Code complete for v1.2.0. TESTING_CHECKLIST.md updated. On-device testing by contributors needed before next submission steps.

---

## Completed Tasks

### ✅ Phase 1: Project Setup (Complete)
- [x] GitHub repository created
- [x] Product vision document
- [x] Development setup research
- [x] Initial documentation

### ✅ Phase 2: Development Environment (Complete)
- [x] uFBT (Micro Flipper Build Tool) installed
- [x] SDK downloaded and configured (v1.4.3)
- [x] Build system verified
- [x] Hello World test app created and deployed

### ✅ Phase 3: Core Application Structure (Complete)
- [x] Application manifest (`application.fam`)
- [x] Header file with type definitions (`flipchanger.h`)
- [x] Main application source (`flipchanger.c`)
- [x] Project structure and build configuration

### ✅ Phase 4: Basic UI and Navigation (Complete)
- [x] Main menu with 5 options (including Help)
- [x] Slot browser (supports 1-200 slots)
- [x] Slot details view
- [x] Full navigation system (UP/DOWN/OK/BACK)
- [x] Empty slot detection
- [x] UI overlap issues fixed

### ✅ Phase 5: Memory Optimization (Complete)
- [x] Reduced stack size from 4096 to 3072 bytes
- [x] Reduced JSON buffer from 4KB to 2KB (fits in stack)
- [x] Implemented SD card-based caching system
- [x] Only 10 slots cached in RAM at a time
- [x] Support for up to 200 slots (stored on SD card)
- [x] App no longer crashes due to memory issues

### ✅ Phase 6: Data Storage (Complete)
- [x] Storage API integration
- [x] JSON file reading from SD card
- [x] JSON file writing to SD card
- [x] Load slots from SD card on startup
- [x] Save slots to SD card on changes
- [x] Custom JSON parser/generator (lightweight, no external libs)
- [x] Error handling for missing/corrupted files
- [x] Data persistence verified

### ✅ Phase 7: Add/Edit Interface (Complete - Basic)
- [x] Text input for artist, album, genre, notes
- [x] Year entry (numeric input)
- [x] Character-by-character input system
- [x] Field navigation (UP/DOWN between fields)
- [x] Cursor movement (LEFT/RIGHT)
- [x] Character selection (UP/DOWN to change character)
- [x] Save functionality
- [x] Form validation and safety checks
- [x] UI overlap fixed

### ✅ Phase 8: Track Management (Complete - Basic)
- [x] Track list display in Add/Edit view
- [x] Track management view
- [x] Add new track
- [x] Delete track
- [x] Track count display
- [x] Safety checks and bounds validation

### ✅ Phase 9: Critical Bug Fixes (Complete)
- [x] Fixed NULL pointer dereference crashes
- [x] Fixed character input bug (can only add 'A')
- [x] Fixed cursor LEFT/RIGHT movement
- [x] Fixed Year field input
- [x] Fixed UI overlap (footer covering menu items)
- [x] Fixed exit crash (proper cleanup sequence)
- [x] Fixed launch crash (stack overflow - buffer size)
- [x] Comprehensive safety checks added

### ✅ Phase 10: Track Editing (Complete)
- [x] Track title editing with character input
- [x] Track duration editing (numeric only, seconds)
- [x] Field switching (LEFT/RIGHT or BACK at start/end)
- [x] Character input for track titles
- [x] Numeric input for track duration (0-9)
- [x] DEL character option in character selector
- [x] Long press BACK to exit track editing
- [x] Proper field navigation and cursor management

### ✅ Phase 11: UI/UX Improvements (Complete)
- [x] Scrollable slot details view (shows 4 items at a time)
- [x] Settings menu stub (VIEW_SETTINGS added)
- [x] Statistics menu stub (VIEW_STATISTICS added)
- [x] Year field fixed (numbers only, proper navigation)
- [x] Long press BACK support throughout app
- [x] Improved field navigation in Add/Edit view

### ✅ Phase 11.5: v1.0.2 Testing Fixes (Jan 2025)
- [x] **Help menu** – Footers removed; instructions in Help (Main menu + R key in Slot list, Details, Settings, Statistics)
- [x] **Slot list wrap-around** – Down at last slot → first; Up at first slot → last
- [x] **Long press Up/Down** – Skip by 10 slots for faster navigation
- [x] **Save fix** – Track title/duration now parsed and persisted on load (was saving blanks)
- [x] **Custom app icon** – User-created 10x10 PNG icon integrated

### ✅ Phase 11.6: Full-Screen Layout & Scroll Fix (Feb 2025)
- [x] **Full-screen menus** – Footers removed; recaptured screen space for content
- [x] **Slot list** – 5 slots visible (was 4); compact headers
- [x] **Slot details** – 4 fields visible (was 3)
- [x] **Add/Edit** – 4 fields visible (was 3)
- [x] **Track management** – 5 tracks visible (4 when editing)
- [x] **Wrap-around scroll** – Screen scrolls to show selection when wrapping (Up at slot 1→100, Down at 100→1)
- [x] **Long-press Up/Down** – Skip by 10 slots (verified working with wrap-around)

### ✅ Phase 11.7: Multi-Changer & Splash Screen (Feb 2025 – Complete)
- [x] **Multi-Changer support** – Changers: Name, Location, Total Slots; each has own slots DB
- [x] **Changer registry** – `flipchanger_changers.json`; per-Changer `flipchanger_<id>.json`
- [x] **Changer select** – Top-level Changer list; header shows Changer name; main menu scrollable (5 visible)
- [x] **Changer admin** – Add, Edit, Delete Changer; block delete if last; long-press OK to edit
- [x] **Persistence** – Save/load `last_used_id` on exit/start
- [x] **Migration** – One-time from single `flipchanger_data.json` to Changer model
- [x] **Splash screen** – Brief FlipChanger logo on launch (~1.2s or key to skip)
- [x] **First-run** – Create default Changer when none exist

See [CHANGERS_DESIGN.md](CHANGERS_DESIGN.md) for architecture.

**Bug fixes**: Main menu scroll (Help was cut off); MemMange/BusFault when backing/selecting Changer (deferred load/save, static buffer).

---

## In Progress / Needs Improvement

### 🚧 Phase 7: Add/Edit Interface (Polish Needed)
- [x] Field display scrolling for long text (implemented)
- [x] Better field navigation (improved with dual-mode input)
- [ ] Pop-out views for full-screen field editing (future enhancement)
- [x] Track editing (title/duration) - ✅ COMPLETE

### 🚧 Phase 8: Track Management (Polish Needed)
- [x] Track editing (title and duration entry) - ✅ COMPLETE
- [ ] Improved button actions (LEFT/RIGHT for add/delete could be clearer)
- [x] Better track list display (improved)

---

## Planned Features

### ✅ Phase 12: Enhanced Fields (Complete)
- [x] Add "Disc Number" field (0=unset, 1–999)
- [x] Add "Album Artist" field (for compilations/DJ sets)
- [x] JSON load/save; Add/Edit form; Slot details display

### 📋 Phase 13: Settings Menu (Partial)
- [x] Settings menu view
- [x] Slot count configuration (3-200)
- [x] Save/load settings to JSON
- [ ] Load settings on startup (uses saved file)
- [x] Slot count editing with OK to edit

### 📋 Phase 14: Statistics View (Partial)
- [x] Statistics view
- [x] Total albums count (from cached slots)
- [x] Total tracks count
- [x] Total play time (hours:minutes:seconds)
- [ ] CDs by artist/genre breakdown (future)
- [ ] Empty slots count (future)

### 📋 Phase 15: IR Integration (Future)
- [ ] Research IR database API
- [ ] Find CD changer remote codes
- [ ] Implement IR control commands
- [ ] Test with actual CD changer

### 📋 Phase 16: Polish and Testing
- [ ] Comprehensive error handling
- [ ] Input validation improvements
- [ ] UI/UX improvements
- [ ] Performance optimization
- [ ] Full device testing
- [ ] Documentation completion

### 📋 Phase 17: App Store Submission
- [ ] Compliance review
- [ ] Final testing
- [ ] Submission to Flipper Apps Catalog
- [ ] Community feedback

---

## Technical Achievements

### Memory Optimization
- **Problem**: Initial implementation allocated 200 slots × ~850 bytes = ~170KB in RAM
- **Solution**: Implemented caching system with only 10 slots in RAM (~8.5KB)
- **Result**: App runs successfully without crashes
- **Stack Optimization**: Reduced buffer sizes (4KB→2KB) to prevent stack overflow

### Build System
- **Tool**: uFBT (Micro Flipper Build Tool)
- **Status**: ✅ Installed and working
- **SDK Version**: 1.4.3 (release channel)
- **Target**: 7, API: 87.1
- **Build Time**: ~2-3 seconds
- **Deployment**: Via USB (57-115 KB/s transfer rate)

### Code Quality
- **Lines of Code**: ~3000 (main application)
- **Compilation**: ✅ No warnings/errors
- **Memory Safety**: ✅ Comprehensive bounds checking and validation
- **Error Handling**: Robust (extensive safety checks added)
- **UI/UX**: ✅ Full-screen layout, Help menu, wrap-around scroll, long press support

### Stability Improvements
- **Crash Fixes**: Multiple critical crashes resolved
- **NULL Pointer Protection**: Extensive validation added
- **Stack Overflow Prevention**: Buffer size optimization
- **Clean Exit**: Proper cleanup sequence implemented

---

## Current Statistics

| Metric | Value |
|--------|-------|
| **Total Slots Supported** | 200 |
| **Slots Cached in RAM** | 10 |
| **Default Slots** | 20 (configurable) |
| **Max Tracks per CD** | 20 (memory optimized) |
| **Stack Size** | 3072 bytes |
| **JSON Buffer Size** | 2048 bytes |
| **Build Success Rate** | 100% |
| **Device Compatibility** | Flipper Zero (all versions with API 87.1+) |

---

## Known Issues / TODO

1. **Settings Menu**: Slot count config working; load on startup uses saved file
2. **Statistics View**: Albums/tracks/time from cached slots; artist/genre breakdown future
3. **Pop-out Views**: Could add full-screen field editing views for better usability (future enhancement)
4. **Track Management**: Button actions (LEFT/RIGHT for add/delete) could be clearer

---

## Next Steps (Priority Order)

1. **Test Current Build** (Immediate)
   - Verify Phase 12 fields (Disc Number, Album Artist) in Add/Edit and Slot details
   - Test full-screen layout, Help menu, wrap-around scroll
   - Test Settings/Statistics, long press BACK and skip by 10

2. **Settings / Statistics** (High Priority)
   - Slot count configuration (3-200)
   - Save/load settings to JSON
   - Settings persistence

3. **Implement Statistics View** (High Priority)
   - Calculate total CDs count
   - Calculate CDs by artist/genre
   - Display collection statistics

4. **Polish & Testing** (Quality)
   - Comprehensive testing
   - User documentation
   - Code comments
   - App icon: ✅ Done

---

## Lessons Learned

1. **Memory Constraints**: Flipper Zero has ~64KB RAM - need careful memory management
2. **SD Card Storage**: Essential for apps with large datasets
3. **Caching Strategy**: Cache only visible/active data in RAM
4. **Build Tools**: uFBT makes development much easier than full firmware build
5. **Testing**: Always test on actual hardware - memory issues don't show in build
6. **Safety First**: Extensive bounds checking prevents crashes - always validate inputs
7. **Stack Overflow**: Large buffers on stack can cause crashes - use heap or smaller buffers

---

**Development Speed**: Excellent progress - core features implemented! App is functional and stable.
