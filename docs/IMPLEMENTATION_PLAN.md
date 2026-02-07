# FlipChanger Implementation Plan
## Features to Implement in Priority Order

**Created**: January 18, 2024  
**Purpose**: Maximize project efficiency by implementing features in optimal dependency order

---

## Priority Order (Based on Dependencies)

### 1. JSON Storage System ✅ NEXT
**Why First**: Foundation for all data persistence
**Dependencies**: None
**Effort**: Medium

**Tasks**:
- [ ] Implement `flipchanger_load_data()` - Parse JSON from SD card
- [ ] Implement `flipchanger_save_data()` - Write JSON to SD card
- [ ] Implement `flipchanger_load_slot_from_sd()` - Load individual slot
- [ ] Implement `flipchanger_save_slot_to_sd()` - Save individual slot
- [ ] Error handling for missing/corrupted files
- [ ] Test save/load cycle

**Files to Modify**:
- `flipchanger.c` - Storage functions

**Approach**: Simple manual JSON parsing for our specific structure (no complex JSON library needed)

---

### 2. Add/Edit CD Interface ✅ AFTER STORAGE
**Why Second**: Core feature, depends on storage
**Dependencies**: JSON Storage (#1)
**Effort**: High

**Tasks**:
- [ ] Text input system for artist/album
- [ ] Year entry (numeric input)
- [ ] Genre entry (text input)
- [ ] Track entry system
  - [ ] Add track
  - [ ] Edit track title
  - [ ] Edit track duration
  - [ ] Delete track
- [ ] Notes/description field
- [ ] Form navigation (UP/DOWN between fields)
- [ ] Data validation
- [ ] Save button (write to storage)

**Files to Modify**:
- `flipchanger.c` - Add/Edit view and input handling
- `flipchanger.h` - Add input state structure

**UI Flow**:
1. User selects slot → Shows Add/Edit view
2. Navigate fields with UP/DOWN
3. Edit field with keyboard input (if supported) or character-by-character
4. Save with OK button
5. Return to slot details

---

### 3. Track Management ✅ AFTER ADD/EDIT
**Why Third**: Enhancement to Add/Edit interface
**Dependencies**: Add/Edit Interface (#2)
**Effort**: Medium

**Tasks**:
- [ ] Track list display in Add/Edit view
- [ ] Add new track
- [ ] Edit existing track
- [ ] Delete track
- [ ] Reorder tracks (optional)
- [ ] Track count display

**Files to Modify**:
- `flipchanger.c` - Track management functions

---

### 4. Settings Menu ✅ POLISH
**Why Fourth**: Non-critical, adds configuration
**Dependencies**: Storage (#1) - to save settings
**Effort**: Low-Medium

**Tasks**:
- [ ] Settings menu view
- [ ] Slot count configuration (3-200)
- [ ] Save settings to JSON
- [ ] Load settings on startup

**Files to Modify**:
- `flipchanger.c` - Settings view
- `flipchanger.h` - Settings structure

---

### 5. Statistics View ✅ POLISH
**Why Fifth**: Nice-to-have feature
**Dependencies**: Storage (#1) - to count slots
**Effort**: Low

**Tasks**:
- [ ] Statistics view
- [ ] Total CDs count
- [ ] CDs by artist (count)
- [ ] CDs by genre (count)
- [ ] Empty slots count
- [ ] Collection size percentage

**Files to Modify**:
- `flipchanger.c` - Statistics calculation and display

---

### 6. Multi-Changer & Splash Screen (NEXT – Design Complete)
**Why Now**: Critical roadmap jump-in before further feature work; users may have multiple CD changers
**Dependencies**: Storage (#1), Settings (#4) – slot count moves into Changer model
**Effort**: High

**Tasks**:
- [ ] Changer registry: `flipchanger_changers.json` (list + last_used_id)
- [ ] Per-Changer slots: `flipchanger_<id>.json` per Changer
- [ ] Migration: one-time from `flipchanger_data.json` to Changer model
- [ ] Changer list view: select Changer; header shows Changer name
- [ ] Changer admin: Add, Edit, Delete Changer
- [ ] Persist last-used Changer on exit; load on startup
- [ ] First-run wizard when no Changers exist
- [ ] Splash screen on launch (brief FlipChanger logo)

**Files to Modify**:
- `flipchanger.h` – Changer struct, registry, views
- `flipchanger.c` – Changer load/save, UI, migration

**Design**: See [CHANGERS_DESIGN.md](CHANGERS_DESIGN.md)

---

## Implementation Approach

### JSON Storage Strategy

**Format**: Simple JSON structure optimized for manual parsing
```json
{
  "version": 1,
  "total_slots": 20,
  "slots": [
    {
      "slot": 1,
      "occupied": true,
      "artist": "Artist Name",
      "album": "Album Title",
      "year": 2023,
      "genre": "Rock",
      "tracks": [
        {"num": 1, "title": "Track 1", "duration": "3:45"},
        {"num": 2, "title": "Track 2", "duration": "4:12"}
      ],
      "notes": "Optional notes"
    }
  ]
}
```

**Parsing Strategy**:
- Simple token-based parser (no full JSON library)
- Read file in chunks to save memory
- Parse slots one at a time
- Store parsed slots to SD card if needed

**Writing Strategy**:
- Build JSON string incrementally
- Write directly to file
- Use string formatting functions

---

### Text Input Strategy

**Constraints**: Flipper Zero has no keyboard, limited input options

**Approach**:
1. **Character Selection**: Use UP/DOWN to select character
2. **Field Navigation**: LEFT/RIGHT or special button to move between fields
3. **Character Set**: A-Z, 0-9, space, common punctuation
4. **Visual Feedback**: Show cursor position

**Alternative** (if complex):
- Use numeric input for year
- Predefined lists for genre (if small)
- Simplified input for artist/album (first letter selection)

---

## Testing Strategy

1. **Unit Testing**:
   - Test JSON parsing with sample file
   - Test JSON generation with sample data
   - Test save/load cycle

2. **Integration Testing**:
   - Test full workflow: Add CD → Save → Reload → View
   - Test with empty slots
   - Test with full slots

3. **Device Testing**:
   - Test on actual Flipper Zero hardware
   - Test with SD card inserted/removed
   - Test with corrupted data file

---

## Success Criteria

### JSON Storage (#1) - DONE WHEN:
- ✅ Can save slots to SD card
- ✅ Can load slots from SD card
- ✅ Data persists after app restart
- ✅ Handles missing file gracefully
- ✅ Handles corrupted file gracefully

### Add/Edit Interface (#2) - DONE WHEN:
- ✅ Can add new CD to empty slot
- ✅ Can edit existing CD
- ✅ All fields can be edited
- ✅ Changes save to storage
- ✅ UI is usable on 128x64 display

### Track Management (#3) - DONE WHEN:
- ✅ Can add tracks to CD
- ✅ Can edit track information
- ✅ Can delete tracks
- ✅ Track count updates correctly

### Settings (#4) - DONE WHEN:
- ✅ Can configure slot count
- ✅ Settings persist across sessions
- ✅ App adapts to new slot count

### Statistics (#5) - DONE WHEN:
- ✅ Shows accurate counts
- ✅ Updates when CDs added/removed
- ✅ Display is readable

---

## Estimated Timeline

| Feature | Estimated Effort | Priority |
|---------|------------------|----------|
| JSON Storage | 2-3 hours | Critical |
| Add/Edit Interface | 3-4 hours | Critical |
| Track Management | 2-3 hours | High |
| Settings | 1-2 hours | Medium |
| Statistics | 1 hour | Low |

**Total Estimated Time**: 9-13 hours for all features

---

## Next Steps

1. ✅ **JSON Storage** – Complete
2. ✅ **Add/Edit, Track Management, Settings, Statistics** – Complete
3. **Implement Multi-Changer & Splash** – See [CHANGERS_DESIGN.md](CHANGERS_DESIGN.md)
4. **Test Changer migration** – Verify existing data migrates correctly
5. **Polish & Deploy** – Final touches

---

**Current Status**: Multi-Changer design complete; ready for implementation.
