/**
 * Hello World App for Flipper Zero (JavaScript Version)
 * 
 * Simplified version that works with basic mJS engine
 * This version uses simpler APIs that are more likely to work
 */

// Basic Hello World - works with minimal setup
console.log("=== FlipChanger Hello World ===");
console.log("Setup test app running!");

// Simple display update function
function displayText(gui, text, y) {
    // This is a simplified version - actual API may vary
    // Check Flipper JS SDK documentation for exact methods
    try {
        gui.drawText(0, y, text);
    } catch (e) {
        // Fallback to console if GUI not available
        console.log(text);
    }
}

// Main function
function main() {
    console.log("Initializing FlipChanger Hello World...");
    
    // Display simple message
    console.log("Hello World!");
    console.log("Flipper Zero setup test");
    console.log("");
    console.log("If you see this output,");
    console.log("your development setup is working!");
    
    // Note: For actual GUI interaction, you'll need to:
    // 1. Install the Flipper JS SDK: npm install @flipperdevices/flipperzero-js-sdk
    // 2. Import the GUI module: import { GUI } from "@flipperdevices/js-api/gui"
    // 3. Use the exact API as documented in the SDK
    
    return "Hello World app completed";
}

// Run main
main();
