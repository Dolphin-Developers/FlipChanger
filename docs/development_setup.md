# Flipper Zero Development Setup Guide
## Research & Requirements for FlipChanger

This document compiles research on Flipper Zero app development requirements, tools, and best practices for the FlipChanger project.

---

## 1. Development Paths: Two Approaches

Flipper Zero supports **two main development approaches**:

### Option A: Native C/C++ Apps (FAPs)
- **Language**: C/C++
- **Build Output**: `.fap` (Flipper Application Package) files
- **Pros**: Full hardware access, maximum performance, deeper integration
- **Cons**: Steeper learning curve, requires firmware repo, compilation needed
- **Best For**: Complex apps, maximum control, performance-critical features

### Option B: JavaScript/TypeScript Apps
- **Language**: JavaScript/TypeScript (via mJS engine)
- **Build Output**: `.js` files (or transpiled scripts)
- **Pros**: Easier iteration, no full firmware build, faster development cycle
- **Cons**: More limited features, less performance, constrained by mJS engine
- **Best For**: Learning, rapid prototyping, simpler apps

**Recommendation for FlipChanger**: Start with **JavaScript/TypeScript** for learning, then consider C/C++ if you need advanced features like complex IR integration or performance optimization.

---

## 2. Required Tools & Prerequisites

### For JavaScript/TypeScript Apps (Recommended Starting Point)

| Tool | Purpose | Installation |
|------|---------|--------------|
| **Node.js** | JavaScript runtime and package manager | Download from [nodejs.org](https://nodejs.org/) |
| **npm/yarn/pnpm** | Package manager (comes with Node.js) | Included with Node.js |
| **Flipper JS SDK** | Official SDK for Flipper Zero JS apps | Installed via `npm install @flipperdevices/create-fz-app` |
| **VS Code** (optional) | Recommended IDE with good TypeScript support | [code.visualstudio.com](https://code.visualstudio.com/) |

### For Native C/C++ Apps (Future Option)

| Tool | Purpose | Installation |
|------|---------|--------------|
| **Git** | Version control and cloning firmware | Pre-installed on macOS |
| **Python 3** | Required for build tools | Pre-installed on macOS |
| **FBT (Flipper Build Tool)** | Main build system for firmware and FAPs | Installed via firmware repo |
| **uFBT** (Alternative) | Lightweight build tool for external apps | `python3 -m pip install --upgrade ufbt` |
| **ARM Toolchain** | Cross-compiler for ARM architecture | Installed automatically by FBT/uFBT |

---

## 3. Development Setup Steps

### Path A: JavaScript/TypeScript Setup (Start Here)

1. **Install Node.js**
   ```bash
   # Check if Node.js is installed
   node --version
   npm --version
   ```

2. **Create a new Flipper Zero JS app**
   ```bash
   npx @flipperdevices/create-fz-app@latest
   ```
   This scaffolds a project with:
   - TypeScript configuration
   - Basic app structure
   - SDK dependencies
   - Build/deploy scripts

3. **Development Workflow**
   ```bash
   npm start  # Builds, transpiles, and uploads to Flipper Zero via USB
   ```
   - Connect Flipper Zero via USB
   - Changes are live: edit code → `npm start` → test on device

4. **File Structure** (Typical JS App)
   ```
   your-app/
   ├── src/
   │   └── index.ts       # Main app code
   ├── package.json
   ├── tsconfig.json
   └── fz-sdk.config.json5  # SDK configuration
   ```

### Path B: Native C/C++ Setup (For Later)

1. **Clone Firmware Repository**
   ```bash
   git clone --recursive https://github.com/flipperdevices/flipperzero-firmware.git
   cd flipperzero-firmware
   ```

2. **Install Build Dependencies**
   ```bash
   # macOS
   brew install scons
   
   # Or use uFBT (simpler)
   python3 -m pip install --upgrade ufbt
   ```

3. **Create Your App**
   ```bash
   # Create app directory
   mkdir -p applications_user/flipchanger
   cd applications_user/flipchanger
   
   # Create application.fam manifest file
   # Write your C source files
   ```

4. **Build Your App**
   ```bash
   # Using FBT (from firmware root)
   ./fbt fap_flipchanger
   
   # Or using uFBT (from app directory)
   ufbt build
   ```

5. **Deploy to Device**
   ```bash
   # Copy .fap file to SD card, or
   ./fbt launch APPSRC=applications_user/flipchanger
   ```

---

## 4. Application Structure & Manifests

### JavaScript Apps
- **No manifest required** - SDK handles packaging
- Configuration in `fz-sdk.config.json5`:
  ```json5
  {
    "app": {
      "name": "FlipChanger",
      "type": "js"
    },
    "build": {
      "minify": false  // Enable for production
    }
  }
  ```

### Native C/C++ Apps - Application Manifest (application.fam)

**Required Fields:**
```python
App(
    appid="flipchanger",
    apptype=FlipperAppType.EXTERNAL,
    name="FlipChanger",
    entry_point="flipchanger_main",
    fap_icon_assets="icons",
    fap_category="Games",  # or "Tools", "Utils", etc.
    requires=[
        FlipperAppRequirement.GUI,
        FlipperAppRequirement.STORAGE,
        FlipperAppRequirement.INFRARED,  # For IR features
    ],
    stack_size=2048,
    cdefines=["APP_FLIPCHANGER"],
)
```

**Key Manifest Fields:**
- `appid`: Unique identifier (lowercase, no spaces)
- `apptype`: `EXTERNAL` for user apps, `PLUGIN` for system integration
- `entry_point`: Main function name
- `requires`: List of hardware/API dependencies
- `stack_size`: Memory allocation (default: 2048 bytes)

---

## 5. Flipper Zero API Access

### Available Hardware/APIs (Both Paths)

| Feature | JavaScript | C/C++ | Notes |
|---------|------------|-------|-------|
| **GUI/Screen** | ✅ Yes | ✅ Yes | 128x64 monochrome display |
| **Buttons/Input** | ✅ Yes | ✅ Yes | D-pad, OK, Back buttons |
| **Storage** | ✅ Yes | ✅ Yes | SD card, internal storage |
| **Infrared (IR)** | ✅ Yes | ✅ Yes | IR database access available |
| **GPIO** | ✅ Yes | ✅ Yes | General-purpose I/O |
| **USB-HID** | ✅ Yes | ✅ Yes | USB Human Interface Device |
| **NFC** | ⚠️ Limited | ✅ Yes | Full NFC support in C/C++ |
| **Sub-GHz Radio** | ⚠️ Limited | ✅ Yes | Radio protocols in C/C++ |
| **Low-level Hardware** | ❌ No | ✅ Yes | Direct hardware access only in C/C++ |

### Infrared (IR) Integration

**Key Points:**
- Flipper Zero has an **existing IR database** of remote control codes
- IR codes can be accessed via API calls
- You can reuse existing codes from the database
- Custom IR codes can be recorded/learned

**IR API Usage (Conceptual):**
```javascript
// JavaScript (conceptual - exact API depends on SDK)
const ir = require("@flipperdevices/js-api/infrared");
// Search database for CD changer remotes
// Send IR commands
```

```c
// C/C++ (conceptual)
#include <infrared.h>
// Use infrared_transmit() function
// Access IR database via infrared_remote_get()
```

**For FlipChanger**: We can search the IR database for CD changer remotes and reuse those codes.

---

## 6. App Store Submission Requirements

### Flipper Apps Catalog Requirements

1. **Open Source**: All apps must be open source and free
2. **Repository**: Must be hosted on GitHub
3. **Metadata**: Proper app metadata in manifest
4. **Documentation**: README with build/usage instructions
5. **Compliance**: Follow Flipper's compliance guidelines
6. **Version Compatibility**: Apps must work with current firmware versions

### Submission Process

1. **Create GitHub Repository** (✅ Done for FlipChanger)
2. **Develop Your App** (Next step)
3. **Test Thoroughly** on actual hardware
4. **Submit Pull Request** to Flipper Apps Catalog repository
5. **Review Process**: Manual + automated checks
6. **Publication**: Once approved, available in Flipper Mobile/Lab

### Compliance Checklist

- [ ] App follows Flipper coding standards
- [ ] No unauthorized protocols/frequencies
- [ ] Proper use of trademarks/logos (if any)
- [ ] App manifest correctly formatted
- [ ] Documentation complete
- [ ] Open source license included (MIT ✅)

---

## 7. Development Constraints & Best Practices

### Hardware Limitations

| Resource | Limit | Impact |
|----------|-------|--------|
| **RAM** | ~64KB total | Efficient memory usage critical |
| **Flash** | ~1MB | Keep app size small |
| **Screen** | 128x64 monochrome | Simple UI, no color/graphics |
| **Storage** | SD card (varies) | Data stored on SD card |

### Best Practices

1. **Keep it Simple**: Flipper Zero has limited resources
2. **Efficient Storage**: Use JSON efficiently, avoid large files
3. **Simple UI**: Design for 128x64 monochrome display
4. **Test Often**: Regular testing on actual hardware
5. **Version Match**: Build for firmware version on your device
6. **Error Handling**: Robust error handling for storage/API failures

### Memory Management (C/C++ Apps)

- **Stack Size**: Set appropriately in manifest (default: 2048)
- **Heap**: Limited when running as FAP from SD card
- **Static Data**: Keep icons/images small
- **Dynamic Allocation**: Use sparingly

---

## 8. Learning Path Recommendation

### Phase 1: Setup & Hello World (Week 1)
1. ✅ Repository created on GitHub
2. Install Node.js and SDK
3. Create simple "Hello World" JS app
4. Test deployment to Flipper Zero
5. Understand basic UI elements

### Phase 2: Core Features (Weeks 2-3)
1. Implement JSON storage (CD metadata)
2. Build slot management UI
3. Create add/edit CD interface
4. Implement data persistence

### Phase 3: Polish & Testing (Week 4)
1. UI/UX improvements
2. Error handling
3. Testing on hardware
4. Documentation updates

### Phase 4: IR Integration (Optional, Future)
1. Research IR database API
2. Find CD changer remote codes
3. Implement IR control features
4. Testing with actual CD changer

### Phase 5: App Store Submission
1. Final testing
2. Prepare submission
3. Submit to Flipper Apps Catalog
4. Address review feedback

---

## 9. Resources & Documentation

### Official Documentation
- **Developer Portal**: https://developer.flipper.net/
- **Doxygen API Docs**: https://developer.flipper.net/flipperzero/doxygen/
- **Firmware Repository**: https://github.com/flipperdevices/flipperzero-firmware
- **JS SDK**: https://developer.flipper.net/flipperzero/doxygen/js_about_js_engine.html
- **App Publishing**: https://developer.flipper.net/flipperzero/doxygen/app_publishing.html

### Community Resources
- **Flipper Wiki**: https://flipper.wiki/
- **Reddit**: r/flipperzero
- **Discord**: Flipper Zero community servers
- **Example Apps**: Browse Flipper Apps Catalog for reference

### Build Tools
- **uFBT**: https://github.com/flipperdevices/flipperzero-ufbt (Lightweight build tool)
- **FBT**: Included in firmware repo (Full build system)

---

## 10. Next Steps for FlipChanger

### Immediate Actions

1. **Choose Development Path**
   - Recommendation: Start with JavaScript/TypeScript
   - Easier learning curve
   - Faster iteration
   - Can switch to C/C++ later if needed

2. **Install Prerequisites**
   ```bash
   # Check Node.js
   node --version
   npm --version
   
   # If not installed, download from nodejs.org
   ```

3. **Create Initial App Structure**
   - Use JS SDK to scaffold project
   - Set up basic project structure
   - Create initial README with setup instructions

4. **Research JSON Storage API**
   - How to read/write JSON files on SD card
   - Storage location and permissions
   - Error handling for missing SD card

5. **Design UI Flow**
   - Main menu
   - Slot browser
   - CD details view
   - Add/edit form

### Questions to Answer Before Coding

- [ ] Which development path: JavaScript or C/C++?
- [ ] How will data be structured? (Review data model in product_vision.md)
- [ ] What's the navigation flow?
- [ ] How many slots will be visible at once on 128x64 screen?
- [ ] Where will JSON file be stored? (SD card path)

---

## 11. Recommendations Summary

### For FlipChanger Project

1. **Start with JavaScript/TypeScript**
   - Lower barrier to entry
   - Faster development cycle
   - Good for learning project

2. **Use JSON for Storage**
   - Human-readable
   - Easy to edit/debug
   - Well-supported in JS

3. **Keep UI Simple**
   - 128x64 display is small
   - List-based navigation
   - Form-based data entry

4. **Plan for App Store**
   - Follow best practices from start
   - Document everything
   - Keep code clean and commented

5. **IR Integration Later**
   - Get core features working first
   - Add IR as enhancement
   - Research IR database after MVP

---

**Document Status**: Research Complete  
**Last Updated**: 2024  
**Next Update**: After initial development setup
