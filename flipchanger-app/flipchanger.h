/**
 * FlipChanger - Header File
 * 
 * Main application header with type definitions and function declarations
 */

#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>
#include <notification/notification.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Maximum number of slots (CDs) - stored on SD card
#define MAX_SLOTS 200
#define MIN_SLOTS 3
#define DEFAULT_SLOTS 100  // Default number of slots

// Memory cache - only keep visible slots in RAM
#define SLOT_CACHE_SIZE 10  // Only keep 10 slots in memory at a time

// Maximum string lengths
#define MAX_STRING_LENGTH 64
#define MAX_ARTIST_LENGTH 64
#define MAX_ALBUM_LENGTH 64
#define MAX_GENRE_LENGTH 32
#define MAX_TRACK_TITLE_LENGTH 64
#define MAX_NOTES_LENGTH 256
#define MAX_TRACKS 20  // Reduced for memory - can increase later

// File paths for data storage
#define FLIPCHANGER_APP_DIR "/ext/apps/Tools"
#define FLIPCHANGER_DATA_PATH FLIPCHANGER_APP_DIR "/flipchanger_data.json"
#define FLIPCHANGER_CHANGERS_PATH FLIPCHANGER_APP_DIR "/flipchanger_changers.json"

// Multi-Changer support
#define MAX_CHANGERS 10
#define CHANGER_ID_LEN 24
#define CHANGER_NAME_LEN 33
#define CHANGER_LOCATION_LEN 33

// Changer metadata (Name, Location, Total Slots)
typedef struct {
    char id[CHANGER_ID_LEN];
    char name[CHANGER_NAME_LEN];
    char location[CHANGER_LOCATION_LEN];
    int32_t total_slots;
} Changer;

// Track information
typedef struct {
    int32_t number;
    char title[MAX_TRACK_TITLE_LENGTH];
    char duration[16];  // Format: "3:45"
} Track;

// CD information
typedef struct {
    char artist[MAX_ARTIST_LENGTH];
    char album[MAX_ALBUM_LENGTH];
    int32_t year;
    char genre[MAX_GENRE_LENGTH];
    Track tracks[MAX_TRACKS];
    int32_t track_count;
    char notes[MAX_NOTES_LENGTH];
} CD;

// Slot information
typedef struct {
    int32_t slot_number;
    bool occupied;
    CD cd;
} Slot;

// Application state
typedef struct {
    Gui* gui;
    ViewPort* view_port;
    NotificationApp* notifications;
    Storage* storage;
    
    // Changers registry
    Changer changers[MAX_CHANGERS];
    int32_t changer_count;
    char current_changer_id[CHANGER_ID_LEN];   // ID of selected Changer
    int32_t current_changer_index;             // Index in changers[] or -1 if none

    // Data - only cache a few slots in memory, rest on SD card
    Slot slots[SLOT_CACHE_SIZE];  // Cache for visible slots
    int32_t total_slots;
    int32_t current_slot_index;  // Currently viewing/editing
    int32_t cache_start_index;   // First cached slot index
    
    // UI State
    enum {
        VIEW_MAIN_MENU,
        VIEW_SLOT_LIST,
        VIEW_SLOT_DETAILS,
        VIEW_ADD_EDIT_CD,
        VIEW_TRACK_MANAGEMENT,
        VIEW_SETTINGS,
        VIEW_STATISTICS,
        VIEW_CHANGERS,
        VIEW_ADD_EDIT_CHANGER,
        VIEW_CONFIRM_DELETE_CHANGER,
        VIEW_SPLASH,
        VIEW_HELP,
        VIEW_CONFIRM_DELETE,
    } current_view;
    
    int32_t details_scroll_offset;  // Scroll offset for slot details view
    int32_t help_return_view;       // View to return to from Help (current_view enum)
    
    int32_t selected_index;      // Selected item in list
    int32_t scroll_offset;        // Scroll position in lists
    bool running;
    bool dirty;                   // Data has been modified, needs save
    
    // Settings state
    bool editing_slot_count;      // True if editing slot count in settings
    int32_t edit_slot_count_pos;  // Cursor position for slot count editing

    // Changer add/edit state
    Changer edit_changer;         // Buffer for add/edit form
    int32_t edit_changer_index;   // -1=add new, >=0=edit existing
    int32_t edit_changer_field;   // 0=name, 1=location, 2=slots
    uint32_t splash_start_tick;   // For splash screen timer
    bool pending_changer_switch;  // Defer load/save to main loop (avoids stack overflow in input callback)
    
    // Add/Edit Input State
    enum {
        FIELD_ARTIST,
        FIELD_ALBUM,
        FIELD_YEAR,
        FIELD_GENRE,
        FIELD_NOTES,
        FIELD_TRACKS,
        FIELD_SAVE,
        FIELD_COUNT
    } edit_field;                 // Current field being edited
    int32_t edit_char_pos;        // Character position in current field
    int32_t edit_char_selection;  // Selected character (for character picker)
    int32_t edit_field_scroll;    // Scroll offset for long field text display
    
    // Track Management State
    int32_t edit_selected_track;  // Selected track index for editing
    bool editing_track;            // True if editing a track (title/duration)
    enum {
        TRACK_FIELD_TITLE,
        TRACK_FIELD_DURATION,
        TRACK_FIELD_COUNT
    } edit_track_field;            // Which track field is being edited
    
} FlipChangerApp;

// Function declarations
int32_t flipchanger_main(void* p);

// Storage functions
bool flipchanger_load_changers(FlipChangerApp* app);
bool flipchanger_save_changers(FlipChangerApp* app);
bool flipchanger_load_data(FlipChangerApp* app);
bool flipchanger_save_data(FlipChangerApp* app);
void flipchanger_get_slots_path(const FlipChangerApp* app, char* path_out, size_t path_size);

// UI functions
void flipchanger_draw_callback(Canvas* canvas, void* ctx);
void flipchanger_input_callback(InputEvent* input_event, void* ctx);
void flipchanger_draw_main_menu(Canvas* canvas, FlipChangerApp* app);
void flipchanger_draw_slot_list(Canvas* canvas, FlipChangerApp* app);
void flipchanger_draw_slot_details(Canvas* canvas, FlipChangerApp* app);
void flipchanger_draw_add_edit(Canvas* canvas, FlipChangerApp* app);

// Navigation functions
void flipchanger_show_main_menu(FlipChangerApp* app);
void flipchanger_show_slot_list(FlipChangerApp* app);
void flipchanger_show_slot_details(FlipChangerApp* app, int32_t slot_index);
void flipchanger_show_add_edit(FlipChangerApp* app, int32_t slot_index, bool is_new);

// Utility functions
void flipchanger_init_slots(FlipChangerApp* app, int32_t total_slots);
const char* flipchanger_get_slot_status(FlipChangerApp* app, int32_t slot_index);
int32_t flipchanger_count_occupied_slots(FlipChangerApp* app);
