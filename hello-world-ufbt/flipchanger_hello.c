/**
 * FlipChanger Hello World - uFBT Version
 * 
 * A simple test app to verify uFBT setup and Flipper Zero development environment.
 * This app displays "Hello World" and responds to button presses.
 */

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

// Application state
typedef struct {
    Gui* gui;
    ViewPort* view_port;
    NotificationApp* notifications;
    int counter;
    bool running;
} FlipChangerHelloApp;

// View port draw callback
static void draw_callback(Canvas* canvas, void* ctx) {
    FlipChangerHelloApp* app = (FlipChangerHelloApp*)ctx;
    
    // Clear screen
    canvas_clear(canvas);
    
    // Draw frame
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_frame(canvas, 0, 0, 128, 64);
    
    // Draw title
    canvas_draw_str(canvas, 10, 12, "FlipChanger");
    canvas_draw_str(canvas, 10, 24, "Hello World!");
    
    // Draw counter
    char counter_str[32];
    snprintf(counter_str, sizeof(counter_str), "Count: %d", app->counter);
    canvas_draw_str(canvas, 10, 36, counter_str);
    
    // Draw instructions
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 10, 50, "OK=Count BACK=Exit");
}

// Input callback
static void input_callback(InputEvent* input_event, void* ctx) {
    FlipChangerHelloApp* app = (FlipChangerHelloApp*)ctx;
    
    if(input_event->type == InputTypePress) {
        if(input_event->key == InputKeyOk) {
            // Increment counter on OK press
            app->counter++;
            notification_message(app->notifications, &sequence_blink_blue_100);
            view_port_update(app->view_port);
        } else if(input_event->key == InputKeyBack) {
            // Exit on Back press
            app->running = false;
        }
    }
}

// Main application entry point
int32_t flipchanger_hello_main(void* p) {
    UNUSED(p);
    
    // Allocate app structure
    FlipChangerHelloApp* app = malloc(sizeof(FlipChangerHelloApp));
    
    // Initialize app
    app->gui = furi_record_open(RECORD_GUI);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->counter = 0;
    app->running = true;
    
    // Create view port
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, draw_callback, app);
    view_port_input_callback_set(app->view_port, input_callback, app);
    
    // Attach view port to GUI
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
    
    // Send notification that app started
    notification_message(app->notifications, &sequence_blink_green_100);
    
    // Main event loop
    while(app->running) {
        view_port_update(app->view_port);
        furi_delay_ms(100);
    }
    
    // Cleanup
    gui_remove_view_port(app->gui, app->view_port);
    view_port_free(app->view_port);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    free(app);
    
    return 0;
}
