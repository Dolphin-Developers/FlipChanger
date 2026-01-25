# FlipChanger App

The main FlipChanger application for Flipper Zero - tracks CD metadata for CD changers (3-200 discs).

## Features Implemented

### ✅ Core Features (v1.0) - WORKING

- **Main Menu**: Navigation hub with 4 options
- **Slot Browser**: View all slots (1-200) with status and scrolling
- **Slot Details**: View CD metadata for each slot (artist, album, year, genre, tracks)
- **Navigation**: Full menu system with UP/DOWN/OK/BACK controls
- **Empty Slot Detection**: Shows which slots are empty vs occupied
- **Memory Optimization**: SD card-based caching (only 10 slots in RAM, supports 200 total)
- **JSON Storage**: Save/load data to SD card ✅ WORKING
- **Add/Edit CD**: Character-by-character input for all fields ✅ WORKING
- **Track Management**: Add/delete tracks ✅ WORKING
- **Data Persistence**: All changes saved to SD card ✅ WORKING
- **Device Testing**: ✅ App successfully deployed and running on Flipper Zero

### ✅ Recent Improvements (v1.0.1)

- **Track Editing**: ✅ Complete - Title and duration editing with character input
- **Field Display**: ✅ Scrolling implemented for long text fields
- **Scrollable Menus**: ✅ Slot details view shows 3 items at a time
- **Settings/Statistics**: ✅ Menu stubs added (functionality coming soon)
- **Footer Improvements**: ✅ Two-line footers with abbreviations (U/D, L/R, K, B, LB)
- **Year Field**: ✅ Numbers only input with proper navigation
- **Long Press BACK**: ✅ Exit functionality throughout app

### 🚧 In Progress / Needs Polish

- **Settings Menu**: Stub complete, needs full functionality
- **Statistics View**: Stub complete, needs calculation logic
- **Pop-out Views**: Full-screen field editing (future enhancement)

### 📋 Planned Features

- **Settings Menu**: Implement slot count configuration and settings persistence
- **Statistics View**: Implement collection statistics calculations and display
- **Enhanced Fields**: Add "Disc Number", split Artist into "Track Artist" and "Album Artist"
- **IR Integration**: Control CD changer via infrared

## Building the App

### Prerequisites

- uFBT installed (✅ Done!)
- Flipper Zero device (for testing)

### Build Commands

```bash
cd flipchanger-app

# Build the app
ufbt build APPID=flipchanger

# Build and deploy to device
ufbt launch APPID=flipchanger
```

### Build Output

The app is compiled into a `.fap` file:
- Location: `/Users/gilbertroberts/.ufbt/build/flipchanger.fap`
- Installed to: `/ext/apps/Tools/flipchanger.fap`
- Target: 7, API: 87.1
- Size: ~2.5KB (compact!)

### Memory Optimization

**Important**: This app uses SD card-based storage to support up to 200 slots:
- **Cache Size**: Only 10 slots kept in RAM at a time
- **Total Support**: Up to 200 slots (stored on SD card)
- **Stack Size**: 3072 bytes (optimized)
- **Memory Usage**: ~8.5KB in RAM (vs ~170KB if all slots in memory)

This allows the app to run on Flipper Zero's limited RAM (~64KB total) while supporting large CD collections.

## File Structure

```
flipchanger-app/
├── application.fam      # App manifest (required)
├── flipchanger.h        # Header file with definitions
├── flipchanger.c        # Main application source
└── README.md            # This file
```

## Usage

### Navigation

1. **Main Menu**:
   - UP/DOWN: Navigate menu items
   - OK: Select option
   - BACK: Exit app

2. **Slot List**:
   - UP/DOWN: Scroll through slots
   - OK: View slot details
   - BACK: Return to main menu

3. **Slot Details**:
   - Shows slot number and CD information
   - OK: Edit CD (if occupied) or Add CD (if empty)
   - BACK: Return to slot list

### Current Features

- ✅ View all slots in a scrollable list
- ✅ See which slots are empty vs occupied
- ✅ View basic CD information (artist, album, year, genre, tracks)
- ✅ Navigate between views

### Coming Soon

- ⏳ Add new CDs to empty slots
- ⏳ Edit existing CD metadata
- ⏳ Add track listings
- ⏳ Save data to SD card (JSON format)
- ⏳ Load saved data on startup

## Data Model

### Slot Structure

```c
typedef struct {
    int32_t slot_number;    // 1-200
    bool occupied;          // Is there a CD in this slot?
    CD cd;                  // CD metadata
} Slot;
```

### CD Structure

```c
typedef struct {
    char artist[64];
    char album[64];
    int32_t year;
    char genre[32];
    Track tracks[20];       // Track listings (reduced for memory)
    int32_t track_count;
    char notes[256];
} CD;
```

### Track Structure

```c
typedef struct {
    int32_t number;         // Track number
    char title[64];         // Track title
    char duration[16];      // Duration (e.g., "3:45")
} Track;
```

## Storage

Data is stored on the SD card at:
- Path: `/ext/apps/Tools/flipchanger_data.json`
- Format: JSON (structure ready, implementation in progress)

### Storage Architecture

- **In-Memory Cache**: 10 slots at a time (loaded on-demand)
- **SD Card Storage**: All 200 slots (JSON format)
- **Load Strategy**: Load slots from SD card when needed
- **Save Strategy**: Save to SD card when data changes

### Current Status

- ✅ Storage API integrated
- ✅ Cache management functions (structure complete)
- 🚧 JSON parsing (to be implemented)
- 🚧 JSON generation (to be implemented)

## Testing

1. **Build the app**:
   ```bash
   cd flipchanger-app
   ufbt build APPID=flipchanger
   ```

2. **Deploy to device**:
   ```bash
   ufbt launch APPID=flipchanger
   ```

3. **Test navigation**:
   - Main menu → View Slots
   - Browse through slots
   - View slot details
   - Navigate back

## Development Status

| Feature | Status |
|---------|--------|
| App Structure | ✅ Complete |
| Main Menu | ✅ Complete |
| Slot Browser | ✅ Complete |
| Slot Details View | ✅ Complete |
| Navigation | ✅ Complete |
| Add/Edit CD | 🚧 TODO |
| JSON Storage | 🚧 TODO |
| Track Management | 🚧 TODO |
| Statistics | 🚧 TODO |
| Settings | 🚧 TODO |

## Next Steps

1. **Implement Add/Edit Interface**:
   - Text input for artist, album, etc.
   - Track entry system
   - Form validation

2. **Implement JSON Storage**:
   - Save to SD card
   - Load on startup
   - Error handling

3. **Add Track Management**:
   - Add tracks to CDs
   - Edit track information
   - Track count display

4. **Polish & Testing**:
   - Test on actual hardware
   - Error handling
   - UI improvements

## Resources

- [Development Setup Guide](../docs/development_setup.md)
- [Product Vision](../docs/product_vision.md)
- [Flipper Zero Developer Docs](https://developer.flipper.net/)

---

**Status**: ✅ **Working on device!** App successfully deployed, tested, and running. Memory optimized for Flipper Zero constraints.
