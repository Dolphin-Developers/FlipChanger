# Installation Instructions for Hello World App

## Quick Start

### Step 1: Install Node.js

**macOS (using Homebrew):**
```bash
brew install node
```

**macOS (Manual):**
1. Visit [nodejs.org](https://nodejs.org/)
2. Download the macOS installer
3. Run the installer
4. Verify installation:
   ```bash
   node --version
   npm --version
   ```

**Linux:**
```bash
# Ubuntu/Debian
sudo apt update
sudo apt install nodejs npm

# Fedora
sudo dnf install nodejs npm
```

**Windows:**
1. Visit [nodejs.org](https://nodejs.org/)
2. Download the Windows installer (.msi)
3. Run the installer
4. Verify in Command Prompt:
   ```cmd
   node --version
   npm --version
   ```

### Step 2: Install Flipper Zero JS SDK

Once Node.js is installed:

```bash
cd hello-world-app
npm install
```

This will install:
- `@flipperdevices/flipperzero-js-sdk` - Official Flipper Zero SDK
- TypeScript (if using TypeScript version)
- Other development dependencies

### Step 3: Verify Setup

```bash
# Check Node.js
node --version  # Should show v16+ or higher

# Check npm
npm --version   # Should show version number

# Check SDK installation
npm list @flipperdevices/flipperzero-js-sdk
```

### Step 4: Connect Flipper Zero

1. **Physical Connection**:
   - Connect Flipper Zero to computer via USB cable
   - Power on the Flipper Zero

2. **Verify Connection** (macOS/Linux):
   ```bash
   ls /dev/tty.usbmodem*  # Should list your Flipper device
   ```

3. **Verify Connection** (Windows):
   - Check Device Manager for "Flipper Zero" or USB serial device

### Step 5: Deploy and Run

```bash
npm start
```

The first time may take longer as it downloads dependencies and builds the app.

## What Happens Next?

1. **SDK connects** to your Flipper Zero via USB
2. **Code is built** (TypeScript → JavaScript, or just JavaScript)
3. **App is deployed** to your device
4. **App runs** on the Flipper Zero

You should see:
- Build messages in the terminal
- Connection status
- The app appearing on your Flipper Zero screen

## Alternative: Manual SDK Installation

If `npm install` doesn't work, try installing the SDK globally:

```bash
npm install -g @flipperdevices/flipperzero-js-sdk
```

Then use the `fz-sdk` command directly:
```bash
fz-sdk start
```

## Troubleshooting Installation

### Node.js not found after installation
- **macOS/Linux**: Restart terminal or run `source ~/.bashrc` (or `~/.zshrc`)
- **Windows**: Restart Command Prompt/PowerShell
- Check PATH environment variable

### npm install fails
- Check internet connection
- Try: `npm install --verbose` for detailed error messages
- Clear npm cache: `npm cache clean --force`
- Try using yarn instead: `npm install -g yarn && yarn install`

### Permission errors (macOS/Linux)
- Use `sudo npm install` (not recommended long-term)
- Better: Fix npm permissions: `mkdir ~/.npm-global && npm config set prefix '~/.npm-global'`

### SDK not found after installation
- Check `node_modules` directory exists
- Verify installation: `npm list @flipperdevices/flipperzero-js-sdk`
- Try reinstalling: `npm uninstall @flipperdevices/flipperzero-js-sdk && npm install @flipperdevices/flipperzero-js-sdk`

## Next Steps After Installation

Once everything is installed:

1. ✅ Run `npm start` to test
2. ✅ Verify app appears on Flipper Zero
3. ✅ Test button interactions
4. ✅ Ready to develop FlipChanger!

---

**Need Help?** Check the [main README](README.md) for troubleshooting or visit the [Flipper Zero Developer Portal](https://developer.flipper.net/).
