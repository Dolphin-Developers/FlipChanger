# FlipChanger Multi-Changer Design

**Created**: February 7, 2025  
**Status**: Design Complete – Ready for Implementation  
**Priority**: Critical (roadmap jump-in before further feature work)

---

## 1. Overview

FlipChanger will support **multiple CD Changers**. Each Changer is a simple entity (Name, Location, Total Slots) with its own slots/tracks database. The user selects a Changer at the top level; that selection persists across app restarts. All existing slot/CD/track functionality operates within the selected Changer.

---

## 2. Changer Model

| Field         | Type   | Constraints   | Description                    |
|---------------|--------|---------------|--------------------------------|
| **id**        | string | unique, stable| Used for file names and lookup |
| **name**      | string | 1–32 chars    | Display name (e.g., "Living Room") |
| **location**  | string | 0–32 chars    | Optional (e.g., "Basement")    |
| **total_slots** | int  | 3–200         | Slot count for this Changer    |

- **id**: Generate on create (e.g., `changer_0`, `changer_1` or timestamp-based). Never change on edit.
- **name**: Shown in header and Changer list.
- **location**: Optional context; shown in Changer list.
- **total_slots**: Per-Changer; moved from global Settings into Changer definition.

---

## 3. Data Structure

### Changers Registry

**File**: `flipchanger_changers.json`

```json
{
  "version": "1.0",
  "last_used_id": "changer_0",
  "changers": [
    {
      "id": "changer_0",
      "name": "Living Room",
      "location": "Basement",
      "total_slots": 100
    },
    {
      "id": "changer_1",
      "name": "Garage",
      "location": "",
      "total_slots": 50
    }
  ]
}
```

- **last_used_id**: Changer selected when user last exited; load this on startup.
- **changers**: List of all Changers. At least one required.

### Per-Changer Slots Data

**File**: `flipchanger_<id>.json` (e.g., `flipchanger_changer_0.json`)

- Same structure as current `flipchanger_data.json`.
- One file per Changer; contains only that Changer’s slots and CD metadata.
- Existing single-file behavior maps to `flipchanger_changer_0.json` when migrating.

---

## 4. UI Flow

### 4.1 App Launch

1. **Splash screen** (brief) → app logo / FlipChanger branding.
2. Load `flipchanger_changers.json`.
3. If no Changers → **First-run wizard** → Create first Changer (Name, Location, Total Slots).
4. If Changers exist → Load `last_used_id`; if invalid → use first Changer.
5. Load slots for selected Changer from `flipchanger_<id>.json`.
6. Show Main Menu with Changer name in header.

### 4.2 Main Menu (when Changer selected)

- **Header**: Changer name (e.g., "Living Room") instead of "FlipChanger".
- **Options** (unchanged functionally):
  1. Slot List
  2. Add CD
  3. Settings
  4. Statistics
  5. **Changers** (new) – switch/add/edit/delete Changers
  6. Help

- User stays on the selected Changer until they choose **Changers** and switch.

### 4.3 Changers View (Admin)

- **Changer list**: Name, Location, Slots (e.g., "Living Room | Basement | 100").
- **Navigation**: Up/Down, OK to select, BACK to return.
- **On select**: Switch to that Changer, return to Main Menu.
- **Long-press or submenu**: Add Changer, Edit Changer, Delete Changer, Help.
- **Add Changer**: Form (Name, Location, Total Slots 3–200) → create entry and empty slots file.
- **Edit Changer**: Same form; id unchanged.
- **Delete Changer**: Confirm; block if it’s the last Changer.
- **Help**: Context help for Changer admin.

### 4.4 Exit / Persistence

- On exit (long-press BACK): Save `last_used_id` to `flipchanger_changers.json`.
- On next launch: Load `last_used_id` and corresponding Changer.

---

## 5. Migration from Single-Changer

- If `flipchanger_changers.json` missing but `flipchanger_data.json` exists:
  1. Create default Changer (e.g., id `changer_0`, name "Default", slots from settings or file).
  2. Rename/copy `flipchanger_data.json` → `flipchanger_changer_0.json`.
  3. Create `flipchanger_changers.json` with this Changer and `last_used_id`.
  4. Optionally remove or archive `flipchanger_data.json`.

---

## 6. Edge Cases

| Case                   | Behavior                                                      |
|------------------------|---------------------------------------------------------------|
| No Changers            | First-run wizard; must create one.                            |
| Delete last Changer    | Disallow; require at least one.                               |
| Invalid `last_used_id` | Fall back to first Changer in list.                           |
| Changer file missing   | Treat as empty slots; allow usage.                            |
| Edit total_slots down  | Truncate/warn if slots beyond new count have data (future).   |
| Rename Changer         | id unchanged; only name/location/slots updated.               |

---

## 7. Splash Screen

- **When**: On app launch, before Changer load.
- **Content**: FlipChanger logo/name, short (e.g., 1–2 seconds).
- **Implementation**: Simple full-screen draw; optional timer or key press to skip.

---

## 8. Implementation Order

1. **Data layer**: Changer registry struct, load/save `flipchanger_changers.json`.
2. **Per-Changer files**: Load/save `flipchanger_<id>.json`; refactor from single file.
3. **Migration**: One-time migration from `flipchanger_data.json` if present.
4. **UI – Changer select**: Changer list view, select and set current Changer.
5. **UI – Header**: Show Changer name instead of "FlipChanger".
6. **UI – Changer admin**: Add, Edit, Delete Changer.
7. **Persistence**: Save/load `last_used_id` on exit/start.
8. **Splash screen**: Add after Changer system is stable.

---

## 9. Files to Modify

| File              | Changes                                                                 |
|-------------------|-------------------------------------------------------------------------|
| `flipchanger.h`   | Changer struct, registry, current_changer_id, VIEW_CHANGERS, etc.       |
| `flipchanger.c`   | Changer load/save, migration, Changer list UI, admin, header text       |
| `docs/PROGRESS.md`| Add Phase 11.7 (Changers + Splash)                                      |
| `README.md`       | Document Multi-Changer and Splash in roadmap                            |

---

## 10. Out of Scope (This Phase)

- Changer reordering
- Import/export per Changer
- Changer backup/restore
- Multiple changers in one physical device (each Changer = one logical changer)

---

**Document Version**: 1.0  
**Next Step**: Implement data layer and migration, then Changer select UI.
