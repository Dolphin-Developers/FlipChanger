/**
 * FlipChanger - Main Application File
 * 
 * Tracks CD metadata for CD changers (3-200 discs)
 * Similar to MP3 ID3 tags for physical CDs
 */

#include "flipchanger.h"
#include <notification/notification_messages.h>
#include <m-array.h>
#include <stream/stream.h>
#include <stream/buffered_file_stream.h>
#include <storage/storage.h>
#include <furi.h>
#include <string.h>

// Initialize slots (only cache in memory, full data on SD card)
void flipchanger_init_slots(FlipChangerApp* app, int32_t total_slots) {
    app->total_slots = (total_slots < MIN_SLOTS) ? MIN_SLOTS : 
                       (total_slots > MAX_SLOTS) ? MAX_SLOTS : total_slots;
    
    // Only initialize cache slots (memory efficient)
    for(int32_t i = 0; i < SLOT_CACHE_SIZE; i++) {
        app->slots[i].slot_number = i + 1;
        app->slots[i].occupied = false;
        memset(&app->slots[i].cd, 0, sizeof(CD));
        app->slots[i].cd.track_count = 0;
    }
    
    app->cache_start_index = 0;
    app->current_slot_index = 0;
    app->selected_index = 0;
    app->scroll_offset = 0;
}

// Load slot from SD card into cache
bool flipchanger_load_slot_from_sd(FlipChangerApp* app, int32_t slot_index) {
    // For now, just reload all data (inefficient but works)
    // TODO: Optimize to load individual slot
    if(slot_index < 0 || slot_index >= app->total_slots) {
        return false;
    }
    
    // Reload entire file and find slot
    // This is inefficient but safe - can optimize later
    return flipchanger_load_data(app);
}

// Save slot to SD card
bool flipchanger_save_slot_to_sd(FlipChangerApp* app, int32_t slot_index) {
    // For now, save all cached slots (efficient enough for our use case)
    // TODO: Optimize to update single slot in file
    if(slot_index < 0 || slot_index >= app->total_slots) {
        return false;
    }
    
    // Save all cached slots to file
    return flipchanger_save_data(app);
}

// Get slot from cache or SD card
Slot* flipchanger_get_slot(FlipChangerApp* app, int32_t slot_index) {
    if(slot_index < 0 || slot_index >= app->total_slots) {
        return NULL;
    }
    
    // Check if slot is in cache
    int32_t cache_index = slot_index - app->cache_start_index;
    if(cache_index >= 0 && cache_index < SLOT_CACHE_SIZE) {
        return &app->slots[cache_index];
    }
    
    // Slot not in cache - try to load from SD card
    // For now, return NULL (will implement SD loading)
    // TODO: Load slot from SD card and update cache
    return NULL;
}

// Update cache to include requested slot (only call from input handler, not draw!)
void flipchanger_update_cache(FlipChangerApp* app, int32_t slot_index) {
    // Calculate new cache start
    int32_t new_cache_start = slot_index - (SLOT_CACHE_SIZE / 2);
    if(new_cache_start < 0) {
        new_cache_start = 0;
    }
    if(new_cache_start + SLOT_CACHE_SIZE > app->total_slots) {
        new_cache_start = app->total_slots - SLOT_CACHE_SIZE;
        if(new_cache_start < 0) {
            new_cache_start = 0;
        }
    }
    
    // Only reload if cache needs to shift
    if(new_cache_start != app->cache_start_index) {
        // Save current cache if dirty (before reloading)
        if(app->dirty && app->storage) {
            flipchanger_save_data(app);
        }
        
        // Reload data from SD card (this loads cached slots)
        // TODO: Optimize to load only the new cache range
        if(app->storage) {
            flipchanger_load_data(app);
        }
        app->cache_start_index = new_cache_start;
        
        // Update slot numbers in cache
        for(int32_t i = 0; i < SLOT_CACHE_SIZE && i < app->total_slots; i++) {
            app->slots[i].slot_number = app->cache_start_index + i + 1;
        }
    }
}

// Get slot status string (from cache or SD)
const char* flipchanger_get_slot_status(FlipChangerApp* app, int32_t slot_index) {
    Slot* slot = flipchanger_get_slot(app, slot_index);
    if(!slot) {
        // Not in cache, try to load
        flipchanger_load_slot_from_sd(app, slot_index);
        slot = flipchanger_get_slot(app, slot_index);
        if(!slot) {
            return "Empty";  // Default to empty if can't load
        }
    }
    
    if(slot->occupied) {
        return slot->cd.album;
    }
    
    return "Empty";
}

// Count occupied slots (counts cached slots only - full count from SD card later)
int32_t flipchanger_count_occupied_slots(FlipChangerApp* app) {
    int32_t count = 0;
    // Only count cached slots for now
    // TODO: Count all slots from SD card
    for(int32_t i = 0; i < SLOT_CACHE_SIZE && i < app->total_slots; i++) {
        if(app->slots[i].occupied) {
            count++;
        }
    }
    return count;
}

// Helper: Skip whitespace in JSON
static const char* skip_whitespace(const char* str) {
    while(*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }
    return str;
}

// Helper: Read string value from JSON
static const char* read_json_string(const char* json, char* buffer, size_t buffer_size) {
    const char* p = skip_whitespace(json);
    if(*p != '"') return NULL;
    p++;  // Skip opening quote
    
    size_t i = 0;
    while(*p && *p != '"' && i < buffer_size - 1) {
        if(*p == '\\' && *(p + 1) == '"') {
            buffer[i++] = '"';
            p += 2;
        } else {
            buffer[i++] = *p++;
        }
    }
    buffer[i] = '\0';
    
    if(*p == '"') p++;  // Skip closing quote
    return p;
}

// Helper: Read integer value from JSON
static const char* read_json_int(const char* json, int32_t* value) {
    const char* p = skip_whitespace(json);
    *value = 0;
    bool negative = false;
    
    if(*p == '-') {
        negative = true;
        p++;
    }
    
    while(*p >= '0' && *p <= '9') {
        *value = *value * 10 + (*p - '0');
        p++;
    }
    
    if(negative) *value = -(*value);
    return p;
}

// Helper: Read boolean value from JSON
static const char* read_json_bool(const char* json, bool* value) {
    const char* p = skip_whitespace(json);
    if(strncmp(p, "true", 4) == 0) {
        *value = true;
        return p + 4;
    } else if(strncmp(p, "false", 5) == 0) {
        *value = false;
        return p + 5;
    }
    return NULL;
}

// Helper: Find JSON key
static const char* find_json_key(const char* json, const char* key) {
    char key_pattern[64];
    snprintf(key_pattern, sizeof(key_pattern), "\"%s\"", key);
    const char* found = strstr(json, key_pattern);
    if(!found) return NULL;
    
    const char* p = found + strlen(key_pattern);
    p = skip_whitespace(p);
    if(*p == ':') {
        return p + 1;
    }
    return NULL;
}

// Load data from JSON file
bool flipchanger_load_data(FlipChangerApp* app) {
    if(!app || !app->storage) {
        return false;
    }
    
    // Initialize with default slots
    flipchanger_init_slots(app, DEFAULT_SLOTS);
    
    // Try to open and read file if it exists
    File* file = storage_file_alloc(app->storage);
    
    if(!storage_file_open(file, FLIPCHANGER_DATA_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        // File doesn't exist - use defaults
        storage_file_free(file);
        return true;
    }
    
    // Read file into buffer (limit size for memory)
    uint8_t buffer[4096];  // 4KB buffer - should be enough for reasonable data
    uint16_t bytes_read = storage_file_read(file, buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';
    
    storage_file_close(file);
    storage_file_free(file);
    
    // Parse JSON
    const char* json = (const char*)buffer;
    const char* p = json;
    
    // Find version (optional)
    p = find_json_key(json, "version");
    if(p) {
        int32_t version = 0;
        p = read_json_int(p, &version);
        // Version handling (for future compatibility)
    }
    
    // Find total_slots
    p = find_json_key(json, "total_slots");
    if(p) {
        int32_t total_slots = DEFAULT_SLOTS;
        p = read_json_int(p, &total_slots);
        if(total_slots >= MIN_SLOTS && total_slots <= MAX_SLOTS) {
            app->total_slots = total_slots;
        }
    }
    
    // Find slots array
    p = find_json_key(json, "slots");
    if(!p) {
        // No slots array - use defaults
        return true;
    }
    
    p = skip_whitespace(p);
    if(*p != '[') {
        return true;  // Invalid format
    }
    p++;  // Skip '['
    
    // Parse slots (limited to what we can cache)
    int32_t slot_index = 0;
    while(*p && slot_index < SLOT_CACHE_SIZE && slot_index < app->total_slots) {
        p = skip_whitespace(p);
        if(*p == ']') break;  // End of array
        if(*p != '{') {
            p++;
            continue;  // Skip invalid entry
        }
        p++;  // Skip '{'
        
        Slot* slot = &app->slots[slot_index];
        slot->slot_number = slot_index + 1;
        slot->occupied = false;
        memset(&slot->cd, 0, sizeof(CD));
        slot->cd.track_count = 0;
        
        // Parse slot number
        const char* slot_key = find_json_key(p, "slot");
        if(slot_key) {
            int32_t slot_num = 0;
            read_json_int(slot_key, &slot_num);
            slot->slot_number = slot_num;
        }
        
        // Parse occupied
        const char* occ_key = find_json_key(p, "occupied");
        if(occ_key) {
            read_json_bool(occ_key, &slot->occupied);
        }
        
        if(slot->occupied) {
            // Parse artist
            const char* artist_key = find_json_key(p, "artist");
            if(artist_key) {
                read_json_string(artist_key, slot->cd.artist, MAX_ARTIST_LENGTH);
            }
            
            // Parse album
            const char* album_key = find_json_key(p, "album");
            if(album_key) {
                read_json_string(album_key, slot->cd.album, MAX_ALBUM_LENGTH);
            }
            
            // Parse year
            const char* year_key = find_json_key(p, "year");
            if(year_key) {
                read_json_int(year_key, &slot->cd.year);
            }
            
            // Parse genre
            const char* genre_key = find_json_key(p, "genre");
            if(genre_key) {
                read_json_string(genre_key, slot->cd.genre, MAX_GENRE_LENGTH);
            }
            
            // Parse tracks array (simplified - just count for now)
            const char* tracks_key = find_json_key(p, "tracks");
            if(tracks_key) {
                const char* tracks_start = skip_whitespace(tracks_key);
                if(*tracks_start == '[') {
                    tracks_start++;
                    int32_t track_count = 0;
                    const char* track_p = tracks_start;
                    
                    // Count tracks
                    while(*track_p && track_count < MAX_TRACKS) {
                        track_p = skip_whitespace(track_p);
                        if(*track_p == ']') break;
                        if(*track_p == '{') {
                            // Parse track (simplified)
                            track_count++;
                            while(*track_p && *track_p != '}') track_p++;
                            if(*track_p == '}') track_p++;
                        } else {
                            track_p++;
                        }
                        if(*track_p == ',') track_p++;
                    }
                    slot->cd.track_count = track_count;
                }
            }
            
            // Parse notes
            const char* notes_key = find_json_key(p, "notes");
            if(notes_key) {
                read_json_string(notes_key, slot->cd.notes, MAX_NOTES_LENGTH);
            }
        }
        
        // Find next slot or end of array
        while(*p && *p != '}' && *p != ']') p++;
        if(*p == '}') p++;  // Skip '}'
        if(*p == ',') p++;  // Skip ','
        
        slot_index++;
    }
    
    return true;
}

// Helper: Write JSON string (escape quotes)
static void write_json_string(File* file, const char* str) {
    if(!str || strlen(str) == 0) {
        storage_file_write(file, (const uint8_t*)"\"\"", 2);
        return;
    }
    
    storage_file_write(file, (const uint8_t*)"\"", 1);
    
    const char* p = str;
    while(*p) {
        if(*p == '"' || *p == '\\') {
            storage_file_write(file, (const uint8_t*)"\\", 1);
        }
        storage_file_write(file, (const uint8_t*)p, 1);
        p++;
    }
    
    storage_file_write(file, (const uint8_t*)"\"", 1);
}

// Save data to JSON file (saves cached slots)
bool flipchanger_save_data(FlipChangerApp* app) {
    if(!app || !app->storage) {
        return false;
    }
    
    // Open file for writing
    File* file = storage_file_alloc(app->storage);
    
    // Create directory if needed
    storage_common_mkdir(app->storage, "/ext/apps/Tools");
    
    if(!storage_file_open(file, FLIPCHANGER_DATA_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_free(file);
        return false;
    }
    
    // Write JSON header
    char header[128];
    snprintf(header, sizeof(header), "{\"version\":1,\"total_slots\":%ld,\"slots\":[", (long)app->total_slots);
    storage_file_write(file, (const uint8_t*)header, strlen(header));
    
    // Write cached slots
    bool first = true;
    for(int32_t i = 0; i < SLOT_CACHE_SIZE && i < app->total_slots; i++) {
        Slot* slot = &app->slots[i];
        
        if(!first) {
            storage_file_write(file, (const uint8_t*)",", 1);
        }
        first = false;
        
        storage_file_write(file, (const uint8_t*)"{", 1);
        
        // Slot number
        char slot_num[32];
        snprintf(slot_num, sizeof(slot_num), "\"slot\":%ld,", (long)slot->slot_number);
        storage_file_write(file, (const uint8_t*)slot_num, strlen(slot_num));
        
        // Occupied
        char occ_str[24];
        snprintf(occ_str, sizeof(occ_str), "\"occupied\":%s,", slot->occupied ? "true" : "false");
        storage_file_write(file, (const uint8_t*)occ_str, strlen(occ_str));
        
        if(slot->occupied) {
            // Artist
            storage_file_write(file, (const uint8_t*)"\"artist\":", 9);
            write_json_string(file, slot->cd.artist);
            storage_file_write(file, (const uint8_t*)",", 1);
            
            // Album
            storage_file_write(file, (const uint8_t*)"\"album\":", 8);
            write_json_string(file, slot->cd.album);
            storage_file_write(file, (const uint8_t*)",", 1);
            
            // Year
            char year_str[32];
            snprintf(year_str, sizeof(year_str), "\"year\":%ld,", (long)slot->cd.year);
            storage_file_write(file, (const uint8_t*)year_str, strlen(year_str));
            
            // Genre
            storage_file_write(file, (const uint8_t*)"\"genre\":", 8);
            write_json_string(file, slot->cd.genre);
            storage_file_write(file, (const uint8_t*)",", 1);
            
            // Tracks array
            storage_file_write(file, (const uint8_t*)"\"tracks\":[", 10);
            bool first_track = true;
            for(int32_t t = 0; t < slot->cd.track_count && t < MAX_TRACKS; t++) {
                if(!first_track) {
                    storage_file_write(file, (const uint8_t*)",", 1);
                }
                first_track = false;
                
                storage_file_write(file, (const uint8_t*)"{", 1);
                
                // Track number
                char track_num[32];
                snprintf(track_num, sizeof(track_num), "\"num\":%ld,", (long)slot->cd.tracks[t].number);
                storage_file_write(file, (const uint8_t*)track_num, strlen(track_num));
                
                // Track title
                storage_file_write(file, (const uint8_t*)"\"title\":", 8);
                write_json_string(file, slot->cd.tracks[t].title);
                storage_file_write(file, (const uint8_t*)",", 1);
                
                // Track duration
                storage_file_write(file, (const uint8_t*)"\"duration\":", 11);
                write_json_string(file, slot->cd.tracks[t].duration);
                
                storage_file_write(file, (const uint8_t*)"}", 1);
            }
            storage_file_write(file, (const uint8_t*)"],", 2);
            
            // Notes
            storage_file_write(file, (const uint8_t*)"\"notes\":", 8);
            write_json_string(file, slot->cd.notes);
        }
        
        storage_file_write(file, (const uint8_t*)"}", 1);
    }
    
    // Write JSON footer
    storage_file_write(file, (const uint8_t*)"]}", 2);
    
    storage_file_close(file);
    storage_file_free(file);
    
    app->dirty = false;
    return true;
}

// Draw main menu
void flipchanger_draw_main_menu(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    // Title
    canvas_draw_str(canvas, 30, 10, "FlipChanger");
    
    // Menu options
    canvas_set_font(canvas, FontSecondary);
    
    int32_t y = 25;
    const char* menu_items[] = {
        "View Slots",
        "Add CD",
        "Statistics",
        "Settings"
    };
    
    int32_t selected = app->selected_index % 4;
    
    for(int32_t i = 0; i < 4; i++) {
        if(i == selected) {
            canvas_draw_box(canvas, 5, y - 8, 118, 10);
            canvas_invert_color(canvas);
        }
        canvas_draw_str(canvas, 10, y, menu_items[i]);
        if(i == selected) {
            canvas_invert_color(canvas);
        }
        y += 12;
    }
    
    // Footer
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str(canvas, 5, 60, "UP/DN:Select OK:Go BACK:Exit");
}

// Draw slot list
void flipchanger_draw_slot_list(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    // Header
    char header[32];
    snprintf(header, sizeof(header), "Slots (%ld total)", app->total_slots);
    canvas_draw_str(canvas, 5, 10, header);
    
    // Calculate visible slots (5 per screen)
    int32_t visible_count = 5;
    int32_t start_index = app->scroll_offset;
    int32_t end_index = start_index + visible_count;
    if(end_index > app->total_slots) {
        end_index = app->total_slots;
    }
    
    canvas_set_font(canvas, FontSecondary);
    int32_t y = 20;
    
    // Don't update cache during draw - only read from cache
    // Cache should be updated before entering this view
    
    for(int32_t i = start_index; i < end_index; i++) {
        char line[80];  // Increased buffer size
        Slot* slot = flipchanger_get_slot(app, i);
        
        if(slot && slot->occupied) {
            // Truncate artist name if too long to fit
            char artist_short[40];
            snprintf(artist_short, sizeof(artist_short), "%.39s", slot->cd.artist);
            snprintf(line, sizeof(line), "%ld: %s", (long)(i + 1), artist_short);
        } else {
            snprintf(line, sizeof(line), "%ld: [Empty]", (long)(i + 1));
        }
        
        if(i == app->selected_index) {
            canvas_draw_box(canvas, 2, y - 8, 124, 9);
            canvas_invert_color(canvas);
        }
        
        canvas_draw_str(canvas, 5, y, line);
        
        if(i == app->selected_index) {
            canvas_invert_color(canvas);
        }
        
        y += 10;
    }
    
    // Footer
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str(canvas, 5, 60, "UP/DN:Nav OK:View BACK:Menu");
}

// Draw slot details
void flipchanger_draw_slot_details(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    
    if(app->current_slot_index < 0 || app->current_slot_index >= app->total_slots) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "Invalid slot");
        return;
    }
    
    // Get slot from cache (cache should already be updated)
    Slot* slot = flipchanger_get_slot(app, app->current_slot_index);
    
    if(!slot) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "Slot not loaded");
        return;
    }
    
    canvas_set_font(canvas, FontPrimary);
    
    // Slot number
    char slot_str[16];
    snprintf(slot_str, sizeof(slot_str), "Slot %ld", (long)slot->slot_number);
    canvas_draw_str(canvas, 5, 10, slot_str);
    
    if(!slot->occupied) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 5, 30, "[Empty Slot]");
        canvas_set_font(canvas, FontKeyboard);
        canvas_draw_str(canvas, 5, 50, "OK:Add CD BACK:Return");
        return;
    }
    
    // CD information
    canvas_set_font(canvas, FontSecondary);
    int32_t y = 22;
    
    // Artist
    canvas_draw_str(canvas, 5, y, "Artist:");
    if(strlen(slot->cd.artist) > 0) {
        canvas_draw_str(canvas, 35, y, slot->cd.artist);
    }
    y += 10;
    
    // Album
    canvas_draw_str(canvas, 5, y, "Album:");
    if(strlen(slot->cd.album) > 0) {
        canvas_draw_str(canvas, 35, y, slot->cd.album);
    }
    y += 10;
    
    // Year
    if(slot->cd.year > 0) {
        char year_str[24];
        snprintf(year_str, sizeof(year_str), "Year: %ld", (long)slot->cd.year);
        canvas_draw_str(canvas, 5, y, year_str);
        y += 10;
    }
    
    // Genre
    if(strlen(slot->cd.genre) > 0) {
        canvas_draw_str(canvas, 5, y, "Genre:");
        canvas_draw_str(canvas, 35, y, slot->cd.genre);
        y += 10;
    }
    
    // Tracks
    if(slot->cd.track_count > 0) {
        char tracks_str[32];
        snprintf(tracks_str, sizeof(tracks_str), "Tracks: %ld", (long)slot->cd.track_count);
        canvas_draw_str(canvas, 5, y, tracks_str);
    }
    
    // Footer
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str(canvas, 5, 60, "OK:Edit BACK:Return");
}

// Draw callback
void flipchanger_draw_callback(Canvas* canvas, void* ctx) {
    FlipChangerApp* app = (FlipChangerApp*)ctx;
    
    switch(app->current_view) {
        case VIEW_MAIN_MENU:
            flipchanger_draw_main_menu(canvas, app);
            break;
        case VIEW_SLOT_LIST:
            flipchanger_draw_slot_list(canvas, app);
            break;
        case VIEW_SLOT_DETAILS:
            flipchanger_draw_slot_details(canvas, app);
            break;
        case VIEW_ADD_EDIT_CD:
            flipchanger_draw_add_edit(canvas, app);
            break;
        default:
            canvas_clear(canvas);
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str(canvas, 5, 30, "Unknown view");
            break;
    }
}

// Navigation functions
void flipchanger_show_main_menu(FlipChangerApp* app) {
    app->current_view = VIEW_MAIN_MENU;
    app->selected_index = 0;
}

void flipchanger_show_slot_list(FlipChangerApp* app) {
    app->current_view = VIEW_SLOT_LIST;
    app->selected_index = 0;
    app->scroll_offset = 0;
}

void flipchanger_show_slot_details(FlipChangerApp* app, int32_t slot_index) {
    app->current_view = VIEW_SLOT_DETAILS;
    app->current_slot_index = slot_index;
}

// Character set for text input
static const char* CHAR_SET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .-,";

void flipchanger_show_add_edit(FlipChangerApp* app, int32_t slot_index, bool is_new) {
    app->current_view = VIEW_ADD_EDIT_CD;
    app->current_slot_index = slot_index;
    app->edit_field = FIELD_ARTIST;
    app->edit_char_pos = 0;
    app->edit_char_selection = 0;
    
    // Ensure slot is in cache
    flipchanger_update_cache(app, slot_index);
    Slot* slot = flipchanger_get_slot(app, slot_index);
    
    if(slot && is_new) {
        // Initialize new slot
        slot->occupied = true;
        slot->slot_number = slot_index + 1;
        memset(&slot->cd, 0, sizeof(CD));
        slot->cd.year = 0;
        slot->cd.track_count = 0;
        slot->cd.artist[0] = '\0';
        slot->cd.album[0] = '\0';
        slot->cd.genre[0] = '\0';
        slot->cd.notes[0] = '\0';
    }
}

// Draw Add/Edit CD view
void flipchanger_draw_add_edit(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    
    if(app->current_slot_index < 0 || app->current_slot_index >= app->total_slots) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "Invalid slot");
        return;
    }
    
    Slot* slot = flipchanger_get_slot(app, app->current_slot_index);
    if(!slot) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "Slot not loaded");
        return;
    }
    
    canvas_set_font(canvas, FontPrimary);
    
    // Title
    char title[32];
    snprintf(title, sizeof(title), "Slot %ld", (long)slot->slot_number);
    canvas_draw_str(canvas, 5, 10, title);
    
    canvas_set_font(canvas, FontSecondary);
    int32_t y = 22;
    
    // Field labels and values
    const char* field_labels[] = {
        "Artist:",
        "Album:",
        "Year:",
        "Genre:",
        "Notes:"
    };
    
    char* field_values[] = {
        slot->cd.artist,
        slot->cd.album,
        NULL,  // Year handled separately
        slot->cd.genre,
        slot->cd.notes
    };
    
    // Draw fields
    for(int32_t i = 0; i < FIELD_SAVE && i < 5; i++) {
        bool is_selected = (app->edit_field == i);
        
        // Highlight selected field
        if(is_selected) {
            canvas_draw_box(canvas, 2, y - 9, 124, 9);
            canvas_invert_color(canvas);
        }
        
        canvas_draw_str(canvas, 5, y, field_labels[i]);
        
        // Draw field value
        if(i == FIELD_YEAR) {
            char year_str[32];
            if(slot->cd.year > 0) {
                snprintf(year_str, sizeof(year_str), "%ld", (long)slot->cd.year);
            } else {
                snprintf(year_str, sizeof(year_str), "0");
            }
            
            // Show cursor for year
            int32_t x_pos = 40;
            canvas_draw_str(canvas, x_pos, y, year_str);
            if(is_selected) {
                int32_t cursor_x = x_pos + (app->edit_char_pos * 6);
                if(cursor_x < 128) {
                    canvas_draw_line(canvas, cursor_x, y, cursor_x, y - 8);
                }
                
                // Show number picker (0-9)
                if(app->edit_char_selection >= 26 && app->edit_char_selection < 36) {
                    int32_t digit = app->edit_char_selection - 26;
                    char digit_display[4];
                    snprintf(digit_display, sizeof(digit_display), "[%ld]", (long)digit);
                    canvas_draw_str(canvas, 100, y, digit_display);
                }
            }
        } else {
            char* value = field_values[i];
            int32_t max_len = 0;
            
            // Get max length for field
            switch(i) {
                case FIELD_ARTIST: max_len = MAX_ARTIST_LENGTH; break;
                case FIELD_ALBUM: max_len = MAX_ALBUM_LENGTH; break;
                case FIELD_GENRE: max_len = MAX_GENRE_LENGTH; break;
                case FIELD_NOTES: max_len = MAX_NOTES_LENGTH; break;
                default: max_len = 32; break;
            }
            
            // Display value (truncate if too long for display)
            char display[64];
            snprintf(display, sizeof(display), "%.30s", value);
            canvas_draw_str(canvas, 40, y, display);
            
            // Show cursor position
            if(is_selected) {
                int32_t x_pos = 40 + (app->edit_char_pos * 6);
                if(x_pos < 128) {
                    canvas_draw_line(canvas, x_pos, y, x_pos, y - 8);
                }
                
                // Show character picker (only if not navigating)
                if(app->edit_char_selection > 0 || strlen(value) == 0 || app->edit_char_pos < (int32_t)strlen(value)) {
                    int32_t char_idx = app->edit_char_selection % strlen(CHAR_SET);
                    char char_display[4];
                    snprintf(char_display, sizeof(char_display), "[%c]", CHAR_SET[char_idx]);
                    canvas_draw_str(canvas, 100, y, char_display);
                }
            }
            
            UNUSED(max_len);  // For future use
        }
        
        if(is_selected) {
            canvas_invert_color(canvas);
        }
        
        y += 10;
    }
    
    // Save button
    bool save_selected = (app->edit_field == FIELD_SAVE);
    y = 58;
    if(save_selected) {
        canvas_draw_box(canvas, 2, y - 8, 124, 8);
        canvas_invert_color(canvas);
    }
    canvas_draw_str(canvas, 5, y, "Save");
    if(save_selected) {
        canvas_invert_color(canvas);
    }
    
    // Footer
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str(canvas, 5, 64, "UP/DN:Field LEFT/R:Pos OK:Add BACK:Del");
}

// Input callback
void flipchanger_input_callback(InputEvent* input_event, void* ctx) {
    FlipChangerApp* app = (FlipChangerApp*)ctx;
    
    if(input_event->type != InputTypePress) {
        return;
    }
    
    switch(app->current_view) {
        case VIEW_MAIN_MENU:
            if(input_event->key == InputKeyUp) {
                app->selected_index = (app->selected_index + 3) % 4;  // Wrap around
            } else if(input_event->key == InputKeyDown) {
                app->selected_index = (app->selected_index + 1) % 4;
            } else if(input_event->key == InputKeyOk) {
                switch(app->selected_index) {
                    case 0:  // View Slots
                        flipchanger_show_slot_list(app);
                        break;
                    case 1:  // Add CD
                        flipchanger_show_slot_list(app);  // Show slots first to select
                        break;
                    case 2:  // Statistics
                        // TODO: Show statistics
                        break;
                    case 3:  // Settings
                        // TODO: Show settings
                        break;
                }
            } else if(input_event->key == InputKeyBack) {
                app->running = false;
            }
            break;
            
        case VIEW_SLOT_LIST:
            if(input_event->key == InputKeyUp) {
                if(app->selected_index > 0) {
                    app->selected_index--;
                    // Auto-scroll
                    if(app->selected_index < app->scroll_offset) {
                        app->scroll_offset = app->selected_index;
                    }
                }
            } else if(input_event->key == InputKeyDown) {
                if(app->selected_index < app->total_slots - 1) {
                    app->selected_index++;
                    // Auto-scroll
                    if(app->selected_index >= app->scroll_offset + 5) {
                        app->scroll_offset = app->selected_index - 4;
                    }
                }
            } else if(input_event->key == InputKeyOk) {
                // Update cache before viewing
                flipchanger_update_cache(app, app->selected_index);
                flipchanger_show_slot_details(app, app->selected_index);
            } else if(input_event->key == InputKeyBack) {
                flipchanger_show_main_menu(app);
            }
            break;
            
        case VIEW_SLOT_DETAILS: {
            Slot* slot = flipchanger_get_slot(app, app->current_slot_index);
            if(input_event->key == InputKeyOk) {
                // If empty, go to add. If occupied, go to edit
                if(!slot || !slot->occupied) {
                    flipchanger_show_add_edit(app, app->current_slot_index, true);
                } else {
                    flipchanger_show_add_edit(app, app->current_slot_index, false);
                }
            } else if(input_event->key == InputKeyBack) {
                flipchanger_show_slot_list(app);
            }
            break;
        }
            
        case VIEW_ADD_EDIT_CD: {
            Slot* slot = flipchanger_get_slot(app, app->current_slot_index);
            if(!slot) {
                if(input_event->key == InputKeyBack) {
                    flipchanger_show_slot_list(app);
                }
                break;
            }
            
            if(app->edit_field == FIELD_SAVE) {
                // Save button selected
                if(input_event->key == InputKeyOk) {
                    // Save the slot
                    slot->occupied = true;
                    app->dirty = true;
                    flipchanger_save_slot_to_sd(app, app->current_slot_index);
                    notification_message(app->notifications, &sequence_blink_green_100);
                    flipchanger_show_slot_details(app, app->current_slot_index);
                } else if(input_event->key == InputKeyUp) {
                    app->edit_field = FIELD_NOTES;
                } else if(input_event->key == InputKeyBack) {
                    flipchanger_show_slot_details(app, app->current_slot_index);
                }
            } else {
                // Editing a field
                if(input_event->key == InputKeyUp) {
                    // Navigate fields or change character
                    if(app->edit_char_selection == 0) {
                        // Navigate to previous field
                        if(app->edit_field > FIELD_ARTIST) {
                            app->edit_field = app->edit_field - 1;
                        } else {
                            app->edit_field = FIELD_SAVE;
                        }
                    } else {
                        // Change selected character (wrap around)
                        app->edit_char_selection = (app->edit_char_selection - 1 + strlen(CHAR_SET)) % strlen(CHAR_SET);
                    }
                } else if(input_event->key == InputKeyDown) {
                    // Navigate fields or change character
                    if(app->edit_char_selection == 0) {
                        // Navigate to next field
                        if(app->edit_field < FIELD_SAVE) {
                            app->edit_field = app->edit_field + 1;
                        } else {
                            app->edit_field = FIELD_ARTIST;
                        }
                    } else {
                        // Change selected character
                        app->edit_char_selection = (app->edit_char_selection + 1) % strlen(CHAR_SET);
                    }
                } else if(input_event->key == InputKeyLeft) {
                    // Move cursor left
                    if(app->edit_char_pos > 0) {
                        app->edit_char_pos--;
                        app->edit_char_selection = 0;
                    }
                } else if(input_event->key == InputKeyRight) {
                    // Move cursor right
                    char* field = NULL;
                    int32_t max_len = 0;
                    
                    switch(app->edit_field) {
                        case FIELD_ARTIST:
                            field = slot->cd.artist;
                            max_len = MAX_ARTIST_LENGTH;
                            break;
                        case FIELD_ALBUM:
                            field = slot->cd.album;
                            max_len = MAX_ALBUM_LENGTH;
                            break;
                        case FIELD_GENRE:
                            field = slot->cd.genre;
                            max_len = MAX_GENRE_LENGTH;
                            break;
                        case FIELD_NOTES:
                            field = slot->cd.notes;
                            max_len = MAX_NOTES_LENGTH;
                            break;
                        default:
                            break;
                    }
                    
                    if(field && app->edit_char_pos < (int32_t)strlen(field) && app->edit_char_pos < max_len - 1) {
                        app->edit_char_pos++;
                        app->edit_char_selection = 0;
                    }
                } else if(input_event->key == InputKeyOk) {
                    // Add/insert character
                    if(app->edit_field == FIELD_YEAR) {
                        // Year input (numeric)
                        if(app->edit_char_selection >= 26 && app->edit_char_selection < 36) {
                            // Number selected (0-9)
                            int32_t digit = app->edit_char_selection - 26;
                            slot->cd.year = slot->cd.year * 10 + digit;
                            if(slot->cd.year > 9999) slot->cd.year = 9999;
                        }
                    } else {
                        // Text field
                        char* field = NULL;
                        int32_t max_len = 0;
                        
                        switch(app->edit_field) {
                            case FIELD_ARTIST:
                                field = slot->cd.artist;
                                max_len = MAX_ARTIST_LENGTH;
                                break;
                            case FIELD_ALBUM:
                                field = slot->cd.album;
                                max_len = MAX_ALBUM_LENGTH;
                                break;
                            case FIELD_GENRE:
                                field = slot->cd.genre;
                                max_len = MAX_GENRE_LENGTH;
                                break;
                            case FIELD_NOTES:
                                field = slot->cd.notes;
                                max_len = MAX_NOTES_LENGTH;
                                break;
                            default:
                                break;
                        }
                        
                        if(field && app->edit_char_pos < max_len - 1) {
                            int32_t len = strlen(field);
                            char ch = CHAR_SET[app->edit_char_selection % strlen(CHAR_SET)];
                            
                            // Insert character at cursor position
                            if(app->edit_char_pos <= len) {
                                // Shift existing characters
                                for(int32_t i = len; i >= app->edit_char_pos && i < max_len - 1; i--) {
                                    field[i + 1] = field[i];
                                }
                                field[app->edit_char_pos] = ch;
                                field[len + 1] = '\0';
                                app->edit_char_pos++;
                            }
                        }
                    }
                } else if(input_event->key == InputKeyBack) {
                    // Delete character at cursor
                    if(app->edit_field == FIELD_YEAR) {
                        // Delete digit from year
                        slot->cd.year = slot->cd.year / 10;
                        if(app->edit_char_pos > 0) app->edit_char_pos--;
                    } else {
                        char* field = NULL;
                        
                        switch(app->edit_field) {
                            case FIELD_ARTIST: field = slot->cd.artist; break;
                            case FIELD_ALBUM: field = slot->cd.album; break;
                            case FIELD_GENRE: field = slot->cd.genre; break;
                            case FIELD_NOTES: field = slot->cd.notes; break;
                            default: break;
                        }
                        
                        if(field) {
                            int32_t len = strlen(field);
                            if(app->edit_char_pos < len && app->edit_char_pos >= 0) {
                                // Delete character at cursor
                                for(int32_t i = app->edit_char_pos; i < len; i++) {
                                    field[i] = field[i + 1];
                                }
                            } else if(app->edit_char_pos > 0 && len > 0) {
                                // Delete character before cursor
                                app->edit_char_pos--;
                                for(int32_t i = app->edit_char_pos; i < len; i++) {
                                    field[i] = field[i + 1];
                                }
                            }
                        }
                    }
                }
            }
            
            view_port_update(app->view_port);
            break;
        }
            
        default:
            break;
    }
    
    view_port_update(app->view_port);
}

// Main entry point
int32_t flipchanger_main(void* p) {
    UNUSED(p);
    
    // Allocate app structure
    FlipChangerApp* app = malloc(sizeof(FlipChangerApp));
    memset(app, 0, sizeof(FlipChangerApp));
    
    // Initialize services
    app->gui = furi_record_open(RECORD_GUI);
    app->storage = furi_record_open(RECORD_STORAGE);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->running = true;
    app->dirty = false;
    
    // Create view port
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, flipchanger_draw_callback, app);
    view_port_input_callback_set(app->view_port, flipchanger_input_callback, app);
    
    // Attach view port to GUI
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);
    
    // Load data
    flipchanger_load_data(app);
    
    // Send notification that app started
    notification_message(app->notifications, &sequence_blink_green_100);
    
    // Start with main menu
    flipchanger_show_main_menu(app);
    view_port_update(app->view_port);
    
    // Main event loop
    while(app->running) {
        furi_delay_ms(100);
    }
    
    // Prevent further input processing
    app->running = false;
    
    // Cleanup GUI first (prevent further draw calls)
    if(app->gui && app->view_port) {
        gui_remove_view_port(app->gui, app->view_port);
    }
    if(app->view_port) {
        view_port_free(app->view_port);
    }
    
    // Save data if dirty (before closing storage)
    if(app->dirty && app->storage) {
        flipchanger_save_data(app);
    }
    
    // Close records
    if(app->gui) {
        furi_record_close(RECORD_GUI);
    }
    if(app->notifications) {
        furi_record_close(RECORD_NOTIFICATION);
    }
    if(app->storage) {
        furi_record_close(RECORD_STORAGE);
    }
    
    // Free app structure
    free(app);
    
    return 0;
}
