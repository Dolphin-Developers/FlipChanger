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
    app->details_scroll_offset = 0;
    app->editing_slot_count = false;
    app->edit_slot_count_pos = 0;
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
    
    // Read file into buffer (must fit in stack - reduced to 2KB)
    uint8_t buffer[2048];  // 2KB buffer - fits in stack, read incrementally if needed
    uint16_t bytes_read = storage_file_read(file, buffer, sizeof(buffer) - 1);
    if(bytes_read >= sizeof(buffer) - 1) {
        // File too large - truncate and warn (shouldn't happen with reasonable data)
        bytes_read = sizeof(buffer) - 1;
    }
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
    
    // Note: Allow saving even if !running (needed for shutdown save)
    
    // Open file for writing
    File* file = storage_file_alloc(app->storage);
    if(!file) {
        return false;
    }
    
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
    
    // Close file (this should flush automatically)
    bool result = storage_file_close(file);
    storage_file_free(file);
    
    if(result) {
        app->dirty = false;
    }
    
    return result;
}

// Forward declarations
void flipchanger_draw_track_management(Canvas* canvas, FlipChangerApp* app);
void flipchanger_draw_settings(Canvas* canvas, FlipChangerApp* app);
void flipchanger_draw_statistics(Canvas* canvas, FlipChangerApp* app);

// Draw main menu
void flipchanger_draw_main_menu(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    // Title
    canvas_draw_str(canvas, 30, 10, "FlipChanger");
    
    // Menu options
    canvas_set_font(canvas, FontSecondary);
    
    int32_t y = 22;  // Adjusted starting position
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
        y += 10;  // Reduced spacing from 12 to 10 to prevent overlap
    }
    
    // Footer - two lines with abbreviations
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str(canvas, 5, 57, "U/D:Select K:Go B:Exit");
    canvas_draw_str(canvas, 5, 63, "LB:Exit");
}

// Draw slot list
void flipchanger_draw_slot_list(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    // Header
    char header[32];
    snprintf(header, sizeof(header), "Slots (%ld total)", app->total_slots);
    canvas_draw_str(canvas, 5, 10, header);
    
    // Calculate visible slots (4 per screen to leave room for footer)
    int32_t visible_count = 4;
    int32_t start_index = app->scroll_offset;
    int32_t end_index = start_index + visible_count;
    if(end_index > app->total_slots) {
        end_index = app->total_slots;
    }
    
    canvas_set_font(canvas, FontSecondary);
    int32_t y = 18;  // Start slightly higher
    
    // Don't update cache during draw - only read from cache
    // Cache should be updated before entering this view
    
    // Ensure we only show exactly 4 items (or fewer if total_slots < 4)
    int32_t items_to_show = (end_index - start_index);
    if(items_to_show > 4) {
        items_to_show = 4;
        end_index = start_index + 4;
    }
    
    for(int32_t i = start_index; i < end_index && (i - start_index) < 4; i++) {
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
        
        y += 11;  // Slightly more spacing to ensure clear separation
    }
    
    // Footer - two lines with abbreviations
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str(canvas, 5, 57, "U/D:Nav K:View B:Return");
    canvas_draw_str(canvas, 5, 63, "LB:Exit");
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
    
    // CD information - scrollable list (show 3 items at a time)
    canvas_set_font(canvas, FontSecondary);
    
    // Build list of fields to display
    typedef struct {
        const char* label;
        char value[64];
        bool visible;
    } DetailField;
    
    DetailField fields[5];
    int32_t field_count = 0;
    
    // Artist
    if(strlen(slot->cd.artist) > 0) {
        fields[field_count].label = "Artist:";
        strncpy(fields[field_count].value, slot->cd.artist, sizeof(fields[field_count].value) - 1);
        fields[field_count].value[sizeof(fields[field_count].value) - 1] = '\0';
        fields[field_count].visible = true;
        field_count++;
    }
    
    // Album
    if(strlen(slot->cd.album) > 0) {
        fields[field_count].label = "Album:";
        strncpy(fields[field_count].value, slot->cd.album, sizeof(fields[field_count].value) - 1);
        fields[field_count].value[sizeof(fields[field_count].value) - 1] = '\0';
        fields[field_count].visible = true;
        field_count++;
    }
    
    // Year
    if(slot->cd.year > 0) {
        fields[field_count].label = "Year:";
        snprintf(fields[field_count].value, sizeof(fields[field_count].value), "%ld", (long)slot->cd.year);
        fields[field_count].visible = true;
        field_count++;
    }
    
    // Genre
    if(strlen(slot->cd.genre) > 0) {
        fields[field_count].label = "Genre:";
        strncpy(fields[field_count].value, slot->cd.genre, sizeof(fields[field_count].value) - 1);
        fields[field_count].value[sizeof(fields[field_count].value) - 1] = '\0';
        fields[field_count].visible = true;
        field_count++;
    }
    
    // Tracks
    if(slot->cd.track_count > 0) {
        fields[field_count].label = "Tracks:";
        snprintf(fields[field_count].value, sizeof(fields[field_count].value), "%ld", (long)slot->cd.track_count);
        fields[field_count].visible = true;
        field_count++;
    }
    
    // Show 3 items at a time with scrolling
    const int32_t VISIBLE_ITEMS = 3;
    int32_t start_index = app->details_scroll_offset;
    int32_t end_index = start_index + VISIBLE_ITEMS;
    if(end_index > field_count) {
        end_index = field_count;
    }
    
    int32_t y = 22;
    for(int32_t i = start_index; i < end_index; i++) {
        canvas_draw_str(canvas, 5, y, fields[i].label);
        canvas_draw_str(canvas, 35, y, fields[i].value);
        y += 10;
    }
    
    // Footer - two lines with abbreviations
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str(canvas, 5, 57, "U/D:Scroll K:Edit B:Return");
    canvas_draw_str(canvas, 5, 63, "LB:Exit");
}

// Draw callback
void flipchanger_draw_callback(Canvas* canvas, void* ctx) {
    FlipChangerApp* app = (FlipChangerApp*)ctx;
    
    // Safety check - don't draw if app is exiting
    if(!app || !app->running) {
        canvas_clear(canvas);
        return;
    }
    
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
        case VIEW_TRACK_MANAGEMENT:
            flipchanger_draw_track_management(canvas, app);
            break;
        case VIEW_SETTINGS:
            flipchanger_draw_settings(canvas, app);
            break;
        case VIEW_STATISTICS:
            flipchanger_draw_statistics(canvas, app);
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
    app->details_scroll_offset = 0;
}

// Character set for text input
// Special: Last character index will be used for DEL (delete)
static const char* CHAR_SET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .-,";
#define CHAR_DEL_INDEX ((int32_t)strlen(CHAR_SET))  // DEL is one past the end of the set

void flipchanger_show_add_edit(FlipChangerApp* app, int32_t slot_index, bool is_new) {
    app->current_view = VIEW_ADD_EDIT_CD;
    app->current_slot_index = slot_index;
    app->edit_field = FIELD_ARTIST;
    app->edit_char_pos = 0;
    app->edit_char_selection = 0;
    app->edit_field_scroll = 0;
    app->edit_selected_track = 0;
    app->editing_track = false;
    app->edit_track_field = TRACK_FIELD_TITLE;
    
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
    
    // Safety checks
    if(!app) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "App error");
        return;
    }
    
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
    
    // Ensure edit_field is valid (cast to int for comparison)
    int32_t edit_field_int = (int32_t)app->edit_field;
    if(edit_field_int < (int32_t)FIELD_ARTIST || edit_field_int >= (int32_t)FIELD_COUNT) {
        app->edit_field = FIELD_ARTIST;
    }
    
    // Ensure cursor position is valid
    if(app->edit_char_pos < 0) {
        app->edit_char_pos = 0;
    }
    
    canvas_set_font(canvas, FontPrimary);
    
    // Title
    char title[32];
    snprintf(title, sizeof(title), "Slot %ld", (long)slot->slot_number);
    canvas_draw_str(canvas, 5, 10, title);
    
    canvas_set_font(canvas, FontSecondary);
    int32_t y = 18;  // Start higher to leave room for footer
    
    // Field labels and values
    const char* field_labels[] = {
        "Artist:",
        "Album:",
        "Year:",
        "Genre:",
        "Notes:",
        "Tracks:"
    };
    
    char* field_values[] = {
        slot->cd.artist,
        slot->cd.album,
        NULL,  // Year handled separately
        slot->cd.genre,
        slot->cd.notes,
        NULL   // Tracks handled separately
    };
    
    // Draw fields (show 3 at a time with scrolling for Add/Edit view)
    const int32_t VISIBLE_FIELDS = 3;
    int32_t field_scroll_offset = 0;
    if((int32_t)app->edit_field >= VISIBLE_FIELDS) {
        field_scroll_offset = (int32_t)app->edit_field - VISIBLE_FIELDS + 1;
    }
    if(field_scroll_offset < 0) field_scroll_offset = 0;
    
    int32_t start_field = field_scroll_offset;
    int32_t end_field = start_field + VISIBLE_FIELDS;
    if(end_field > FIELD_SAVE) end_field = FIELD_SAVE;
    
    for(int32_t i = start_field; i < end_field; i++) {
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
                // Show cursor at end of year string
                int32_t year_len = strlen(year_str);
                int32_t cursor_x = x_pos + (year_len * 6);
                if(cursor_x < 128) {
                    canvas_draw_line(canvas, cursor_x, y, cursor_x, y - 8);
                }
                
                // Show number picker (0-9) - ensure selection is in valid range
                int32_t char_selection = app->edit_char_selection;
                if(char_selection < 26 || char_selection >= 36) {
                    char_selection = 26; // Default to '0' if out of range
                }
                if(char_selection >= 26 && char_selection < 36) {
                    int32_t digit = char_selection - 26;
                    char digit_display[4];
                    snprintf(digit_display, sizeof(digit_display), "[%ld]", (long)digit);
                    canvas_draw_str(canvas, 100, y, digit_display);
                }
            }
        } else {
            char* value = field_values[i];
            int32_t max_len = 0;
            
            // Safety check - ensure value is valid
            if(!value && i != FIELD_YEAR && i != FIELD_TRACKS) {
                // Should not happen, but handle gracefully
                continue;
            }
            
            // Get max length for field
            switch(i) {
                case FIELD_ARTIST: max_len = MAX_ARTIST_LENGTH; break;
                case FIELD_ALBUM: max_len = MAX_ALBUM_LENGTH; break;
                case FIELD_GENRE: max_len = MAX_GENRE_LENGTH; break;
                case FIELD_NOTES: max_len = MAX_NOTES_LENGTH; break;
                default: max_len = 32; break;
            }
            
            // Display value with scrolling for long text
            if(value) {
                // Ensure string is null-terminated
                value[max_len - 1] = '\0';
                
                int32_t value_len = strlen(value);
                
                // Calculate visible width: screen is 128px, text starts at x=40, char picker at x=90 (38px)
                // So we have about 50px for text = ~8-9 characters (FontSecondary is ~6px per char)
                // Let's use 12 characters to be safe (leaves room for char picker)
                const int32_t VISIBLE_CHARS = 12;
                
                // Auto-scroll to keep cursor visible when editing
                if(is_selected) {
                    // Ensure cursor position is within bounds
                    if(app->edit_char_pos > value_len) {
                        app->edit_char_pos = value_len;
                    }
                    
                    // Auto-scroll: if cursor is before visible area, scroll left
                    if(app->edit_char_pos < app->edit_field_scroll) {
                        app->edit_field_scroll = app->edit_char_pos;
                    }
                    // Auto-scroll: if cursor is at or after visible area, scroll right
                    else if(app->edit_char_pos >= app->edit_field_scroll + VISIBLE_CHARS) {
                        app->edit_field_scroll = app->edit_char_pos - VISIBLE_CHARS + 1;
                    }
                    // Also scroll if cursor is at the end of visible area
                    else if(app->edit_char_pos == app->edit_field_scroll + VISIBLE_CHARS - 1 && 
                            app->edit_char_pos < value_len) {
                        // If we're at the last visible character and there's more text, scroll forward
                        if(value_len > VISIBLE_CHARS) {
                            app->edit_field_scroll = app->edit_char_pos - VISIBLE_CHARS + 2;
                        }
                    }
                    
                    // Ensure scroll doesn't go negative or beyond text
                    if(app->edit_field_scroll < 0) {
                        app->edit_field_scroll = 0;
                    }
                    int32_t max_scroll = (value_len > VISIBLE_CHARS) ? value_len - VISIBLE_CHARS : 0;
                    if(app->edit_field_scroll > max_scroll) {
                        app->edit_field_scroll = max_scroll;
                    }
                }
                
                // Calculate display start and length
                int32_t display_start = app->edit_field_scroll;
                int32_t display_len = value_len - display_start;
                if(display_len > VISIBLE_CHARS) {
                    display_len = VISIBLE_CHARS;
                }
                
                // Display the visible portion
                char display[64];
                if(display_start < value_len && display_len > 0) {
                    // Copy visible portion
                    strncpy(display, value + display_start, display_len);
                    display[display_len] = '\0';
                } else {
                    display[0] = '\0';
                }
                
                canvas_draw_str(canvas, 40, y, display);
                
                // Show cursor position (relative to visible area)
                if(is_selected) {
                    int32_t cursor_rel_pos = app->edit_char_pos - app->edit_field_scroll;
                    int32_t x_pos = 40 + (cursor_rel_pos * 6);
                    if(x_pos >= 40 && x_pos < 90) {  // Only show if within visible area
                        canvas_draw_line(canvas, x_pos, y, x_pos, y - 8);
                    }
                    
                    // Show character picker (including DEL option)
                    int32_t char_set_len = strlen(CHAR_SET);
                    if(char_set_len > 0) {
                        char char_display[8];
                        if(app->edit_char_selection >= CHAR_DEL_INDEX) {
                            // Show DEL
                            snprintf(char_display, sizeof(char_display), "[DEL]");
                        } else {
                            // Show selected character
                            snprintf(char_display, sizeof(char_display), "[%c]", CHAR_SET[app->edit_char_selection]);
                        }
                        canvas_draw_str(canvas, 90, y, char_display);
                    }
                }
            }
            
            UNUSED(max_len);  // For future use
        }
        
        // Special handling for Tracks field
        if(i == FIELD_TRACKS) {
            char tracks_display[32];
            snprintf(tracks_display, sizeof(tracks_display), "%ld tracks", (long)slot->cd.track_count);
            canvas_draw_str(canvas, 40, y, tracks_display);
        }
        
        if(is_selected) {
            canvas_invert_color(canvas);
        }
        
        y += 10;  // Better spacing between fields (was 6, now 10 for readability)
    }
    
    // Save button - show if in visible range
    bool save_selected = (app->edit_field == FIELD_SAVE);
    if(FIELD_SAVE >= start_field && FIELD_SAVE < end_field) {
        // Save button is in visible range - draw it
        if(save_selected) {
            canvas_draw_box(canvas, 2, y - 8, 124, 8);
            canvas_invert_color(canvas);
        }
        canvas_draw_str(canvas, 5, y, "Save");
        if(save_selected) {
            canvas_invert_color(canvas);
        }
    }
    
    // Footer - two lines with abbreviations
    canvas_set_font(canvas, FontKeyboard);
    if(app->edit_field == FIELD_TRACKS) {
        canvas_draw_str(canvas, 5, 57, "K:Tracks B:Return");
        canvas_draw_str(canvas, 5, 63, "LB:Exit");
    } else if(app->edit_field == FIELD_ARTIST || app->edit_field == FIELD_ALBUM || 
                app->edit_field == FIELD_GENRE || app->edit_field == FIELD_NOTES) {
        // Field editing mode - show different hint based on state
        if(app->edit_char_pos == 0 && app->edit_char_selection == 0) {
            // Navigation mode - split across two lines
            canvas_draw_str(canvas, 5, 57, "U/D:Field K:Edit");
            canvas_draw_str(canvas, 5, 63, "B:Return LB:Exit");
        } else {
            // Editing mode - split across two lines
            canvas_draw_str(canvas, 5, 57, "U/D:Char K:Add");
            canvas_draw_str(canvas, 5, 63, "B:Return LB:Exit");
        }
    } else if(app->edit_field == FIELD_YEAR) {
        // Year field - numeric only - split across two lines
        canvas_draw_str(canvas, 5, 57, "U/D:Num K:Add");
        canvas_draw_str(canvas, 5, 63, "B:Del LB:Exit");
    } else {
        // Default footer - split across two lines
        canvas_draw_str(canvas, 5, 57, "U/D:Field K:Add");
        canvas_draw_str(canvas, 5, 63, "B:Return LB:Exit");
    }
}

// Draw Track Management view
void flipchanger_draw_track_management(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    
    // Safety checks
    if(!app || app->current_slot_index < 0 || app->current_slot_index >= app->total_slots) {
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
    
    // Ensure track_count is valid
    if(slot->cd.track_count < 0) slot->cd.track_count = 0;
    if(slot->cd.track_count > MAX_TRACKS) slot->cd.track_count = MAX_TRACKS;
    
    // Ensure selected track is valid
    if(app->edit_selected_track < 0) app->edit_selected_track = 0;
    if(slot->cd.track_count > 0 && app->edit_selected_track >= slot->cd.track_count) {
        app->edit_selected_track = slot->cd.track_count - 1;
    }
    
    canvas_set_font(canvas, FontPrimary);
    
    // Title
    char title[48];
    snprintf(title, sizeof(title), "Tracks (%ld)", (long)slot->cd.track_count);
    canvas_draw_str(canvas, 5, 10, title);
    
    canvas_set_font(canvas, FontSecondary);
    
    // Show tracks (up to 4 visible)
    int32_t y = 22;
    int32_t start_track = 0;
    if(slot->cd.track_count > 0 && app->edit_selected_track >= 4) {
        start_track = app->edit_selected_track - 3;
    }
    
    for(int32_t i = start_track; i < slot->cd.track_count && i < start_track + 4 && i >= 0 && i < MAX_TRACKS; i++) {
        bool is_selected = (i == app->edit_selected_track);
        
        if(is_selected) {
            canvas_draw_box(canvas, 2, y - 8, 124, 9);
            canvas_invert_color(canvas);
        }
        
        // Track number and title - ensure track pointer is valid
        char track_line[80];
        if(i >= 0 && i < MAX_TRACKS) {
            Track* track = &slot->cd.tracks[i];
            snprintf(track_line, sizeof(track_line), "%ld. %s", (long)track->number, track->title);
            canvas_draw_str(canvas, 5, y, track_line);
            
            // Duration on right
            if(strlen(track->duration) > 0) {
                canvas_draw_str(canvas, 100, y, track->duration);
            }
        }
        
        if(is_selected) {
            canvas_invert_color(canvas);
        }
        
        y += 10;
    }
    
    // Show editing interface if editing a track
    if(app->editing_track && app->edit_selected_track >= 0 && app->edit_selected_track < slot->cd.track_count) {
        Track* track = &slot->cd.tracks[app->edit_selected_track];
        if(track) {
            canvas_set_font(canvas, FontSecondary);
            int32_t edit_y = 50;
            
            // Show which field is being edited
            if(app->edit_track_field == TRACK_FIELD_TITLE) {
                canvas_draw_str(canvas, 5, edit_y, "Title:");
                char* field = track->title;
                int32_t field_len = strlen(field);
                
                // Display with scrolling
                const int32_t VISIBLE_CHARS = 15;
                int32_t display_start = 0;
                int32_t display_len = field_len;
                if(display_len > VISIBLE_CHARS) {
                    display_len = VISIBLE_CHARS;
                    // Auto-scroll to keep cursor visible
                    if(app->edit_char_pos >= VISIBLE_CHARS) {
                        display_start = app->edit_char_pos - VISIBLE_CHARS + 1;
                        display_len = VISIBLE_CHARS;
                    }
                }
                
                char display[64];
                if(display_start < field_len && display_len > 0) {
                    strncpy(display, field + display_start, display_len);
                    display[display_len] = '\0';
                } else {
                    display[0] = '\0';
                }
                
                canvas_draw_str(canvas, 40, edit_y, display);
                
                // Show cursor
                int32_t cursor_rel_pos = app->edit_char_pos - display_start;
                int32_t x_pos = 40 + (cursor_rel_pos * 6);
                if(x_pos >= 40 && x_pos < 120) {
                    canvas_draw_line(canvas, x_pos, edit_y, x_pos, edit_y - 8);
                }
                
                // Show character picker
                int32_t char_set_len = strlen(CHAR_SET);
                if(char_set_len > 0) {
                    char char_display[8];
                    if(app->edit_char_selection >= CHAR_DEL_INDEX) {
                        snprintf(char_display, sizeof(char_display), "[DEL]");
                    } else {
                        snprintf(char_display, sizeof(char_display), "[%c]", CHAR_SET[app->edit_char_selection]);
                    }
                    canvas_draw_str(canvas, 100, edit_y, char_display);
                }
            } else {
                // Duration field - numeric only (seconds)
                canvas_draw_str(canvas, 5, edit_y, "Duration (sec):");
                char* field = track->duration;
                
                // Display duration value
                canvas_draw_str(canvas, 70, edit_y, field);
                
                // Show number picker (0-9 only for duration)
                if(app->edit_char_selection >= 26 && app->edit_char_selection < 36) {
                    int32_t digit = app->edit_char_selection - 26;
                    char digit_display[4];
                    snprintf(digit_display, sizeof(digit_display), "[%ld]", (long)digit);
                    canvas_draw_str(canvas, 100, edit_y, digit_display);
                }
            }
        }
    }
    
    // Footer - two lines with abbreviations
    canvas_set_font(canvas, FontKeyboard);
    if(app->editing_track) {
        if(app->edit_track_field == TRACK_FIELD_DURATION) {
            canvas_draw_str(canvas, 5, 57, "U/D:Num K:Add B:Switch");
            canvas_draw_str(canvas, 5, 63, "LB:Exit");
        } else {
            canvas_draw_str(canvas, 5, 57, "U/D:Char K:Add B:Switch");
            canvas_draw_str(canvas, 5, 63, "LB:Exit");
        }
    } else {
        canvas_draw_str(canvas, 5, 57, "K:Edit +:Add -:Del B:Return");
        canvas_draw_str(canvas, 5, 63, "LB:Exit");
    }
}

// Input callback
void flipchanger_input_callback(InputEvent* input_event, void* ctx) {
    FlipChangerApp* app = (FlipChangerApp*)ctx;
    
    // Safety check - don't process input if app is exiting
    if(!app || !app->running) {
        return;
    }
    
    // Handle both short press and long press
    bool is_long_press = (input_event->type == InputTypeLong || input_event->type == InputTypeRepeat);
    bool is_short_press = (input_event->type == InputTypePress);
    
    if(!is_short_press && !is_long_press) {
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
                        app->current_view = VIEW_STATISTICS;
                        app->selected_index = 0;
                        break;
                    case 3:  // Settings
                        app->current_view = VIEW_SETTINGS;
                        app->selected_index = 0;
                        app->editing_slot_count = false;
                        app->edit_slot_count_pos = 0;
                        break;
                }
            } else if(input_event->key == InputKeyBack) {
                app->running = false;
                // Don't update view port after setting running to false
                return;
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
                    if(app->selected_index >= app->scroll_offset + 4) {
                        app->scroll_offset = app->selected_index - 3;
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
            // Safety check - ensure slot index is valid
            if(app->current_slot_index < 0 || app->current_slot_index >= app->total_slots) {
                if(input_event->key == InputKeyBack) {
                    app->current_view = VIEW_SLOT_LIST;
                }
                break;
            }
            
            Slot* slot = flipchanger_get_slot(app, app->current_slot_index);
            if(!slot) {
                if(input_event->key == InputKeyBack) {
                    app->current_view = VIEW_SLOT_LIST;
                }
                break;
            }
            
            // Ensure edit_field is valid (cast to int for comparison)
            int32_t edit_field_int = (int32_t)app->edit_field;
            if(edit_field_int < (int32_t)FIELD_ARTIST || edit_field_int >= (int32_t)FIELD_COUNT) {
                app->edit_field = FIELD_ARTIST;
            }
            
            // Ensure cursor position is valid
            if(app->edit_char_pos < 0) {
                app->edit_char_pos = 0;
            }
            
            // Ensure char_selection is valid
            int32_t char_set_len = strlen(CHAR_SET);
            if(char_set_len > 0 && app->edit_char_selection < 0) {
                app->edit_char_selection = 0;
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
                    app->edit_field = FIELD_TRACKS;
                    app->edit_field_scroll = 0;
                } else if(input_event->key == InputKeyDown) {
                    // Wrap to top
                    app->edit_field = FIELD_ARTIST;
                    app->edit_field_scroll = 0;
                } else if(input_event->key == InputKeyBack) {
                    flipchanger_show_slot_details(app, app->current_slot_index);
                }
            } else if(app->edit_field == FIELD_TRACKS) {
                // Tracks field selected
                if(input_event->key == InputKeyOk) {
                    // Enter track management view
                    app->current_view = VIEW_TRACK_MANAGEMENT;
                    app->edit_selected_track = 0;
                    app->editing_track = false;
                } else if(input_event->key == InputKeyUp) {
                    app->edit_field = FIELD_NOTES;
                    app->edit_field_scroll = 0;
                } else if(input_event->key == InputKeyDown) {
                    app->edit_field = FIELD_SAVE;
                    app->edit_field_scroll = 0;
                } else if(input_event->key == InputKeyBack) {
                    flipchanger_show_slot_details(app, app->current_slot_index);
                }
            } else if(app->edit_field == FIELD_YEAR) {
                // Year field - numeric only (0-9)
                // Initialize to '0' if not already in number range
                if(app->edit_char_selection < 26 || app->edit_char_selection >= 36) {
                    app->edit_char_selection = 26; // Start at '0'
                }
                
                if(input_event->key == InputKeyUp) {
                    // Navigate to previous field if at start (0), otherwise change number selection
                    if(app->edit_char_selection == 26) {
                        // At start (0) - navigate to previous field
                        app->edit_field = FIELD_ALBUM;
                        app->edit_char_pos = 0;
                        app->edit_char_selection = 0;
                        app->edit_field_scroll = 0;
                    } else {
                        // Change number selection (previous number, 0-9)
                        app->edit_char_selection--;
                    }
                } else if(input_event->key == InputKeyDown) {
                    // Navigate to next field if at start (0), otherwise change number selection
                    if(app->edit_char_selection == 26) {
                        // At start (0) - navigate to next field
                        app->edit_field = FIELD_GENRE;
                        app->edit_char_pos = 0;
                        app->edit_char_selection = 0;
                        app->edit_field_scroll = 0;
                    } else {
                        // Change number selection (next number, 0-9)
                        if(app->edit_char_selection < 35) {
                            app->edit_char_selection++;
                        } else {
                            app->edit_char_selection = 26; // Wrap to 0
                        }
                    }
                } else if(input_event->key == InputKeyOk) {
                    // Add digit to year
                    if(app->edit_char_selection >= 26 && app->edit_char_selection < 36) {
                        // Number selected (0-9)
                        int32_t digit = app->edit_char_selection - 26;
                        slot->cd.year = slot->cd.year * 10 + digit;
                        if(slot->cd.year > 9999) slot->cd.year = 9999;
                        app->dirty = true;
                    }
                } else if(input_event->key == InputKeyBack) {
                    if(is_long_press) {
                        // Long press - exit to slot details
                        flipchanger_show_slot_details(app, app->current_slot_index);
                    } else {
                        // Short press - delete last digit
                        slot->cd.year = slot->cd.year / 10;
                        app->dirty = true;
                    }
                }
            } else {
                // Editing a field (Artist, Album, Genre, Notes)
                // UP/DOWN: Navigate fields if not actively editing, otherwise change character selection
                // BACK exits field editing - allows navigation to other fields
                if(input_event->key == InputKeyUp) {
                    // If at default state (cursor at 0, selection at 0), navigate to previous field
                    // Otherwise, change character selection
                    if(app->edit_char_pos == 0 && app->edit_char_selection == 0) {
                        // Navigate to previous field
                        if(app->edit_field == FIELD_ARTIST) {
                            app->edit_field = FIELD_NOTES;  // Wrap to bottom
                        } else if(app->edit_field == FIELD_ALBUM) {
                            app->edit_field = FIELD_ARTIST;
                        } else if(app->edit_field == FIELD_GENRE) {
                            app->edit_field = FIELD_ALBUM;
                        } else if(app->edit_field == FIELD_NOTES) {
                            app->edit_field = FIELD_GENRE;
                        }
                        app->edit_field_scroll = 0;
                    } else {
                        // Change character selection (previous character, including DEL)
                        int32_t max_selection = CHAR_DEL_INDEX;  // Can select up to DEL
                        if(app->edit_char_selection > 0) {
                            app->edit_char_selection--;
                        } else {
                            app->edit_char_selection = max_selection;  // Wrap to DEL
                        }
                    }
                } else if(input_event->key == InputKeyDown) {
                    // If at default state (cursor at 0, selection at 0), navigate to next field
                    // Otherwise, change character selection
                    if(app->edit_char_pos == 0 && app->edit_char_selection == 0) {
                        // Navigate to next field
                        if(app->edit_field == FIELD_ARTIST) {
                            app->edit_field = FIELD_ALBUM;
                        } else if(app->edit_field == FIELD_ALBUM) {
                            app->edit_field = FIELD_GENRE;
                        } else if(app->edit_field == FIELD_GENRE) {
                            app->edit_field = FIELD_NOTES;
                        } else if(app->edit_field == FIELD_NOTES) {
                            app->edit_field = FIELD_TRACKS;  // Next is Tracks
                        }
                        app->edit_field_scroll = 0;
                    } else {
                        // Change character selection (next character, including DEL)
                        int32_t max_selection = CHAR_DEL_INDEX;  // Can select up to DEL
                        if(app->edit_char_selection < max_selection) {
                            app->edit_char_selection++;
                        } else {
                            app->edit_char_selection = 0;  // Wrap to start
                        }
                    }
                } else if(input_event->key == InputKeyLeft) {
                    // Move cursor left
                    if(app->edit_char_pos > 0) {
                        app->edit_char_pos--;
                    }
                    // Don't reset char_selection - keep current selection
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
                    
                    if(field) {
                        int32_t field_len = strlen(field);
                        // Allow moving cursor right to end of field + 1 (for appending)
                        if(app->edit_char_pos < field_len && app->edit_char_pos < max_len - 1) {
                            app->edit_char_pos++;
                        } else if(app->edit_char_pos == field_len && app->edit_char_pos < max_len - 1) {
                            // Already at end, allow staying at end position
                            app->edit_char_pos = field_len;
                        }
                        // Don't reset char_selection - keep current selection
                    }
                } else if(input_event->key == InputKeyOk) {
                    // Add/insert character or DELETE
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
                        
                        if(field) {
                            // Check if DEL is selected
                            if(app->edit_char_selection >= CHAR_DEL_INDEX) {
                                // DELETE character at cursor
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
                            } else if(app->edit_char_pos >= 0 && app->edit_char_pos < max_len - 1) {
                                // Insert character
                                // Ensure field is null-terminated
                                field[max_len - 1] = '\0';
                                
                                int32_t len = strlen(field);
                                
                                // Ensure cursor position is within bounds
                                if(app->edit_char_pos > len) {
                                    app->edit_char_pos = len;
                                }
                                
                                int32_t char_set_len = strlen(CHAR_SET);
                                if(char_set_len > 0 && app->edit_char_selection < char_set_len) {
                                    char ch = CHAR_SET[app->edit_char_selection];
                                    
                                    // Insert character at cursor position
                                    if(app->edit_char_pos <= len && len < max_len - 1) {
                                        // Shift existing characters
                                        for(int32_t i = len; i >= app->edit_char_pos && i < max_len - 2; i--) {
                                            field[i + 1] = field[i];
                                        }
                                    field[app->edit_char_pos] = ch;
                                    field[len + 1] = '\0';
                                    if(app->edit_char_pos < max_len - 2) {
                                        app->edit_char_pos++;
                                    }
                                    // Trigger scroll update on next draw by ensuring cursor is visible
                                    // The draw function will handle auto-scrolling
                                }
                            }
                        }
                    }
                }
                } else if(input_event->key == InputKeyBack) {
                    // BACK exits field editing - returns to slot details view
                    // Changes are preserved and will be saved when navigating away or on app exit
                    // To delete characters, use DEL option in character selector, then OK
                    flipchanger_show_slot_details(app, app->current_slot_index);
                }
            }
            
            // Only update if app is still running
            if(app->running && app->view_port) {
                view_port_update(app->view_port);
            }
            break;
        }
            
        case VIEW_TRACK_MANAGEMENT: {
            // Safety check - ensure slot index is valid
            if(app->current_slot_index < 0 || app->current_slot_index >= app->total_slots) {
                if(input_event->key == InputKeyBack) {
                    if(is_long_press) {
                        app->current_view = VIEW_SLOT_LIST;
                    } else {
                        app->current_view = VIEW_ADD_EDIT_CD;
                    }
                }
                break;
            }
            
            Slot* slot = flipchanger_get_slot(app, app->current_slot_index);
            if(!slot) {
                if(input_event->key == InputKeyBack) {
                    if(is_long_press) {
                        app->current_view = VIEW_SLOT_LIST;
                    } else {
                        app->current_view = VIEW_ADD_EDIT_CD;
                    }
                }
                break;
            }
            
            // Ensure track_count is valid
            if(slot->cd.track_count < 0) slot->cd.track_count = 0;
            if(slot->cd.track_count > MAX_TRACKS) slot->cd.track_count = MAX_TRACKS;
            
            // Ensure selected track is valid
            if(app->edit_selected_track < 0) app->edit_selected_track = 0;
            if(slot->cd.track_count > 0 && app->edit_selected_track >= slot->cd.track_count) {
                app->edit_selected_track = slot->cd.track_count - 1;
            }
            
            if(app->editing_track) {
                // Editing track title or duration
                Track* track = &slot->cd.tracks[app->edit_selected_track];
                if(!track) {
                    app->editing_track = false;
                    break;
                }
                
                // Ensure edit_track_field is valid
                int32_t track_field_int = (int32_t)app->edit_track_field;
                if(track_field_int < (int32_t)TRACK_FIELD_TITLE || 
                   track_field_int >= (int32_t)TRACK_FIELD_COUNT) {
                    app->edit_track_field = TRACK_FIELD_TITLE;
                }
                
                // Ensure cursor position is valid
                if(app->edit_char_pos < 0) {
                    app->edit_char_pos = 0;
                }
                
                // Ensure char_selection is valid
                if(app->edit_char_selection < 0) {
                    app->edit_char_selection = 0;
                }
                
                char* field = NULL;
                int32_t max_len = 0;
                
                if(app->edit_track_field == TRACK_FIELD_TITLE) {
                    field = track->title;
                    max_len = MAX_TRACK_TITLE_LENGTH;
                } else if(app->edit_track_field == TRACK_FIELD_DURATION) {
                    field = track->duration;
                    max_len = 16; // Max length for duration string
                }
                
                if(!field) {
                    app->editing_track = false;
                    break;
                }
                
                if(input_event->key == InputKeyUp) {
                    // Change character selection (including DEL)
                    int32_t max_selection = CHAR_DEL_INDEX;
                    if(app->edit_char_selection > 0) {
                        app->edit_char_selection--;
                    } else {
                        app->edit_char_selection = max_selection;
                    }
                } else if(input_event->key == InputKeyDown) {
                    // Change character selection (including DEL)
                    int32_t max_selection = CHAR_DEL_INDEX;
                    if(app->edit_char_selection < max_selection) {
                        app->edit_char_selection++;
                    } else {
                        app->edit_char_selection = 0;
                    }
                } else if(input_event->key == InputKeyLeft) {
                    // Move cursor left, or switch to previous field if at start
                    if(app->edit_char_pos > 0) {
                        app->edit_char_pos--;
                    } else {
                        // At start of field - switch to other field
                        if(app->edit_track_field == TRACK_FIELD_DURATION) {
                            app->edit_track_field = TRACK_FIELD_TITLE;
                            app->edit_char_pos = 0;
                            app->edit_char_selection = 0;
                        }
                    }
                } else if(input_event->key == InputKeyRight) {
                    // Move cursor right, or switch to next field if at end
                    int32_t field_len = strlen(field);
                    if(app->edit_char_pos < field_len && app->edit_char_pos < max_len - 1) {
                        app->edit_char_pos++;
                    } else if(app->edit_char_pos == field_len && app->edit_char_pos < max_len - 1) {
                        app->edit_char_pos = field_len;
                    } else {
                        // At end of field - switch to other field
                        if(app->edit_track_field == TRACK_FIELD_TITLE) {
                            app->edit_track_field = TRACK_FIELD_DURATION;
                            app->edit_char_pos = 0;
                            app->edit_char_selection = 26; // Start at '0' for numeric input
                        }
                    }
                } else if(input_event->key == InputKeyOk) {
                    // Add/insert character or DELETE
                    // For duration field, handle numeric input
                    if(app->edit_track_field == TRACK_FIELD_DURATION) {
                        // Duration is numeric only (seconds)
                        if(app->edit_char_selection >= 26 && app->edit_char_selection < 36) {
                            // Number selected (0-9)
                            int32_t digit = app->edit_char_selection - 26;
                            // Convert duration string to number, add digit, convert back
                            int32_t current_seconds = 0;
                            if(strlen(track->duration) > 0) {
                                current_seconds = atoi(track->duration);
                            }
                            current_seconds = current_seconds * 10 + digit;
                            // Limit to reasonable max (99999 seconds = ~27 hours)
                            if(current_seconds > 99999) current_seconds = 99999;
                            snprintf(track->duration, sizeof(track->duration), "%ld", (long)current_seconds);
                            app->dirty = true;
                        }
                    } else if(app->edit_char_selection >= CHAR_DEL_INDEX) {
                        // DELETE character at cursor
                        int32_t len = strlen(field);
                        if(app->edit_char_pos < len && app->edit_char_pos >= 0) {
                            for(int32_t i = app->edit_char_pos; i < len; i++) {
                                field[i] = field[i + 1];
                            }
                        } else if(app->edit_char_pos > 0 && len > 0) {
                            app->edit_char_pos--;
                            for(int32_t i = app->edit_char_pos; i < len; i++) {
                                field[i] = field[i + 1];
                            }
                        }
                        app->dirty = true;
                    } else if(app->edit_track_field == TRACK_FIELD_TITLE && 
                              app->edit_char_pos >= 0 && app->edit_char_pos < max_len - 1) {
                        // Insert character (for title field only - duration is numeric)
                        field[max_len - 1] = '\0';
                        int32_t len = strlen(field);
                        if(app->edit_char_pos > len) {
                            app->edit_char_pos = len;
                        }
                        int32_t char_set_len = strlen(CHAR_SET);
                        if(char_set_len > 0 && app->edit_char_selection < char_set_len) {
                            char ch = CHAR_SET[app->edit_char_selection];
                            if(app->edit_char_pos <= len && len < max_len - 1) {
                                for(int32_t i = len; i >= app->edit_char_pos && i < max_len - 2; i--) {
                                    field[i + 1] = field[i];
                                }
                                field[app->edit_char_pos] = ch;
                                field[len + 1] = '\0';
                                if(app->edit_char_pos < max_len - 2) {
                                    app->edit_char_pos++;
                                }
                            }
                        }
                        app->dirty = true;
                    }
                } else if(input_event->key == InputKeyBack) {
                    if(is_long_press) {
                        // Long press BACK - exit track editing mode
                        app->editing_track = false;
                        app->edit_char_pos = 0;
                        app->edit_char_selection = 0;
                    } else {
                        // Short press BACK behavior:
                        // - If at start of field, switch to other field
                        // - If not at start, delete character/digit
                        if(app->edit_char_pos == 0) {
                            // At start - switch to other field
                            if(app->edit_track_field == TRACK_FIELD_TITLE) {
                                app->edit_track_field = TRACK_FIELD_DURATION;
                                app->edit_char_selection = 26; // Start at '0' for numeric input
                            } else {
                                app->edit_track_field = TRACK_FIELD_TITLE;
                                app->edit_char_selection = 0;
                            }
                            app->edit_char_pos = 0;
                        } else {
                            // Not at start - delete character/digit
                            if(app->edit_track_field == TRACK_FIELD_DURATION) {
                                // Delete last digit
                                int32_t current_seconds = 0;
                                if(strlen(track->duration) > 0) {
                                    current_seconds = atoi(track->duration);
                                }
                                current_seconds = current_seconds / 10;
                                if(current_seconds > 0) {
                                    snprintf(track->duration, sizeof(track->duration), "%ld", (long)current_seconds);
                                } else {
                                    track->duration[0] = '\0';
                                }
                                app->dirty = true;
                            } else {
                                // Delete character in title
                                int32_t len = strlen(field);
                                if(app->edit_char_pos <= len && app->edit_char_pos > 0) {
                                    for(int32_t i = app->edit_char_pos - 1; i < len; i++) {
                                        field[i] = field[i + 1];
                                    }
                                    app->edit_char_pos--;
                                }
                                app->dirty = true;
                            }
                        }
                    }
                }
            } else {
                // Track list navigation
                if(input_event->key == InputKeyUp) {
                    if(app->edit_selected_track > 0) {
                        app->edit_selected_track--;
                    }
                } else if(input_event->key == InputKeyDown) {
                    if(app->edit_selected_track < slot->cd.track_count - 1) {
                        app->edit_selected_track++;
                    }
                } else if(input_event->key == InputKeyOk) {
                    // Edit selected track
                    if(app->edit_selected_track >= 0 && app->edit_selected_track < slot->cd.track_count) {
                        app->editing_track = true;
                        app->edit_track_field = TRACK_FIELD_TITLE;
                        app->edit_char_pos = 0;
                        app->edit_char_selection = 0;
                    }
                } else if(input_event->key == InputKeyRight) {
                    // Add new track
                    if(slot->cd.track_count >= 0 && slot->cd.track_count < MAX_TRACKS) {
                        Track* new_track = &slot->cd.tracks[slot->cd.track_count];
                        if(new_track) {
                            new_track->number = slot->cd.track_count + 1;
                            new_track->title[0] = '\0';
                            new_track->duration[0] = '\0';
                            slot->cd.track_count++;
                            if(slot->cd.track_count > MAX_TRACKS) slot->cd.track_count = MAX_TRACKS;
                            app->edit_selected_track = slot->cd.track_count - 1;
                            if(app->edit_selected_track < 0) app->edit_selected_track = 0;
                            app->dirty = true;
                            if(app->notifications) {
                                notification_message(app->notifications, &sequence_blink_blue_100);
                            }
                        }
                    }
                } else if(input_event->key == InputKeyLeft) {
                    // Delete selected track
                    if(slot->cd.track_count > 0 && app->edit_selected_track >= 0 && app->edit_selected_track < slot->cd.track_count && app->edit_selected_track < MAX_TRACKS) {
                        // Shift tracks down
                        for(int32_t i = app->edit_selected_track; i < slot->cd.track_count - 1 && i < MAX_TRACKS - 1; i++) {
                            if(i + 1 < MAX_TRACKS) {
                                slot->cd.tracks[i] = slot->cd.tracks[i + 1];
                                slot->cd.tracks[i].number = i + 1;
                            }
                        }
                        slot->cd.track_count--;
                        if(slot->cd.track_count < 0) slot->cd.track_count = 0;
                        if(app->edit_selected_track >= slot->cd.track_count && app->edit_selected_track > 0) {
                            app->edit_selected_track--;
                        }
                        if(app->edit_selected_track < 0) app->edit_selected_track = 0;
                        app->dirty = true;
                        if(app->notifications) {
                            notification_message(app->notifications, &sequence_blink_red_100);
                        }
                    }
                } else if(input_event->key == InputKeyBack) {
                    if(is_long_press) {
                        // Long press BACK - exit to slot list
                        app->current_view = VIEW_SLOT_LIST;
                    } else {
                        // Short press BACK - return to Add/Edit view
                        if(app->current_slot_index >= 0 && app->current_slot_index < app->total_slots) {
                            app->current_view = VIEW_ADD_EDIT_CD;
                            app->edit_field = FIELD_TRACKS;  // Stay on tracks field
                        } else {
                            // Invalid slot, go to slot list instead
                            app->current_view = VIEW_SLOT_LIST;
                        }
                    }
                }
            }
            break;
        }
        
        case VIEW_SETTINGS: {
            if(app->editing_slot_count) {
                // Editing slot count - use UP/DOWN to change value
                if(input_event->key == InputKeyUp) {
                    // Increment slot count
                    app->total_slots += 1;
                    if(app->total_slots > MAX_SLOTS) {
                        app->total_slots = MAX_SLOTS;
                    }
                    app->dirty = true;
                } else if(input_event->key == InputKeyDown) {
                    // Decrement slot count
                    app->total_slots -= 1;
                    if(app->total_slots < MIN_SLOTS) {
                        app->total_slots = MIN_SLOTS;
                    }
                    app->dirty = true;
                } else if(input_event->key == InputKeyBack) {
                    if(is_long_press) {
                        // Long press - save and exit
                        if(app->dirty && app->storage) {
                            // Reinitialize slots with new count
                            flipchanger_init_slots(app, app->total_slots);
                            flipchanger_save_data(app);
                            app->dirty = false;
                        }
                        app->editing_slot_count = false;
                        flipchanger_show_main_menu(app);
                    } else {
                        // Short press - exit editing mode (don't save)
                        app->editing_slot_count = false;
                    }
                }
            } else {
                // Settings menu navigation
                if(input_event->key == InputKeyOk) {
                    // Start editing slot count
                    app->editing_slot_count = true;
                    app->edit_slot_count_pos = 0;
                } else if(input_event->key == InputKeyBack) {
                    if(is_long_press) {
                        app->running = false;
                        return;
                    } else {
                        flipchanger_show_main_menu(app);
                    }
                }
            }
            break;
        }
        
        case VIEW_STATISTICS: {
            if(input_event->key == InputKeyBack) {
                if(is_long_press) {
                    app->running = false;
                    return;
                } else {
                    flipchanger_show_main_menu(app);
                }
            }
            break;
        }
            
        default:
            break;
    }
    
    // Only update if app is still running
    if(app->running && app->view_port) {
        view_port_update(app->view_port);
    }
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
    
    // Exit cleanup sequence (must be in exact order to prevent crashes)
    
    // 1. Remove view port from GUI FIRST - this prevents any further callbacks
    if(app->gui && app->view_port) {
        gui_remove_view_port(app->gui, app->view_port);
    }
    
    // 2. Clear callbacks from view port before freeing
    if(app->view_port) {
        view_port_draw_callback_set(app->view_port, NULL, NULL);
        view_port_input_callback_set(app->view_port, NULL, NULL);
    }
    
    // 3. Set running to false after view port is removed (redundant but safe)
    app->running = false;
    
    // 4. Save data NOW (view port removed, but storage/GUI still valid)
    if(app->dirty && app->storage) {
        flipchanger_save_data(app);
    }
    
    // 5. Free view port
    if(app->view_port) {
        view_port_free(app->view_port);
        app->view_port = NULL;
    }
    
    // 6. Close GUI record
    if(app->gui) {
        furi_record_close(RECORD_GUI);
        app->gui = NULL;
    }
    
    // 7. Close notifications
    if(app->notifications) {
        furi_record_close(RECORD_NOTIFICATION);
        app->notifications = NULL;
    }
    
    // 8. Close storage LAST
    if(app->storage) {
        furi_record_close(RECORD_STORAGE);
        app->storage = NULL;
    }
    
    // 9. Free app structure
    free(app);
    
    return 0;
}

// Draw Settings view
void flipchanger_draw_settings(Canvas* canvas, FlipChangerApp* app) {
    if(!canvas || !app) {
        return;
    }
    
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    // Title
    canvas_draw_str(canvas, 30, 10, "Settings");
    
    canvas_set_font(canvas, FontSecondary);
    int32_t y = 22;
    
    // Slot Count setting
    canvas_draw_str(canvas, 5, y, "Slot Count:");
    
    // Display current slot count
    char slot_count_str[32];
    snprintf(slot_count_str, sizeof(slot_count_str), "%ld", (long)app->total_slots);
    
    // Highlight if editing
    if(app->editing_slot_count) {
        canvas_draw_box(canvas, 60, y - 9, 60, 9);
        canvas_invert_color(canvas);
    }
    
    canvas_draw_str(canvas, 65, y, slot_count_str);
    
    if(app->editing_slot_count) {
        canvas_invert_color(canvas);
        
        // Show cursor at end of number
        int32_t cursor_x = 65 + (strlen(slot_count_str) * 6);
        if(cursor_x < 120) {
            canvas_draw_line(canvas, cursor_x, y, cursor_x, y - 8);
        }
        
        // Show number picker hint
        canvas_draw_str(canvas, 5, y + 10, "U/D:Num K:Add");
    }
    
    // Range hint
    y += 12;
    canvas_set_font(canvas, FontKeyboard);
    char range_str[32];
    snprintf(range_str, sizeof(range_str), "Range: %d-%d", MIN_SLOTS, MAX_SLOTS);
    canvas_draw_str(canvas, 5, y, range_str);
    
    // Footer - two lines with abbreviations
    canvas_set_font(canvas, FontKeyboard);
    if(app->editing_slot_count) {
        canvas_draw_str(canvas, 5, 57, "U/D:Num K:Add B:Del");
        canvas_draw_str(canvas, 5, 63, "LB:Save & Exit");
    } else {
        canvas_draw_str(canvas, 5, 57, "K:Edit Slot Count");
        canvas_draw_str(canvas, 5, 63, "B:Return LB:Exit");
    }
}

// Calculate statistics from JSON file (simplified, safe version)
static void flipchanger_calculate_statistics(FlipChangerApp* app, int32_t* total_albums, int32_t* total_tracks, int32_t* total_seconds) {
    *total_albums = 0;
    *total_tracks = 0;
    *total_seconds = 0;
    
    if(!app || !app->storage || !total_albums || !total_tracks || !total_seconds) {
        return;
    }
    
    // Use cached slots only for safety (avoids stack overflow from large JSON parsing)
    // This is more memory-efficient and safer
    for(int32_t i = 0; i < SLOT_CACHE_SIZE && i < app->total_slots; i++) {
        Slot* slot = &app->slots[i];
        if(slot && slot->occupied) {
            (*total_albums)++;
            
            // Count tracks and sum durations
            if(slot->cd.track_count > 0 && slot->cd.track_count <= MAX_TRACKS) {
                for(int32_t t = 0; t < slot->cd.track_count; t++) {
                    Track* track = &slot->cd.tracks[t];
                    if(track) {
                        (*total_tracks)++;
                        
                        // Parse duration (stored as string, e.g., "180" for 180 seconds)
                        if(strlen(track->duration) > 0) {
                            int32_t seconds = atoi(track->duration);
                            if(seconds > 0 && seconds < 999999) {  // Sanity check
                                *total_seconds += seconds;
                            }
                        }
                    }
                }
            }
        }
    }
    
    // If we want to count ALL slots (not just cached), we'd need to load from SD card
    // But that's risky with stack size - for now, count cached slots only
    // This is safe and won't crash, even if it doesn't show all slots
}

// Draw Statistics view
void flipchanger_draw_statistics(Canvas* canvas, FlipChangerApp* app) {
    // Safety check
    if(!canvas || !app) {
        return;
    }
    
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    // Title
    canvas_draw_str(canvas, 20, 10, "Statistics");
    
    // Calculate statistics (safe version - uses cached slots only)
    int32_t total_albums = 0;
    int32_t total_tracks = 0;
    int32_t total_seconds = 0;
    
    // Safety: ensure app is valid before calculating
    if(app && app->storage) {
        flipchanger_calculate_statistics(app, &total_albums, &total_tracks, &total_seconds);
    }
    
    // Format total time (convert seconds to hours:minutes:seconds)
    int32_t hours = total_seconds / 3600;
    int32_t minutes = (total_seconds % 3600) / 60;
    int32_t seconds = total_seconds % 60;
    
    canvas_set_font(canvas, FontSecondary);
    int32_t y = 22;
    
    // Total Albums
    char albums_str[32];
    snprintf(albums_str, sizeof(albums_str), "Albums: %ld", (long)total_albums);
    canvas_draw_str(canvas, 5, y, albums_str);
    y += 10;
    
    // Total Tracks
    char tracks_str[32];
    snprintf(tracks_str, sizeof(tracks_str), "Tracks: %ld", (long)total_tracks);
    canvas_draw_str(canvas, 5, y, tracks_str);
    y += 10;
    
    // Total Time
    char time_str[48];
    if(hours > 0) {
        snprintf(time_str, sizeof(time_str), "Time: %ldh %ldm", (long)hours, (long)minutes);
    } else if(minutes > 0) {
        snprintf(time_str, sizeof(time_str), "Time: %ldm %lds", (long)minutes, (long)seconds);
    } else {
        snprintf(time_str, sizeof(time_str), "Time: %lds", (long)seconds);
    }
    canvas_draw_str(canvas, 5, y, time_str);
    
    // Footer - two lines with abbreviations
    canvas_set_font(canvas, FontKeyboard);
    canvas_draw_str(canvas, 5, 57, "B:Return");
    canvas_draw_str(canvas, 5, 63, "LB:Exit");
}
