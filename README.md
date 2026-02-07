# FlipChanger

FlipChanger is an open-source application for the Flipper Zero that helps users track physical CDs loaded into a CD changer system (3-200 discs). Store metadata like artist, album, and track listings directly on your Flipper Zero, similar to MP3 ID3 tags.

## Features

- **Slot Management**: Track which CDs are in which slots (1-200)
- **Metadata Storage**: Store artist, album, year, genre, and track listings for each CD
- **View & Browse**: Navigate your collection easily on the Flipper Zero display
- **Add/Edit CDs**: Manage your collection directly from the device
- **Data Persistence**: All data saved to SD card, survives reboots
- **Multi-Changer**: Multiple CD changers (Name, Location, Slots); switch at top menu; per-Changer data
- **IR Remote Integration** (planned): Control your CD changer using Flipper Zero's infrared capabilities

## Status

✅ **Active Development** - Core features implemented and working! App builds, runs, and persists data successfully on Flipper Zero.

### Current Progress (February 2025) – v1.1.0

- ✅ **Multi-Changer**: Changers (Name, Location, Slots); select at top menu; header shows Changer name; Add/Edit/Delete
- ✅ **Splash Screen**: Brief FlipChanger logo on launch
- ✅ **Main Menu**: 6 options (View Slots, Add CD, Settings, Statistics, Changers, Help); scrollable (5 visible)
- ✅ **Core UI**: Slot browser, slot details view; per-Changer slots database
- ✅ **Navigation**: Wrap-around with scroll, long-press skip by 10; Changer switch persists
- ✅ **Memory Optimization**: SD card-based caching; deferred load/save to avoid stack overflow
- ✅ **Add/Edit Interface**: Character input; track management (title, duration)
- ✅ **Settings/Statistics**: Slot count per Changer; statistics
- 📋 **Planned**: Enhanced fields (Disc Number, Artist split), IR integration

## Documentation

- [Product Vision Document](docs/product_vision.md) - Complete project vision and goals
- [Changers Design](docs/CHANGERS_DESIGN.md) - Multi-Changer architecture (next phase)
- [Development Setup Guide](docs/development_setup.md) - Flipper Zero development research
- [Quick Start Checklist](docs/quick_start_checklist.md) - Development checklist
- [Progress Report](docs/PROGRESS.md) - Current development status and achievements

## Application Status

### ✅ FlipChanger App (`flipchanger-app/`) - **MAIN APP**
- Core FlipChanger application
- Slot management (1-200 slots)
- CD metadata tracking
- **Quick start:**
  ```bash
  cd flipchanger-app
  ufbt build APPID=flipchanger    # Build the app
  ufbt launch APPID=flipchanger   # Deploy and run
  ```
- See [flipchanger-app/README.md](flipchanger-app/README.md) for details
- **Status**: ✅ **Working!** App successfully deployed and running on device. Memory optimized for Flipper Zero constraints.

### Test Apps

#### 1. C/C++ Hello World (`hello-world-ufbt/`)
- Simple test app to verify uFBT setup
- See [hello-world-ufbt/README.md](hello-world-ufbt/README.md)

#### 2. JavaScript/TypeScript Version (`hello-world-app/`)
- JavaScript SDK test app (requires Node.js)
- See [hello-world-app/README.md](hello-world-app/README.md)

## Installation

> **Coming Soon** - Once published to the Flipper Zero app store, installation instructions will be provided here.

## Development

### Requirements

- Flipper Zero device
- Flipper Zero development environment (see [official docs](https://docs.flipperzero.one/development/firmware))
- SD card for data storage

### Building

**Prerequisites:**
- uFBT installed: `pipx install ufbt` (✅ Done!)
- Flipper Zero device with USB cable
- SD card inserted (for data storage)

**Build and Deploy:**
```bash
cd flipchanger-app
ufbt build APPID=flipchanger    # Build the app
ufbt launch APPID=flipchanger   # Deploy and run on device
```

**File Location:**
- Built app: `/Users/gilbertroberts/.ufbt/build/flipchanger.fap`
- Installed to device: `/ext/apps/Tools/flipchanger.fap`

## Contributing

Contributions are welcome! This is a learning project, so whether you're a beginner or experienced developer, your input is valuable.

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

[MIT License](LICENSE)

## Acknowledgments

- Flipper Zero community
- All contributors and future contributors

---

**Note**: This is an educational project for learning Flipper Zero application development and open-source practices.
