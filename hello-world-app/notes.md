# Important Notes About This Hello World App

## API Accuracy

**Important**: The code in `src/index.ts` uses example API calls that may not match the exact Flipper Zero JS SDK API.

The actual Flipper Zero JavaScript API may differ. You should:

1. **Check Official Documentation**:
   - Visit: https://developer.flipper.net/flipperzero/doxygen/js_about_js_engine.html
   - Review the exact API structure

2. **Start Simple**:
   - Use `console.log()` for initial testing (shown in `index.js`)
   - Verify the app runs before adding GUI complexity

3. **Use the SDK Scaffold**:
   ```bash
   npx @flipperdevices/create-fz-app@latest hello-test
   ```
   This will generate a working example with the correct API calls.

## Recommended Approach

1. **First**: Install Node.js
2. **Second**: Use the official SDK scaffold to see working examples:
   ```bash
   npx @flipperdevices/create-fz-app@latest hello-test
   cd hello-test
   npm install
   npm start
   ```
3. **Third**: Study the generated code to understand the actual API
4. **Fourth**: Adapt this hello-world-app to use the correct API patterns

## Alternative: Manual Testing

If the SDK isn't working yet, you can test with a simple script:

1. Create a simple `.js` file with just `console.log("Hello World")`
2. Copy it to your Flipper Zero SD card in the `Apps/Scripts` directory
3. Run it from the Flipper Zero menu: Apps → Scripts → Your script

## What This App Demonstrates

- Project structure
- Package configuration
- TypeScript setup
- Build configuration
- Basic app template

You'll need to adjust the actual code based on the real SDK API once you have Node.js installed and can check the documentation.
