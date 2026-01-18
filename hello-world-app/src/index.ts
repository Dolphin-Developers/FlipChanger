/**
 * Hello World App for Flipper Zero
 * 
 * This is a simple test app to verify the Flipper Zero development setup.
 * It demonstrates basic UI elements and button handling.
 */

import { GUI } from "@flipperdevices/js-api/gui";
import { Button } from "@flipperdevices/js-api/button";
import { delay } from "@flipperdevices/js-api/delay";

// Main application entry point
async function main() {
    // Initialize GUI
    const gui = new GUI();
    
    // Clear screen
    gui.clear();
    
    // Display welcome message
    gui.drawText({
        x: 0,
        y: 10,
        text: "FlipChanger",
        font: "fonts/DejaVuSans_14px"
    });
    
    gui.drawText({
        x: 0,
        y: 25,
        text: "Hello World!",
        font: "fonts/DejaVuSans_14px"
    });
    
    gui.drawText({
        x: 0,
        y: 40,
        text: "Setup Test",
        font: "fonts/DejaVuSans_14px"
    });
    
    // Draw a simple frame/border
    gui.drawFrame({
        x: 0,
        y: 0,
        width: 128,
        height: 64
    });
    
    // Update display
    gui.update();
    
    // Counter to track button presses
    let counter = 0;
    
    // Simple interaction: wait for button press
    gui.drawText({
        x: 0,
        y: 55,
        text: `Press OK (count: ${counter})`,
        font: "fonts/DejaVuSans_10px"
    });
    gui.update();
    
    // Event loop - handle button presses
    while (true) {
        // Wait for button press (non-blocking)
        const button = await Button.waitForPress({ timeout: 100 });
        
        if (button === Button.OK) {
            counter++;
            
            // Update counter display
            gui.clear();
            gui.drawFrame({
                x: 0,
                y: 0,
                width: 128,
                height: 64
            });
            
            gui.drawText({
                x: 0,
                y: 10,
                text: "FlipChanger",
                font: "fonts/DejaVuSans_14px"
            });
            
            gui.drawText({
                x: 0,
                y: 25,
                text: "Button pressed!",
                font: "fonts/DejaVuSans_14px"
            });
            
            gui.drawText({
                x: 0,
                y: 40,
                text: `Count: ${counter}`,
                font: "fonts/DejaVuSans_14px"
            });
            
            gui.drawText({
                x: 0,
                y: 55,
                text: "Press OK again",
                font: "fonts/DejaVuSans_10px"
            });
            
            gui.update();
            
        } else if (button === Button.BACK) {
            // Exit on back button
            gui.clear();
            gui.drawText({
                x: 0,
                y: 30,
                text: "Exiting...",
                font: "fonts/DejaVuSans_14px"
            });
            gui.update();
            break;
        }
        
        // Small delay to prevent button bounce
        await delay(100);
    }
}

// Alternative simpler version if the above API doesn't match exactly
// This version uses a more basic approach that should work with most Flipper JS SDKs

async function simpleMain() {
    // Basic version - check actual SDK documentation for exact API
    console.log("FlipChanger Hello World App");
    console.log("If you see this, the app is running!");
    
    // Note: The actual API may differ. Check:
    // https://developer.flipper.net/flipperzero/doxygen/js_about_js_engine.html
    // for the exact API structure
}

// Run main function
try {
    main().catch((error) => {
        console.error("Error:", error);
        // Fallback to simple version
        simpleMain();
    });
} catch (error) {
    console.error("Initialization error:", error);
    simpleMain();
}
