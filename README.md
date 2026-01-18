# FlipChanger

FlipChanger is an open-source application for the Flipper Zero that helps users track physical CDs loaded into a CD changer system (3-200 discs). Store metadata like artist, album, and track listings directly on your Flipper Zero, similar to MP3 ID3 tags.

## Features

- **Slot Management**: Track which CDs are in which slots (1-200)
- **Metadata Storage**: Store artist, album, year, genre, and track listings for each CD
- **View & Browse**: Navigate your collection easily on the Flipper Zero display
- **Add/Edit CDs**: Manage your collection directly from the device
- **Data Persistence**: All data saved to SD card, survives reboots
- **IR Remote Integration** (planned): Control your CD changer using Flipper Zero's infrared capabilities

## Status

🚧 **In Development** - This is a learning project. Contributions welcome!

## Documentation

- [Product Vision Document](docs/product_vision.md)
- [Development Setup Guide](docs/development_setup.md) - Complete research on Flipper Zero app development
- [Quick Start Checklist](docs/quick_start_checklist.md) - Get started checklist

## Hello World Test App

A sample "Hello World" app is included in `hello-world-app/` to test your development setup.

**To use it:**
1. Install Node.js (see [hello-world-app/INSTALL.md](hello-world-app/INSTALL.md))
2. Install dependencies: `cd hello-world-app && npm install`
3. Connect your Flipper Zero via USB
4. Run: `npm start`

See [hello-world-app/README.md](hello-world-app/README.md) for full instructions.

## Installation

> **Coming Soon** - Once published to the Flipper Zero app store, installation instructions will be provided here.

## Development

### Requirements

- Flipper Zero device
- Flipper Zero development environment (see [official docs](https://docs.flipperzero.one/development/firmware))
- SD card for data storage

### Building

> Build instructions coming soon.

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
