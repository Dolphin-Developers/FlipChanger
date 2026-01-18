# FlipChanger Hello World - Test App

A simple "Hello World" app to test your Flipper Zero development setup.

## Purpose

This app helps verify that:
- Your development environment is set up correctly
- You can build and deploy apps to Flipper Zero
- Basic UI and button handling works
- You're ready to start developing FlipChanger

## Prerequisites

1. **Node.js** installed (v16 or higher)
   ```bash
   node --version
   npm --version
   ```

2. **Flipper Zero Device**
   - Connected via USB
   - Latest firmware installed
   - SD card inserted

3. **Flipper Zero JS SDK** (installed via npm)

## Setup

1. **Install Node.js** (if not already installed)
   - Download from [nodejs.org](https://nodejs.org/)
   - Or use Homebrew on macOS: `brew install node`

2. **Install Dependencies**
   ```bash
   cd hello-world-app
   npm install
   ```

3. **Connect Flipper Zero**
   - Connect via USB cable
   - Ensure device is powered on
   - Check connection:
     ```bash
     # macOS/Linux
     ls /dev/tty.usbmodem*  # Should show Flipper device
     ```

## Running the App

### Basic Run
```bash
npm start
```

This will:
- Build the TypeScript code
- Connect to your Flipper Zero
- Deploy the app
- Run it on the device

### Build Only (without deploying)
```bash
npm run build
```

### Watch Mode (auto-reload on changes)
```bash
npm run watch
```

## What to Expect

When the app runs successfully:

1. **Console Output**: You should see build messages and connection status
2. **On Device**: The app should display:
   - "FlipChanger" title
   - "Hello World!" message
   - Button interaction counter

3. **Interaction**:
   - Press **OK** button: Increment counter
   - Press **BACK** button: Exit app

## Troubleshooting

### "node: command not found"
- Install Node.js from [nodejs.org](https://nodejs.org/)

### "Cannot connect to Flipper Zero"
- Check USB connection
- Ensure device is powered on
- Try different USB cable/port
- Check device shows up: `ls /dev/tty.usbmodem*`

### "Module not found" errors
- Run `npm install` in the hello-world-app directory
- Check that `node_modules` directory exists

### App doesn't appear on device
- Ensure Flipper Zero firmware is up to date
- Check that the app was deployed successfully
- Look in Apps → Scripts (for JS apps) or Apps → External (for compiled apps)

### Build errors
- Check TypeScript version: `npm install typescript --save-dev`
- Verify SDK installation: `npm list @flipperdevices/flipperzero-js-sdk`
- Review error messages for missing dependencies

## Files Explained

- `src/index.ts` - TypeScript version (recommended)
- `src/index.js` - JavaScript fallback version
- `package.json` - Dependencies and scripts
- `tsconfig.json` - TypeScript configuration
- `fz-sdk.config.json5` - Flipper SDK configuration

## Next Steps

Once this app works:

1. ✅ Development environment verified
2. ✅ Basic deployment tested
3. ✅ Ready to build FlipChanger features

Proceed to building the actual FlipChanger app!

## Resources

- [Flipper Zero JS SDK Docs](https://developer.flipper.net/flipperzero/doxygen/js_about_js_engine.html)
- [Development Setup Guide](../docs/development_setup.md)
- [Flipper Zero Developer Portal](https://developer.flipper.net/)

---

**Note**: If the TypeScript version doesn't work due to API differences, try the simpler JavaScript version (`index.js`) and check the official SDK documentation for the exact API structure.
