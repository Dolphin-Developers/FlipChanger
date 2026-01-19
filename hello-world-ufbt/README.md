# FlipChanger Hello World - uFBT Version

A simple "Hello World" C app for Flipper Zero using uFBT (Micro Flipper Build Tool).

## What This Demonstrates

- Basic Flipper Zero app structure
- Using uFBT to build and deploy apps
- GUI rendering on 128x64 display
- Button input handling
- View port and canvas usage

## Prerequisites

1. **uFBT installed** ✅ (Done!)
   ```bash
   ufbt --version  # Verify installation
   ```

2. **Flipper Zero Device**
   - Connected via USB
   - Latest firmware installed
   - SD card inserted (optional, for data)

## Building the App

### Build the .fap file
```bash
cd hello-world-ufbt
ufbt build
```

This will:
- Compile the C source code
- Create a `.fap` file in the `dist/` directory
- Download any required SDK/toolchain components on first run

### Build and Launch (Recommended)
```bash
ufbt launch
```

This will:
- Build the app
- Connect to your Flipper Zero via USB
- Deploy the app
- Run it on the device

## What to Expect

When the app runs on your Flipper Zero:

1. **Screen Display**:
   - "FlipChanger" title
   - "Hello World!" message
   - Counter (starts at 0)
   - Instructions at bottom

2. **Interactions**:
   - Press **OK** button: Increments counter, blue LED blinks
   - Press **BACK** button: Exits app

3. **LED Feedback**:
   - Green blink when app starts
   - Blue blink on each OK press

## File Structure

```
hello-world-ufbt/
├── application.fam       # App manifest (required)
├── flipchanger_hello.c   # Main C source file
└── README.md             # This file
```

After building, you'll also have:
```
dist/
└── flipchanger_hello.fap  # Compiled app package
```

## Troubleshooting

### "ufbt: command not found"
- Ensure uFBT is installed: `pipx install ufbt`
- Check PATH: `echo $PATH` (should include pipx bin directory)
- Restart terminal if needed

### "Cannot connect to Flipper Zero"
- Check USB connection
- Ensure device is powered on
- Close other Flipper apps (QFlipper, etc.) that might lock the port
- Verify device: `ls /dev/tty.usbmodem*` (macOS/Linux)

### Build errors
- Ensure you're in the app directory (`hello-world-ufbt/`)
- Check that `application.fam` exists
- Verify uFBT is up to date: `pipx upgrade ufbt`

### App doesn't appear on device
- Ensure Flipper Zero firmware is up to date
- Check build was successful: look for `dist/flipchanger_hello.fap`
- Try manually copying `.fap` file to SD card: `/ext/apps/Tools/flipchanger_hello.fap`

## Manual Installation

If `ufbt launch` doesn't work, you can manually install:

1. **Build the app**:
   ```bash
   ufbt build
   ```

2. **Copy .fap file to SD card**:
   - Eject SD card from Flipper Zero
   - Insert into computer
   - Copy `dist/flipchanger_hello.fap` to:
     - `/ext/apps/Tools/flipchanger_hello.fap`

3. **Run on device**:
   - Reinsert SD card
   - Navigate: Apps → Tools → FlipChanger Hello

## Next Steps

Once this app works:

1. ✅ uFBT setup verified
2. ✅ Basic C app structure understood
3. ✅ Ready to build FlipChanger features

Proceed to building the actual FlipChanger app using this structure!

## Resources

- [uFBT GitHub](https://github.com/flipperdevices/flipperzero-ufbt)
- [Flipper Zero Development Docs](https://developer.flipper.net/)
- [Development Setup Guide](../docs/development_setup.md)

---

**Note**: This is a C/C++ app (not JavaScript). For JavaScript apps, see `../hello-world-app/` directory.
