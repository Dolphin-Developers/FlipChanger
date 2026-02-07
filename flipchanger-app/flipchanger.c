/**
 * FlipChanger - Main Application File
 *
 * Tracks CD metadata for physical CD changers (3-200 discs).
 * Similar to MP3 ID3 tags: artist, album, year, genre, tracks, notes.
 *
 * Architecture:
 *   - Multi-Changer: Each changer has name, location, slot count; own JSON file
 *   - Cache: Only SLOT_CACHE_SIZE slots in RAM; rest on SD card
 *   - pending_changer_switch: Defer load/save from input callback to main loop (avoids BusFault)
 *   - Views: Main menu, Slot list, Slot details, Add/Edit CD, Track mgmt, Settings, Statistics, Changers
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

/* === JSON helpers (lightweight parser - no external lib) === */
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

// Helper: Write JSON string (escape quotes) - forward decl, defined later
static void write_json_string(File* file, const char* str);

/* Character set for text input (Add/Edit Changer, CD fields). Index 39 = DEL. */
static const char* CHAR_SET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .-,";
#define CHAR_DEL_INDEX ((int32_t)39)

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

// Migrate from legacy single-file to Changer model
static bool flipchanger_migrate_from_legacy(FlipChangerApp* app) {
    if(!app || !app->storage) return false;

    File* f = storage_file_alloc(app->storage);
    if(!storage_file_open(f, FLIPCHANGER_DATA_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(f);
        return false;
    }

    uint8_t buf[2048];
    uint16_t n = storage_file_read(f, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    storage_file_close(f);
    storage_file_free(f);

    int32_t total_slots = DEFAULT_SLOTS;
    const char* p = find_json_key((const char*)buf, "total_slots");
    if(p) {
        read_json_int(p, &total_slots);
        if(total_slots < MIN_SLOTS || total_slots > MAX_SLOTS) total_slots = DEFAULT_SLOTS;
    }

    storage_common_mkdir(app->storage, FLIPCHANGER_APP_DIR);
    char new_path[64];
    snprintf(new_path, sizeof(new_path), "%s/flipchanger_changer_0.json", FLIPCHANGER_APP_DIR);

    File* out = storage_file_alloc(app->storage);
    if(!storage_file_open(out, new_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_free(out);
        return false;
    }
    storage_file_write(out, buf, n);
    storage_file_close(out);
    storage_file_free(out);

    Changer* c = &app->changers[0];
    strncpy(c->id, "changer_0", CHANGER_ID_LEN - 1);
    c->id[CHANGER_ID_LEN - 1] = '\0';
    strncpy(c->name, "Default", CHANGER_NAME_LEN - 1);
    c->name[CHANGER_NAME_LEN - 1] = '\0';
    c->location[0] = '\0';
    c->total_slots = total_slots;
    app->changer_count = 1;
    app->current_changer_index = 0;
    strncpy(app->current_changer_id, "changer_0", CHANGER_ID_LEN - 1);
    app->current_changer_id[CHANGER_ID_LEN - 1] = '\0';

    flipchanger_save_changers(app);
    return true;
}

// Build path to slots file for current Changer (e.g. flipchanger_changer_0.json)
void flipchanger_get_slots_path(const FlipChangerApp* app, char* path_out, size_t path_size) {
    if(!app || !path_out || path_size < 32) {
        if(path_out && path_size > 0) path_out[0] = '\0';
        return;
    }
    if(app->current_changer_id[0] != '\0') {
        snprintf(path_out, path_size, "%s/flipchanger_%s.json", FLIPCHANGER_APP_DIR, app->current_changer_id);
    } else {
        snprintf(path_out, path_size, "%s", FLIPCHANGER_DATA_PATH);
    }
}

// Load changers registry from flipchanger_changers.json
bool flipchanger_load_changers(FlipChangerApp* app) {
    if(!app || !app->storage) {
        return false;
    }

    app->changer_count = 0;
    app->current_changer_index = -1;
    app->current_changer_id[0] = '\0';
    memset(app->changers, 0, sizeof(app->changers));

    File* file = storage_file_alloc(app->storage);
    if(!storage_file_open(file, FLIPCHANGER_CHANGERS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        if(flipchanger_migrate_from_legacy(app)) {
            return true;
        }
        return true;
    }

    uint8_t buf[512];
    uint16_t n = storage_file_read(file, buf, sizeof(buf) - 1);
    buf[n] = '\0';
    storage_file_close(file);
    storage_file_free(file);

    const char* json = (const char*)buf;

    const char* p = find_json_key(json, "last_used_id");
    if(p) {
        char last_id[CHANGER_ID_LEN];
        if(read_json_string(p, last_id, sizeof(last_id))) {
            strncpy(app->current_changer_id, last_id, CHANGER_ID_LEN - 1);
            app->current_changer_id[CHANGER_ID_LEN - 1] = '\0';
        }
    }

    p = find_json_key(json, "changers");
    if(!p) return true;
    p = skip_whitespace(p);
    if(*p != '[') return true;
    p++;

    int32_t i = 0;
    while(*p && i < MAX_CHANGERS) {
        p = skip_whitespace(p);
        if(*p == ']') break;
        if(*p != '{') {
            p++;
            continue;
        }
        p++;

        Changer* c = &app->changers[i];
        memset(c, 0, sizeof(Changer));
        c->total_slots = DEFAULT_SLOTS;

        const char* id_k = find_json_key(p, "id");
        if(id_k) read_json_string(id_k, c->id, CHANGER_ID_LEN);
        const char* name_k = find_json_key(p, "name");
        if(name_k) read_json_string(name_k, c->name, CHANGER_NAME_LEN);
        const char* loc_k = find_json_key(p, "location");
        if(loc_k) read_json_string(loc_k, c->location, CHANGER_LOCATION_LEN);
        const char* slots_k = find_json_key(p, "total_slots");
        if(slots_k) {
            int32_t ts = DEFAULT_SLOTS;
            read_json_int(slots_k, &ts);
            if(ts >= MIN_SLOTS && ts <= MAX_SLOTS) c->total_slots = ts;
        }

        if(c->id[0] != '\0') {
            i++;
            if(strcmp(c->id, app->current_changer_id) == 0) {
                app->current_changer_index = i - 1;
            }
        }

        while(*p && *p != '}' && *p != ']') p++;
        if(*p == '}') p++;
        if(*p == ',') p++;
    }
    app->changer_count = i;

    if(app->changer_count > 0 && app->current_changer_index < 0) {
        app->current_changer_index = 0;
        strncpy(app->current_changer_id, app->changers[0].id, CHANGER_ID_LEN - 1);
        app->current_changer_id[CHANGER_ID_LEN - 1] = '\0';
    }

    return true;
}

// Save changers registry to flipchanger_changers.json
bool flipchanger_save_changers(FlipChangerApp* app) {
    if(!app || !app->storage) {
        return false;
    }

    storage_common_mkdir(app->storage, FLIPCHANGER_APP_DIR);

    File* file = storage_file_alloc(app->storage);
    if(!storage_file_open(file, FLIPCHANGER_CHANGERS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_free(file);
        return false;
    }

    char hdr[96];
    snprintf(hdr, sizeof(hdr), "{\"version\":1,\"last_used_id\":");
    storage_file_write(file, (const uint8_t*)hdr, strlen(hdr));
    write_json_string(file, app->current_changer_id);
    storage_file_write(file, (const uint8_t*)",\"changers\":[", 13);

    for(int32_t i = 0; i < app->changer_count; i++) {
        if(i > 0) storage_file_write(file, (const uint8_t*)",", 1);

        Changer* c = &app->changers[i];
        storage_file_write(file, (const uint8_t*)"{", 1);
        storage_file_write(file, (const uint8_t*)"\"id\":", 5);
        write_json_string(file, c->id);
        storage_file_write(file, (const uint8_t*)",\"name\":", 8);
        write_json_string(file, c->name);
        storage_file_write(file, (const uint8_t*)",\"location\":", 12);
        write_json_string(file, c->location);
        char slots[24];
        snprintf(slots, sizeof(slots), ",\"total_slots\":%ld}", (long)c->total_slots);
        storage_file_write(file, (const uint8_t*)slots, strlen(slots));
    }
    storage_file_write(file, (const uint8_t*)"]}", 2);

    bool ok = storage_file_close(file);
    storage_file_free(file);
    return ok;
}

// Load data from JSON file (uses per-Changer path)
bool flipchanger_load_data(FlipChangerApp* app) {
    if(!app || !app->storage) {
        return false;
    }

    int32_t slots = DEFAULT_SLOTS;
    if(app->current_changer_index >= 0 && app->current_changer_index < app->changer_count) {
        slots = app->changers[app->current_changer_index].total_slots;
    }
    flipchanger_init_slots(app, slots);
    app->total_slots = slots;

    char path[64];
    flipchanger_get_slots_path(app, path, sizeof(path));
    if(path[0] == '\0') return true;

    File* file = storage_file_alloc(app->storage);
    if(!storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        return true;
    }
    
    // Use static buffer to avoid stack overflow in nested callbacks (was 2KB on stack -> BusFault)
    static uint8_t buffer[2048];
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
    
    p = find_json_key(json, "total_slots");
    if(p) {
        int32_t total_slots = DEFAULT_SLOTS;
        p = read_json_int(p, &total_slots);
        if(total_slots >= MIN_SLOTS && total_slots <= MAX_SLOTS) {
            app->total_slots = total_slots;
            if(app->current_changer_index >= 0 && app->current_changer_index < app->changer_count) {
                app->changers[app->current_changer_index].total_slots = total_slots;
            }
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
            const char* artist_key = find_json_key(p, "artist");
            if(artist_key) {
                read_json_string(artist_key, slot->cd.artist, MAX_ARTIST_LENGTH);
            }
            const char* album_artist_key = find_json_key(p, "album_artist");
            if(album_artist_key) {
                read_json_string(album_artist_key, slot->cd.album_artist, MAX_ARTIST_LENGTH);
            }
            const char* album_key = find_json_key(p, "album");
            if(album_key) {
                read_json_string(album_key, slot->cd.album, MAX_ALBUM_LENGTH);
            }
            
            const char* year_key = find_json_key(p, "year");
            if(year_key) {
                read_json_int(year_key, &slot->cd.year);
            }
            const char* disc_key = find_json_key(p, "disc_number");
            if(disc_key) {
                read_json_int(disc_key, &slot->cd.disc_number);
                if(slot->cd.disc_number < 0) slot->cd.disc_number = 0;
            }
            const char* genre_key = find_json_key(p, "genre");
            if(genre_key) {
                read_json_string(genre_key, slot->cd.genre, MAX_GENRE_LENGTH);
            }
            
            // Parse tracks array (full parse - title, duration)
            const char* tracks_key = find_json_key(p, "tracks");
            if(tracks_key) {
                const char* tracks_start = skip_whitespace(tracks_key);
                if(*tracks_start == '[') {
                    tracks_start++;
                    int32_t track_count = 0;
                    const char* track_p = tracks_start;
                    
                    while(*track_p && track_count < MAX_TRACKS) {
                        track_p = skip_whitespace(track_p);
                        if(*track_p == ']') break;
                        if(*track_p == '{') {
                            Track* track = &slot->cd.tracks[track_count];
                            track->number = track_count + 1;
                            track->title[0] = '\0';
                            track->duration[0] = '\0';
                            
                            const char* title_key = find_json_key(track_p, "title");
                            if(title_key) {
                                read_json_string(title_key, track->title, MAX_TRACK_TITLE_LENGTH);
                            }
                            const char* dur_key = find_json_key(track_p, "duration");
                            if(dur_key) {
                                read_json_string(dur_key, track->duration, sizeof(track->duration) - 1);
                            }
                            const char* num_key = find_json_key(track_p, "num");
                            if(num_key) {
                                read_json_int(num_key, &track->number);
                            }
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
    
    storage_common_mkdir(app->storage, FLIPCHANGER_APP_DIR);

    char path[64];
    flipchanger_get_slots_path(app, path, sizeof(path));
    if(path[0] == '\0') {
        storage_file_free(file);
        return false;
    }
    if(!storage_file_open(file, path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
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
            storage_file_write(file, (const uint8_t*)"\"artist\":", 9);
            write_json_string(file, slot->cd.artist);
            storage_file_write(file, (const uint8_t*)",", 1);
            storage_file_write(file, (const uint8_t*)"\"album_artist\":", 15);
            write_json_string(file, slot->cd.album_artist);
            storage_file_write(file, (const uint8_t*)",", 1);
            storage_file_write(file, (const uint8_t*)"\"album\":", 8);
            write_json_string(file, slot->cd.album);
            storage_file_write(file, (const uint8_t*)",", 1);
            
            char year_str[32];
            snprintf(year_str, sizeof(year_str), "\"year\":%ld,\"disc_number\":%ld,", (long)slot->cd.year, (long)slot->cd.disc_number);
            storage_file_write(file, (const uint8_t*)year_str, strlen(year_str));
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

/* === View drawing functions === */
void flipchanger_draw_track_management(Canvas* canvas, FlipChangerApp* app);
void flipchanger_draw_settings(Canvas* canvas, FlipChangerApp* app);
void flipchanger_draw_statistics(Canvas* canvas, FlipChangerApp* app);
void flipchanger_draw_changers(Canvas* canvas, FlipChangerApp* app);
void flipchanger_draw_add_edit_changer(Canvas* canvas, FlipChangerApp* app);
void flipchanger_draw_confirm_delete_changer(Canvas* canvas, FlipChangerApp* app);

// Draw main menu (scrollable - 5 visible at a time)
void flipchanger_draw_main_menu(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    const char* title = "FlipChanger";
    if(app->changer_count > 0 && app->current_changer_index >= 0 && app->current_changer_index < app->changer_count
       && app->changers[app->current_changer_index].name[0] != '\0') {
        title = app->changers[app->current_changer_index].name;
    }
    canvas_draw_str(canvas, 5, 8, title);

    canvas_set_font(canvas, FontSecondary);
    const char* menu_items[] = {
        "View Slots",
        "Add CD",
        "Settings",
        "Statistics",
        "Changers",
        "Help"
    };
    const int32_t main_menu_count = 6;
    const int32_t visible_count = 5;
    int32_t selected = ((app->selected_index % main_menu_count) + main_menu_count) % main_menu_count;

    int32_t start = app->scroll_offset;
    if(start < 0) start = 0;
    if(start + visible_count > main_menu_count) start = main_menu_count - visible_count;
    if(start < 0) start = 0;

    int32_t y = 16;
    for(int32_t i = start; i < start + visible_count && i < main_menu_count; i++) {
        if(i == selected) {
            canvas_draw_box(canvas, 5, y - 8, 118, 10);
            canvas_invert_color(canvas);
        }
        canvas_draw_str(canvas, 10, y, menu_items[i]);
        if(i == selected) {
            canvas_invert_color(canvas);
        }
        y += 10;
    }
}

#define CHANGER_FIELD_NAME    0
#define CHANGER_FIELD_LOCATION 1
#define CHANGER_FIELD_SLOTS   2
#define CHANGER_FIELD_SAVE    3
#define CHANGER_FIELD_DELETE  4  // Only when editing

// Draw Changer select list
void flipchanger_draw_changers(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 8, "Select Changer");

    bool show_add = (app->changer_count < MAX_CHANGERS);
    int32_t total_rows = app->changer_count + (show_add ? 1 : 0);
    if(total_rows == 0) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 5, 28, "+ Add Changer (tap)");
        return;
    }

    canvas_set_font(canvas, FontSecondary);
    int32_t visible = 5;
    int32_t start = app->scroll_offset;
    if(start < 0) start = 0;
    if(start + visible > total_rows) start = total_rows - visible;
    if(start < 0) start = 0;

    int32_t y = 16;
    for(int32_t i = start; i < start + visible && i < total_rows; i++) {
        bool is_add_row = (show_add && i == app->changer_count);
        bool is_selected = (i == app->selected_index);

        if(is_add_row) {
            if(is_selected) {
                canvas_draw_box(canvas, 2, y - 8, 124, 9);
                canvas_invert_color(canvas);
            }
            canvas_draw_str(canvas, 5, y, "+ Add Changer");
            if(is_selected) canvas_invert_color(canvas);
        } else if(i < app->changer_count) {
            Changer* c = &app->changers[i];
            char line[48];
            if(c->location[0] != '\0') {
                snprintf(line, sizeof(line), "%.10s|%.6s|%ld", c->name, c->location, (long)c->total_slots);
            } else {
                snprintf(line, sizeof(line), "%.18s |%ld", c->name, (long)c->total_slots);
            }
            if(is_selected) {
                canvas_draw_box(canvas, 2, y - 8, 124, 9);
                canvas_invert_color(canvas);
            }
            canvas_draw_str(canvas, 5, y, line);
            if(is_selected) canvas_invert_color(canvas);
        }
        y += 10;
    }
}

// Draw Add/Edit Changer form
void flipchanger_draw_add_edit_changer(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 8, app->edit_changer_index >= 0 ? "Edit Changer" : "Add Changer");

    canvas_set_font(canvas, FontSecondary);
    bool has_delete = (app->edit_changer_index >= 0 && app->changer_count > 1);
    const char* labels[] = {"Name:", "Location:", "Slots:", has_delete ? "Delete" : "Save", "Save"};
    int32_t field_count = has_delete ? 5 : 4;
    if(has_delete) labels[4] = "Save";

    int32_t y = 16;
    for(int32_t i = 0; i < field_count; i++) {
        bool sel = (app->edit_changer_field == i);
        if(sel) {
            canvas_draw_box(canvas, 2, y - 8, 124, 9);
            canvas_invert_color(canvas);
        }
        canvas_draw_str(canvas, 5, y, labels[i]);
        if(i == CHANGER_FIELD_NAME) {
            canvas_draw_str(canvas, 45, y, app->edit_changer.name[0] ? app->edit_changer.name : "-");
            if(sel) {
                char cd[8];
                int32_t cs = app->edit_char_selection;
                if(cs >= CHAR_DEL_INDEX) snprintf(cd, sizeof(cd), "[DEL]");
                else snprintf(cd, sizeof(cd), "[%c]", CHAR_SET[cs % (strlen(CHAR_SET) + 1)]);
                canvas_draw_str(canvas, 95, y, cd);
            }
        } else if(i == CHANGER_FIELD_LOCATION) {
            canvas_draw_str(canvas, 55, y, app->edit_changer.location[0] ? app->edit_changer.location : "-");
            if(sel) {
                char cd[8];
                int32_t cs = app->edit_char_selection;
                if(cs >= CHAR_DEL_INDEX) snprintf(cd, sizeof(cd), "[DEL]");
                else snprintf(cd, sizeof(cd), "[%c]", CHAR_SET[cs % (strlen(CHAR_SET) + 1)]);
                canvas_draw_str(canvas, 95, y, cd);
            }
        } else if(i == CHANGER_FIELD_SLOTS) {
            char s[16];
            snprintf(s, sizeof(s), "%ld", (long)app->edit_changer.total_slots);
            canvas_draw_str(canvas, 45, y, s);
        }
        if(sel) canvas_invert_color(canvas);
        y += 10;
    }
}

// Draw confirm delete Changer
void flipchanger_draw_confirm_delete_changer(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 8, "Delete Changer?");
    canvas_set_font(canvas, FontSecondary);
    if(app->edit_changer_index >= 0 && app->edit_changer_index < app->changer_count && app->changer_count > 1) {
        canvas_draw_str(canvas, 5, 24, app->changers[app->edit_changer_index].name);
    }
    canvas_draw_str(canvas, 5, 40, "OK=Yes  Back=No");
}

// Draw slot list
void flipchanger_draw_slot_list(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    
    // Header - compact
    char header[32];
    snprintf(header, sizeof(header), "Slots (%ld total)", app->total_slots);
    canvas_draw_str(canvas, 5, 8, header);
    
    // Full screen: 5 slots visible (was 4 when footer reserved space)
    int32_t visible_count = 5;
    int32_t start_index = app->scroll_offset;
    int32_t end_index = start_index + visible_count;
    if(end_index > app->total_slots) {
        end_index = app->total_slots;
    }
    
    canvas_set_font(canvas, FontSecondary);
    int32_t y = 16;
    
    int32_t items_to_show = (end_index - start_index);
    if(items_to_show > 5) {
        items_to_show = 5;
        end_index = start_index + 5;
    }
    
    for(int32_t i = start_index; i < end_index && (i - start_index) < 5; i++) {
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
        
        y += 10;  // 5 items × 10px = 50px, fits in 16-60
    }
}

// Draw slot details
void flipchanger_draw_slot_details(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    
    if(app->current_slot_index < 0 || app->current_slot_index >= app->total_slots) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "Invalid. Press Back.");
        return;
    }
    
    Slot* slot = flipchanger_get_slot(app, app->current_slot_index);
    
    if(!slot) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "Loading. Press Back.");
        return;
    }
    
    canvas_set_font(canvas, FontPrimary);
    
    // Slot number - compact header
    char slot_str[16];
    snprintf(slot_str, sizeof(slot_str), "Slot %ld", (long)slot->slot_number);
    canvas_draw_str(canvas, 5, 8, slot_str);
    
    if(!slot->occupied) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 5, 28, "[Empty Slot]");
        return;
    }
    
    // CD information - 4 items visible (full screen)
    canvas_set_font(canvas, FontSecondary);
    
    // Build list of fields to display
    typedef struct {
        const char* label;
        char value[64];
        bool visible;
    } DetailField;
    
    DetailField fields[10];
    int32_t field_count = 0;
    
    if(strlen(slot->cd.artist) > 0) {
        fields[field_count].label = "Artist:";
        strncpy(fields[field_count].value, slot->cd.artist, sizeof(fields[field_count].value) - 1);
        fields[field_count].value[sizeof(fields[field_count].value) - 1] = '\0';
        fields[field_count].visible = true;
        field_count++;
    }
    if(strlen(slot->cd.album_artist) > 0) {
        fields[field_count].label = "Album Artist:";
        strncpy(fields[field_count].value, slot->cd.album_artist, sizeof(fields[field_count].value) - 1);
        fields[field_count].value[sizeof(fields[field_count].value) - 1] = '\0';
        fields[field_count].visible = true;
        field_count++;
    }
    if(strlen(slot->cd.album) > 0) {
        fields[field_count].label = "Album:";
        strncpy(fields[field_count].value, slot->cd.album, sizeof(fields[field_count].value) - 1);
        fields[field_count].value[sizeof(fields[field_count].value) - 1] = '\0';
        fields[field_count].visible = true;
        field_count++;
    }
    if(slot->cd.disc_number > 0) {
        fields[field_count].label = "Disc:";
        snprintf(fields[field_count].value, sizeof(fields[field_count].value), "%ld", (long)slot->cd.disc_number);
        fields[field_count].visible = true;
        field_count++;
    }
    if(slot->cd.year > 0) {
        fields[field_count].label = "Year:";
        snprintf(fields[field_count].value, sizeof(fields[field_count].value), "%ld", (long)slot->cd.year);
        fields[field_count].visible = true;
        field_count++;
    }
    if(strlen(slot->cd.genre) > 0) {
        fields[field_count].label = "Genre:";
        strncpy(fields[field_count].value, slot->cd.genre, sizeof(fields[field_count].value) - 1);
        fields[field_count].value[sizeof(fields[field_count].value) - 1] = '\0';
        fields[field_count].visible = true;
        field_count++;
    }
    if(strlen(slot->cd.notes) > 0) {
        fields[field_count].label = "Notes:";
        strncpy(fields[field_count].value, slot->cd.notes, sizeof(fields[field_count].value) - 1);
        fields[field_count].value[sizeof(fields[field_count].value) - 1] = '\0';
        fields[field_count].visible = true;
        field_count++;
    }
    if(slot->cd.track_count > 0) {
        fields[field_count].label = "Tracks:";
        snprintf(fields[field_count].value, sizeof(fields[field_count].value), "%ld", (long)slot->cd.track_count);
        fields[field_count].visible = true;
        field_count++;
    }
    
    // Show 4 items at a time (full screen)
    const int32_t VISIBLE_ITEMS = 4;
    int32_t start_index = app->details_scroll_offset;
    int32_t end_index = start_index + VISIBLE_ITEMS;
    if(end_index > field_count) {
        end_index = field_count;
    }
    
    int32_t y = 18;
    for(int32_t i = start_index; i < end_index; i++) {
        canvas_draw_str(canvas, 5, y, fields[i].label);
        canvas_draw_str(canvas, 35, y, fields[i].value);
        y += 10;
    }
}

// Draw Help view (full screen)
void flipchanger_draw_help(Canvas* canvas, FlipChangerApp* app) {
    UNUSED(app);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 42, 8, "Help");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 5, 18, "U/D:Select  K:OK  B:Back");
    canvas_draw_str(canvas, 5, 27, "LB:Long Back  R:Help");
    canvas_draw_str(canvas, 5, 36, "Slots: wrap U/D");
    canvas_draw_str(canvas, 5, 45, "LPU/LPD: skip 10");
    canvas_draw_str(canvas, 5, 54, "B or K: close");
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
        case VIEW_CHANGERS:
            flipchanger_draw_changers(canvas, app);
            break;
        case VIEW_ADD_EDIT_CHANGER:
            flipchanger_draw_add_edit_changer(canvas, app);
            break;
        case VIEW_CONFIRM_DELETE_CHANGER:
            flipchanger_draw_confirm_delete_changer(canvas, app);
            break;
        case VIEW_SPLASH:
            canvas_clear(canvas);
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str_aligned(canvas, 64, 26, AlignCenter, AlignCenter, "FlipChanger");
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignCenter, "CD Changer Tracker");
            break;
        case VIEW_HELP:
            flipchanger_draw_help(canvas, app);
            break;
        default:
            canvas_clear(canvas);
            canvas_set_font(canvas, FontPrimary);
            canvas_draw_str(canvas, 5, 30, "Error. Press Back.");
            break;
    }
}

// Navigation functions
void flipchanger_show_main_menu(FlipChangerApp* app) {
    app->current_view = VIEW_MAIN_MENU;
    app->selected_index = 0;
    app->scroll_offset = 0;
    if(app->current_changer_index < 0 || app->current_changer_index >= app->changer_count) {
        app->current_changer_index = (app->changer_count > 0) ? 0 : -1;
        if(app->changer_count > 0) {
            strncpy(app->current_changer_id, app->changers[0].id, CHANGER_ID_LEN - 1);
            app->current_changer_id[CHANGER_ID_LEN - 1] = '\0';
        } else {
            app->current_changer_id[0] = '\0';
        }
    }
}

void flipchanger_show_changers(FlipChangerApp* app) {
    app->current_view = VIEW_CHANGERS;
    app->scroll_offset = 0;
    if(app->changer_count > 0) {
        if(app->current_changer_index >= 0 && app->current_changer_index < app->changer_count) {
            app->selected_index = app->current_changer_index;
        } else {
            app->selected_index = 0;
        }
    } else {
        app->selected_index = 0;
    }
}

void flipchanger_show_add_edit_changer(FlipChangerApp* app, int32_t index) {
    app->current_view = VIEW_ADD_EDIT_CHANGER;
    app->edit_changer_index = index;
    app->edit_changer_field = CHANGER_FIELD_NAME;
    app->edit_char_pos = 0;
    app->edit_char_selection = 0;

    if(index >= 0 && index < app->changer_count) {
        memcpy(&app->edit_changer, &app->changers[index], sizeof(Changer));
    } else {
        memset(&app->edit_changer, 0, sizeof(Changer));
        app->edit_changer.total_slots = DEFAULT_SLOTS;
    }
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
        slot->cd.album_artist[0] = '\0';
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
        canvas_draw_str(canvas, 5, 30, "Error. Restart app.");
        return;
    }
    
    if(app->current_slot_index < 0 || app->current_slot_index >= app->total_slots) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "Invalid. Press Back.");
        return;
    }
    
    Slot* slot = flipchanger_get_slot(app, app->current_slot_index);
    if(!slot) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "Loading. Press Back.");
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
    canvas_draw_str(canvas, 5, 8, title);
    
    canvas_set_font(canvas, FontSecondary);
    int32_t y = 16;
    
    const char* field_labels[] = {
        "Artist:",
        "Album Artist:",
        "Album:",
        "Disc #:",
        "Year:",
        "Genre:",
        "Notes:",
        "Tracks:"
    };
    
    char* field_values[] = {
        slot->cd.artist,
        slot->cd.album_artist,
        slot->cd.album,
        NULL,  // Disc # handled separately
        NULL,  // Year handled separately
        slot->cd.genre,
        slot->cd.notes,
        NULL   // Tracks handled separately
    };
    
    // Draw fields (4 visible - full screen)
    const int32_t VISIBLE_FIELDS = 4;
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
        
        if(i == FIELD_DISC_NUMBER) {
            char disc_str[16];
            if(slot->cd.disc_number > 0) {
                snprintf(disc_str, sizeof(disc_str), "%ld", (long)slot->cd.disc_number);
            } else {
                disc_str[0] = '-'; disc_str[1] = '\0';  // - = unset
            }
            int32_t x_pos = 40;
            canvas_draw_str(canvas, x_pos, y, disc_str);
            if(is_selected) {
                int32_t len = strlen(disc_str);
                int32_t cursor_x = x_pos + (len * 6);
                if(cursor_x < 128) {
                    canvas_draw_line(canvas, cursor_x, y, cursor_x, y - 8);
                }
                int32_t char_selection = app->edit_char_selection;
                if(char_selection < 26 || char_selection >= 36) {
                    char_selection = 26;
                }
                if(char_selection >= 26 && char_selection < 36) {
                    int32_t digit = char_selection - 26;
                    char digit_display[4];
                    snprintf(digit_display, sizeof(digit_display), "[%ld]", (long)digit);
                    canvas_draw_str(canvas, 100, y, digit_display);
                }
            }
        } else if(i == FIELD_YEAR) {
            char year_str[32];
            if(slot->cd.year > 0) {
                snprintf(year_str, sizeof(year_str), "%ld", (long)slot->cd.year);
            } else {
                snprintf(year_str, sizeof(year_str), "0");
            }
            int32_t x_pos = 40;
            canvas_draw_str(canvas, x_pos, y, year_str);
            if(is_selected) {
                int32_t year_len = strlen(year_str);
                int32_t cursor_x = x_pos + (year_len * 6);
                if(cursor_x < 128) {
                    canvas_draw_line(canvas, cursor_x, y, cursor_x, y - 8);
                }
                int32_t char_selection = app->edit_char_selection;
                if(char_selection < 26 || char_selection >= 36) {
                    char_selection = 26;
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
            
            if(!value && i != FIELD_YEAR && i != FIELD_DISC_NUMBER && i != FIELD_TRACKS) {
                // Should not happen, but handle gracefully
                continue;
            }
            
            switch(i) {
                case FIELD_ARTIST: max_len = MAX_ARTIST_LENGTH; break;
                case FIELD_ALBUM_ARTIST: max_len = MAX_ARTIST_LENGTH; break;
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
}

// Draw Track Management view
void flipchanger_draw_track_management(Canvas* canvas, FlipChangerApp* app) {
    canvas_clear(canvas);
    
    // Safety checks
    if(!app || app->current_slot_index < 0 || app->current_slot_index >= app->total_slots) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "Invalid. Press Back.");
        return;
    }
    
    Slot* slot = flipchanger_get_slot(app, app->current_slot_index);
    if(!slot) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 5, 30, "Loading. Press Back.");
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
    canvas_draw_str(canvas, 5, 8, title);
    
    canvas_set_font(canvas, FontSecondary);
    
    // Show tracks: 5 when not editing, 4 when editing (to leave room for edit UI)
    int32_t max_visible = app->editing_track ? 4 : 5;
    int32_t y = 18;
    int32_t start_track = 0;
    if(slot->cd.track_count > 0 && app->edit_selected_track >= max_visible) {
        start_track = app->edit_selected_track - max_visible + 1;
    }
    
    for(int32_t i = start_track; i < slot->cd.track_count && i < start_track + max_visible && i >= 0 && i < MAX_TRACKS; i++) {
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
    
    // Show editing interface if editing a track (use bottom area)
    if(app->editing_track && app->edit_selected_track >= 0 && app->edit_selected_track < slot->cd.track_count) {
        Track* track = &slot->cd.tracks[app->edit_selected_track];
        if(track) {
            canvas_set_font(canvas, FontSecondary);
            int32_t edit_y = 56;  // Bottom area for edit UI (when 4 tracks shown)
            
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
}

// Input callback
/* === Input handling - routes to view-specific handlers === */
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

    if(app->current_view == VIEW_SPLASH) {
        flipchanger_show_main_menu(app);
        return;
    }
    
    switch(app->current_view) {
        case VIEW_MAIN_MENU: {
            const int32_t main_menu_count = 6;
            const int32_t visible_count = 5;
            if(input_event->key == InputKeyUp) {
                app->selected_index = (app->selected_index + main_menu_count - 1) % main_menu_count;
                if(app->selected_index < app->scroll_offset) app->scroll_offset = app->selected_index;
            } else if(input_event->key == InputKeyDown) {
                app->selected_index = (app->selected_index + 1) % main_menu_count;
                if(app->selected_index >= app->scroll_offset + visible_count)
                    app->scroll_offset = app->selected_index - visible_count + 1;
            } else if(input_event->key == InputKeyOk) {
                int32_t sel = ((app->selected_index % main_menu_count) + main_menu_count) % main_menu_count;
                switch(sel) {
                    case 0:
                        flipchanger_show_slot_list(app);
                        break;
                    case 1:
                        flipchanger_show_slot_list(app);
                        break;
                    case 2:
                        app->current_view = VIEW_SETTINGS;
                        app->selected_index = 0;
                        app->editing_slot_count = false;
                        app->edit_slot_count_pos = 0;
                        break;
                    case 3:
                        app->current_view = VIEW_STATISTICS;
                        app->selected_index = 0;
                        break;
                    case 4:
                        flipchanger_show_changers(app);
                        break;
                    case 5:
                        app->help_return_view = VIEW_MAIN_MENU;
                        app->current_view = VIEW_HELP;
                        break;
                }
            } else if(input_event->key == InputKeyBack) {
                app->running = false;
                return;
            }
            break;
        }
        case VIEW_CHANGERS: {
            bool show_add = (app->changer_count < MAX_CHANGERS);
            int32_t total_rows = app->changer_count + (show_add ? 1 : 0);
            bool is_add_row = (show_add && app->selected_index == app->changer_count);

            if(input_event->key == InputKeyRight) {
                app->help_return_view = VIEW_CHANGERS;
                app->current_view = VIEW_HELP;
            } else if(input_event->key == InputKeyUp && total_rows > 0) {
                app->selected_index--;
                if(app->selected_index < 0) app->selected_index = total_rows - 1;
                if(app->selected_index >= app->scroll_offset + 5) app->scroll_offset = app->selected_index - 4;
                else if(app->selected_index < app->scroll_offset) app->scroll_offset = app->selected_index;
            } else if(input_event->key == InputKeyDown && total_rows > 0) {
                app->selected_index++;
                if(app->selected_index >= total_rows) app->selected_index = 0;
                if(app->selected_index >= app->scroll_offset + 5) app->scroll_offset = app->selected_index - 4;
                else if(app->selected_index < app->scroll_offset) app->scroll_offset = app->selected_index;
            } else if(input_event->key == InputKeyOk) {
                if(is_add_row) {
                    flipchanger_show_add_edit_changer(app, -1);
                } else if(is_long_press && app->selected_index < app->changer_count) {
                    flipchanger_show_add_edit_changer(app, app->selected_index);
                } else if(app->selected_index >= 0 && app->selected_index < app->changer_count) {
                    app->current_changer_index = app->selected_index;
                    strncpy(app->current_changer_id, app->changers[app->selected_index].id, CHANGER_ID_LEN - 1);
                    app->current_changer_id[CHANGER_ID_LEN - 1] = '\0';
                    app->total_slots = app->changers[app->selected_index].total_slots;
                    app->pending_changer_switch = true;
                    app->scroll_offset = 0;
                    flipchanger_show_main_menu(app);
                }
            } else if(input_event->key == InputKeyBack) {
                app->scroll_offset = 0;
                flipchanger_show_main_menu(app);
            }
            break;
        }
        case VIEW_ADD_EDIT_CHANGER: {
            bool has_delete = (app->edit_changer_index >= 0 && app->changer_count > 1);
            int32_t max_field = has_delete ? (int32_t)CHANGER_FIELD_DELETE : (int32_t)CHANGER_FIELD_SAVE;
            int32_t char_set_len = strlen(CHAR_SET);
            int32_t max_sel = char_set_len;

            if(input_event->key == InputKeyBack) {
                flipchanger_show_changers(app);
            } else if(app->edit_changer_field == CHANGER_FIELD_SAVE) {
                if(input_event->key == InputKeyOk && app->edit_changer.name[0] != '\0') {
                    if(app->edit_changer_index >= 0) {
                        memcpy(&app->changers[app->edit_changer_index], &app->edit_changer, sizeof(Changer));
                        if(app->current_changer_index == app->edit_changer_index) {
                            app->total_slots = app->edit_changer.total_slots;
                        }
                    } else {
                        char id[CHANGER_ID_LEN];
                        snprintf(id, sizeof(id), "changer_%ld", (long)app->changer_count);
                        strncpy(app->edit_changer.id, id, CHANGER_ID_LEN - 1);
                        app->edit_changer.id[CHANGER_ID_LEN - 1] = '\0';
                        if(app->edit_changer.total_slots < MIN_SLOTS) app->edit_changer.total_slots = MIN_SLOTS;
                        if(app->edit_changer.total_slots > MAX_SLOTS) app->edit_changer.total_slots = MAX_SLOTS;
                        memcpy(&app->changers[app->changer_count], &app->edit_changer, sizeof(Changer));
                        app->changer_count++;
                        char new_path[64];
                        snprintf(new_path, sizeof(new_path), "%s/flipchanger_%s.json", FLIPCHANGER_APP_DIR, app->edit_changer.id);
                        File* nf = storage_file_alloc(app->storage);
                        if(storage_file_open(nf, new_path, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
                            char init[80];
                            snprintf(init, sizeof(init), "{\"version\":1,\"total_slots\":%ld,\"slots\":[]}", (long)app->edit_changer.total_slots);
                            storage_file_write(nf, (const uint8_t*)init, strlen(init));
                            storage_file_close(nf);
                        }
                        storage_file_free(nf);
                    }
                    flipchanger_save_changers(app);
                    flipchanger_show_changers(app);
                } else if(input_event->key == InputKeyUp) {
                    app->edit_changer_field = (has_delete ? CHANGER_FIELD_DELETE : CHANGER_FIELD_SLOTS);
                } else if(input_event->key == InputKeyDown) {
                    app->edit_changer_field = CHANGER_FIELD_NAME;
                }
            } else if(app->edit_changer_field == CHANGER_FIELD_DELETE) {
                if(input_event->key == InputKeyOk) {
                    app->current_view = VIEW_CONFIRM_DELETE_CHANGER;
                } else if(input_event->key == InputKeyUp) {
                    app->edit_changer_field = CHANGER_FIELD_SLOTS;
                } else if(input_event->key == InputKeyDown) {
                    app->edit_changer_field = CHANGER_FIELD_SAVE;
                }
            } else if(app->edit_changer_field == CHANGER_FIELD_SLOTS) {
                if(input_event->key == InputKeyUp) {
                    app->edit_changer.total_slots++;
                    if(app->edit_changer.total_slots > MAX_SLOTS) app->edit_changer.total_slots = MAX_SLOTS;
                } else if(input_event->key == InputKeyDown) {
                    app->edit_changer.total_slots--;
                    if(app->edit_changer.total_slots < MIN_SLOTS) app->edit_changer.total_slots = MIN_SLOTS;
                } else if(input_event->key == InputKeyOk) {
                    app->edit_changer_field = (has_delete ? CHANGER_FIELD_DELETE : CHANGER_FIELD_SAVE);
                }
            } else if(app->edit_changer_field == CHANGER_FIELD_NAME || app->edit_changer_field == CHANGER_FIELD_LOCATION) {
                char* field = (app->edit_changer_field == CHANGER_FIELD_NAME) ? app->edit_changer.name : app->edit_changer.location;
                int32_t max_len = (app->edit_changer_field == CHANGER_FIELD_NAME) ? CHANGER_NAME_LEN - 1 : CHANGER_LOCATION_LEN - 1;

                if(input_event->key == InputKeyUp) {
                    app->edit_char_selection--;
                    if(app->edit_char_selection < 0) app->edit_char_selection = max_sel;
                } else if(input_event->key == InputKeyDown) {
                    app->edit_char_selection++;
                    if(app->edit_char_selection > max_sel) app->edit_char_selection = 0;
                } else if(input_event->key == InputKeyLeft) {
                    if(app->edit_char_pos > 0) {
                        app->edit_char_pos--;
                    } else if(app->edit_changer_field == CHANGER_FIELD_LOCATION) {
                        app->edit_changer_field = CHANGER_FIELD_NAME;
                        app->edit_char_pos = strlen(app->edit_changer.name);
                        app->edit_char_selection = 0;
                    } else {
                        app->edit_changer_field = max_field;
                    }
                } else if(input_event->key == InputKeyRight) {
                    int32_t flen = strlen(field);
                    if(app->edit_char_pos < flen && app->edit_char_pos < max_len - 1) {
                        app->edit_char_pos++;
                    } else if(app->edit_char_pos == flen) {
                        if(app->edit_changer_field == CHANGER_FIELD_NAME) {
                            app->edit_changer_field = CHANGER_FIELD_LOCATION;
                            app->edit_char_pos = 0;
                            app->edit_char_selection = 0;
                        } else {
                            app->edit_changer_field = CHANGER_FIELD_SLOTS;
                        }
                    }
                } else if(input_event->key == InputKeyOk) {
                    if(app->edit_char_selection >= CHAR_DEL_INDEX) {
                        int32_t len = strlen(field);
                        if(app->edit_char_pos < len) {
                            memmove(field + app->edit_char_pos, field + app->edit_char_pos + 1, len - app->edit_char_pos);
                        } else if(app->edit_char_pos > 0) {
                            app->edit_char_pos--;
                            memmove(field + app->edit_char_pos, field + app->edit_char_pos + 1, strlen(field) - app->edit_char_pos);
                        }
                    } else if(app->edit_char_selection < char_set_len && app->edit_char_pos < max_len - 1) {
                        int32_t len = strlen(field);
                        if(len < max_len) {
                            memmove(field + app->edit_char_pos + 1, field + app->edit_char_pos, len - app->edit_char_pos + 1);
                            field[app->edit_char_pos] = CHAR_SET[app->edit_char_selection];
                            if(app->edit_char_pos <= len) app->edit_char_pos++;
                        }
                    }
                }
            }
            break;
        }
        case VIEW_CONFIRM_DELETE_CHANGER:
            if(input_event->key == InputKeyOk && app->edit_changer_index >= 0 && app->changer_count > 1) {
                for(int32_t i = app->edit_changer_index; i < app->changer_count - 1; i++) {
                    memcpy(&app->changers[i], &app->changers[i + 1], sizeof(Changer));
                }
                app->changer_count--;
                if(app->current_changer_index >= app->changer_count) app->current_changer_index = app->changer_count - 1;
                if(app->current_changer_index >= 0) {
                    strncpy(app->current_changer_id, app->changers[app->current_changer_index].id, CHANGER_ID_LEN - 1);
                    app->current_changer_id[CHANGER_ID_LEN - 1] = '\0';
                    app->total_slots = app->changers[app->current_changer_index].total_slots;
                }
                flipchanger_save_changers(app);
                flipchanger_load_data(app);
                flipchanger_show_changers(app);
            } else if(input_event->key == InputKeyBack) {
                app->current_view = VIEW_ADD_EDIT_CHANGER;
            }
            break;
        case VIEW_HELP:
            if(input_event->key == InputKeyBack || input_event->key == InputKeyOk) {
                app->current_view = app->help_return_view;
            }
            break;
            
        case VIEW_SLOT_LIST:
            if(input_event->key == InputKeyRight) {
                app->help_return_view = VIEW_SLOT_LIST;
                app->current_view = VIEW_HELP;
            } else if(input_event->key == InputKeyUp) {
                if(is_long_press) {
                    // Long press Up: skip back by 10
                    app->selected_index -= 10;
                    if(app->selected_index < 0) {
                        app->selected_index = app->total_slots - 1;  // Wrap to last
                    }
                } else {
                    // Wrap: at slot 1, Up goes to last slot
                    if(app->selected_index <= 0) {
                        app->selected_index = app->total_slots - 1;
                    } else {
                        app->selected_index--;
                    }
                }
                // Keep selected slot visible (scroll when needed, including wrap)
                if(app->selected_index < app->scroll_offset) {
                    app->scroll_offset = app->selected_index;
                } else if(app->selected_index >= app->scroll_offset + 5) {
                    app->scroll_offset = app->selected_index - 4;
                }
            } else if(input_event->key == InputKeyDown) {
                if(is_long_press) {
                    // Long press Down: skip forward by 10
                    app->selected_index += 10;
                    if(app->selected_index >= app->total_slots) {
                        app->selected_index = 0;  // Wrap to first
                    }
                } else {
                    // Wrap: at last slot, Down goes to first
                    if(app->selected_index >= app->total_slots - 1) {
                        app->selected_index = 0;
                    } else {
                        app->selected_index++;
                    }
                }
                // Keep selected slot visible (scroll when needed, including wrap)
                if(app->selected_index >= app->scroll_offset + 5) {
                    app->scroll_offset = app->selected_index - 4;
                } else if(app->selected_index < app->scroll_offset) {
                    app->scroll_offset = app->selected_index;
                }
            } else if(input_event->key == InputKeyOk) {
                flipchanger_update_cache(app, app->selected_index);
                flipchanger_show_slot_details(app, app->selected_index);
            } else if(input_event->key == InputKeyBack) {
                flipchanger_show_main_menu(app);
            }
            break;
            
        case VIEW_SLOT_DETAILS: {
            Slot* slot = flipchanger_get_slot(app, app->current_slot_index);
            if(input_event->key == InputKeyRight) {
                app->help_return_view = VIEW_SLOT_DETAILS;
                app->current_view = VIEW_HELP;
            } else if(input_event->key == InputKeyOk) {
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
            } else if(app->edit_field == FIELD_DISC_NUMBER) {
                if(app->edit_char_selection < 26 || app->edit_char_selection >= 36) {
                    app->edit_char_selection = 26;
                }
                if(input_event->key == InputKeyUp) {
                    if(app->edit_char_selection == 26) {
                        app->edit_field = FIELD_ALBUM;
                        app->edit_char_pos = 0;
                        app->edit_char_selection = 0;
                        app->edit_field_scroll = 0;
                    } else {
                        app->edit_char_selection--;
                    }
                } else if(input_event->key == InputKeyDown) {
                    if(app->edit_char_selection == 26) {
                        app->edit_field = FIELD_YEAR;
                        app->edit_char_pos = 0;
                        app->edit_char_selection = 26;
                        app->edit_field_scroll = 0;
                    } else {
                        if(app->edit_char_selection < 35) {
                            app->edit_char_selection++;
                        } else {
                            app->edit_char_selection = 26;
                        }
                    }
                } else if(input_event->key == InputKeyOk) {
                    if(app->edit_char_selection >= 26 && app->edit_char_selection < 36) {
                        int32_t digit = app->edit_char_selection - 26;
                        slot->cd.disc_number = slot->cd.disc_number * 10 + digit;
                        if(slot->cd.disc_number > 999) slot->cd.disc_number = 999;
                        app->dirty = true;
                    }
                } else if(input_event->key == InputKeyBack) {
                    if(is_long_press) {
                        flipchanger_show_slot_details(app, app->current_slot_index);
                    } else {
                        slot->cd.disc_number = slot->cd.disc_number / 10;
                        app->dirty = true;
                    }
                }
            } else if(app->edit_field == FIELD_YEAR) {
                if(app->edit_char_selection < 26 || app->edit_char_selection >= 36) {
                    app->edit_char_selection = 26;
                }
                if(input_event->key == InputKeyUp) {
                    if(app->edit_char_selection == 26) {
                        app->edit_field = FIELD_DISC_NUMBER;
                        app->edit_char_pos = 0;
                        app->edit_char_selection = 26;
                        app->edit_field_scroll = 0;
                    } else {
                        app->edit_char_selection--;
                    }
                } else if(input_event->key == InputKeyDown) {
                    if(app->edit_char_selection == 26) {
                        app->edit_field = FIELD_GENRE;
                        app->edit_char_pos = 0;
                        app->edit_char_selection = 0;
                        app->edit_field_scroll = 0;
                    } else {
                        if(app->edit_char_selection < 35) {
                            app->edit_char_selection++;
                        } else {
                            app->edit_char_selection = 26;
                        }
                    }
                } else if(input_event->key == InputKeyOk) {
                    if(app->edit_char_selection >= 26 && app->edit_char_selection < 36) {
                        int32_t digit = app->edit_char_selection - 26;
                        slot->cd.year = slot->cd.year * 10 + digit;
                        if(slot->cd.year > 9999) slot->cd.year = 9999;
                        app->dirty = true;
                    }
                } else if(input_event->key == InputKeyBack) {
                    if(is_long_press) {
                        flipchanger_show_slot_details(app, app->current_slot_index);
                    } else {
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
                        if(app->edit_field == FIELD_ARTIST) {
                            app->edit_field = FIELD_NOTES;
                        } else if(app->edit_field == FIELD_ALBUM_ARTIST) {
                            app->edit_field = FIELD_ARTIST;
                        } else if(app->edit_field == FIELD_ALBUM) {
                            app->edit_field = FIELD_ALBUM_ARTIST;
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
                        if(app->edit_field == FIELD_ARTIST) {
                            app->edit_field = FIELD_ALBUM_ARTIST;
                        } else if(app->edit_field == FIELD_ALBUM_ARTIST) {
                            app->edit_field = FIELD_ALBUM;
                        } else if(app->edit_field == FIELD_ALBUM) {
                            app->edit_field = FIELD_DISC_NUMBER;
                        } else if(app->edit_field == FIELD_GENRE) {
                            app->edit_field = FIELD_NOTES;
                        } else if(app->edit_field == FIELD_NOTES) {
                            app->edit_field = FIELD_TRACKS;
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
                        case FIELD_ALBUM_ARTIST:
                            field = slot->cd.album_artist;
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
                            case FIELD_ALBUM_ARTIST:
                                field = slot->cd.album_artist;
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
                    app->total_slots += 1;
                    if(app->total_slots > MAX_SLOTS) app->total_slots = MAX_SLOTS;
                    if(app->current_changer_index >= 0 && app->current_changer_index < app->changer_count) {
                        app->changers[app->current_changer_index].total_slots = app->total_slots;
                    }
                    app->dirty = true;
                } else if(input_event->key == InputKeyDown) {
                    app->total_slots -= 1;
                    if(app->total_slots < MIN_SLOTS) app->total_slots = MIN_SLOTS;
                    if(app->current_changer_index >= 0 && app->current_changer_index < app->changer_count) {
                        app->changers[app->current_changer_index].total_slots = app->total_slots;
                    }
                    app->dirty = true;
                } else if(input_event->key == InputKeyBack) {
                    if(is_long_press) {
                        if(app->dirty && app->storage) {
                            flipchanger_init_slots(app, app->total_slots);
                            flipchanger_save_data(app);
                            flipchanger_save_changers(app);
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
                if(input_event->key == InputKeyRight) {
                    app->help_return_view = VIEW_SETTINGS;
                    app->current_view = VIEW_HELP;
                } else if(input_event->key == InputKeyOk) {
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
            if(input_event->key == InputKeyRight) {
                app->help_return_view = VIEW_STATISTICS;
                app->current_view = VIEW_HELP;
            } else if(input_event->key == InputKeyBack) {
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
    
    app->current_view = VIEW_SPLASH;
    app->splash_start_tick = furi_get_tick();
    
    flipchanger_load_changers(app);
    if(app->changer_count == 0) {
        Changer* c = &app->changers[0];
        strncpy(c->id, "changer_0", CHANGER_ID_LEN - 1);
        strncpy(c->name, "Default", CHANGER_NAME_LEN - 1);
        c->location[0] = '\0';
        c->total_slots = DEFAULT_SLOTS;
        app->changer_count = 1;
        app->current_changer_index = 0;
        strncpy(app->current_changer_id, "changer_0", CHANGER_ID_LEN - 1);
        app->current_changer_id[CHANGER_ID_LEN - 1] = '\0';
        flipchanger_save_changers(app);
    }
    
    flipchanger_load_data(app);
    notification_message(app->notifications, &sequence_blink_green_100);
    view_port_update(app->view_port);
    
    while(app->running) {
        if(app->current_view == VIEW_SPLASH) {
            if(furi_get_tick() - app->splash_start_tick >= 1200) {
                flipchanger_show_main_menu(app);
                view_port_update(app->view_port);
            }
        } else if(app->pending_changer_switch) {
            app->pending_changer_switch = false;
            flipchanger_load_data(app);
            flipchanger_save_changers(app);
            view_port_update(app->view_port);
        }
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
    if(app->storage) {
        flipchanger_save_changers(app);
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
    
    canvas_draw_str(canvas, 35, 8, "Settings");
    
    canvas_set_font(canvas, FontSecondary);
    int32_t y = 20;
    
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
    
    // Range hint - use remaining space
    y += 14;
    canvas_set_font(canvas, FontKeyboard);
    char range_str[32];
    snprintf(range_str, sizeof(range_str), "Range: %d-%d", MIN_SLOTS, MAX_SLOTS);
    canvas_draw_str(canvas, 5, y, range_str);
}

/**
 * Calculate stats from cached slots only (avoids full JSON parse - stack safety).
 * For 200-slot changer, full parse risks overflow; cache gives partial but safe stats.
 */
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
    
    canvas_draw_str(canvas, 30, 8, "Statistics");
    
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
    int32_t y = 20;
    
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
}
