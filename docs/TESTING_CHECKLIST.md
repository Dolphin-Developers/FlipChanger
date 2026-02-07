# FlipChanger Testing Checklist

**Date**: February 8, 2025  
**Version**: v1.1.0 (Multi-Changer, Splash)  
**Tester**: _______________

---

## v1.1.0 Changes (Feb 2025)
- **Multi-Changer** – Changers (Name, Location, Slots); select at top menu; Add/Edit/Delete
- **Splash screen** – Brief FlipChanger logo on launch (~1.2s or key to skip)
- **Main menu** – 6 items (View Slots, Add CD, Settings, Statistics, Changers, Help); scrollable (5 visible)
- **Header** – Shows current Changer name instead of "FlipChanger"
- **Per-Changer data** – Each Changer has own slots file; last-used persisted

---

## Instructions

For each test item, respond with:
- ✅ **PASS** - Works as expected
- ❌ **FAIL** - Doesn't work or crashes
- ⚠️ **PARTIAL** - Works but has issues
- ➖ **SKIP** - Not tested

If you mark FAIL or PARTIAL, please describe the issue in the notes section.

---

## 1. App Launch & Basic Navigation

### 1.1 App Launch
- [ ] Splash screen appears (FlipChanger, CD Changer Tracker)
- [ ] Splash dismisses after ~1.2s or on key press
- [ ] Main menu appears with Changer name in header
- [ ] 6 menu items (View Slots, Add CD, Settings, Statistics, Changers, Help)
- [ ] Main menu scrolls (5 visible at a time; Up/Down to scroll)
- [ ] No UI overlap (footers removed; use Help for instructions)
- [ ] No overlap between menu items

**Notes**: _________________________________________________

### 1.2 Main Menu Navigation
- [ ] UP/DOWN navigates between menu items
- [ ] Selected item is highlighted
- [ ] OK button selects menu item
- [ ] BACK button exits app
- [ ] Long press BACK exits app

**Notes**: _________________________________________________

### 1.3 Changers (v1.1.0)
- [ ] Changers menu item visible and selectable
- [ ] Changer list shows Name | Location | slots
- [ ] "+ Add Changer" row appears when < 10 changers
- [ ] OK on Changer selects/switches (returns to main menu)
- [ ] Long-press OK on Changer opens Edit form
- [ ] OK on "+ Add Changer" opens Add form
- [ ] BACK returns to main menu (no crash)
- [ ] Header shows Changer name after switch

**Notes**: _________________________________________________

---

## 2. View Slots

### 2.1 Slot List Display
- [ ] Slot list view appears
- [ ] Shows correct number of slots (default or configured)
- [ ] Empty slots show "Empty" status
- [ ] Occupied slots show album name
- [ ] No UI overlap (press R for Help)
- [ ] Shows 5 items at a time (scrollable)
- [ ] Wrap-around: Down at last slot → first; Up at first → last
- [ ] Screen scrolls when wrapping so selection stays visible
- [ ] Long press Up/Down skips by 10 slots

**Notes**: _________________________________________________

### 2.2 Slot List Navigation
- [ ] UP/DOWN scrolls through slots
- [ ] Selected slot is highlighted
- [ ] OK button opens slot details
- [ ] BACK button returns to main menu
- [ ] Long press BACK exits app

**Notes**: _________________________________________________

---

## 3. Slot Details View

### 3.1 Empty Slot Display
- [ ] Empty slot shows "[Empty Slot]" message
- [ ] OK button opens Add/Edit view
- [ ] BACK button returns to slot list
- [ ] R opens Help

**Notes**: _________________________________________________

### 3.2 Occupied Slot Display
- [ ] Shows slot number in header
- [ ] Shows all fields: Artist, Album, Year, Genre, Tracks
- [ ] Fields are scrollable (shows 4 at a time)
- [ ] UP/DOWN scrolls through fields
- [ ] Footer shows "U/D:Scroll K:Edit B:Return"
- [ ] OK button opens Add/Edit view
- [ ] BACK button returns to slot list
- [ ] No UI overlap

**Notes**: _________________________________________________

---

## 4. Add/Edit CD View

### 4.1 View Display
- [ ] Shows slot number in header
- [ ] Shows all fields: Artist, Album, Year, Genre, Notes, Tracks, Save
- [ ] Shows 4 fields at a time (scrollable)
- [ ] Selected field is highlighted
- [ ] Footer text visible and readable (two lines)
- [ ] Footer doesn't overlap menu items
- [ ] Footer text wraps properly (doesn't run off screen)

**Notes**: _________________________________________________

### 4.2 Field Navigation
- [ ] UP/DOWN navigates between fields when at start (cursor pos 0, char selection 0)
- [ ] UP/DOWN changes character selection when editing
- [ ] Can navigate to all fields: Artist, Album, Year, Genre, Notes, Tracks, Save
- [ ] Navigation wraps around (from Save to Artist, etc.)

**Notes**: _________________________________________________

### 4.3 Text Field Editing (Artist, Album, Genre, Notes)
- [ ] Can select characters with UP/DOWN
- [ ] Character picker shows selected character (e.g., "[A]")
- [ ] DEL option appears in character picker
- [ ] OK button adds selected character
- [ ] OK with DEL selected deletes character
- [ ] LEFT/RIGHT moves cursor
- [ ] Text scrolls horizontally when longer than visible area
- [ ] Cursor stays visible when scrolling
- [ ] Can input all characters including 'A'
- [ ] BACK button exits field editing (returns to slot details)
- [ ] Long press BACK exits to slot list

**Notes**: _________________________________________________

### 4.4 Year Field Editing
- [ ] Year field accepts numbers only (0-9)
- [ ] UP/DOWN at start (0) navigates to previous/next field
- [ ] UP/DOWN when editing changes number selection (0-9)
- [ ] OK button adds selected digit
- [ ] BACK button deletes last digit
- [ ] Long press BACK exits to slot details
- [ ] Year value displays correctly

**Notes**: _________________________________________________

### 4.5 Tracks Field
- [ ] Tracks field shows track count
- [ ] OK button opens Track Management view
- [ ] Can navigate to/from Tracks field

**Notes**: _________________________________________________

### 4.6 Save Button
- [ ] Save button is accessible
- [ ] OK on Save button saves the CD
- [ ] Returns to slot details after save
- [ ] Saved data persists (visible after restart)

**Notes**: _________________________________________________

---

## 5. Track Management View

### 5.1 Track List Display
- [ ] Shows track list with numbers and titles
- [ ] Shows durations for tracks
- [ ] Selected track is highlighted
- [ ] Footer text visible and readable
- [ ] No UI overlap

**Notes**: _________________________________________________

### 5.2 Track List Navigation
- [ ] UP/DOWN navigates through tracks
- [ ] OK button starts editing selected track
- [ ] RIGHT button (+) adds new track
- [ ] LEFT button (-) deletes selected track
- [ ] BACK button returns to Add/Edit view
- [ ] Long press BACK exits to slot list

**Notes**: _________________________________________________

### 5.3 Track Title Editing
- [ ] Title editing mode appears
- [ ] Shows "Title:" label
- [ ] Can select characters with UP/DOWN
- [ ] Character picker shows selected character
- [ ] DEL option available
- [ ] OK button adds selected character
- [ ] OK with DEL deletes character
- [ ] LEFT/RIGHT moves cursor
- [ ] Text scrolls horizontally
- [ ] Can input 'A' character (doesn't switch fields)
- [ ] BACK button switches to Duration field (when at start) or exits editing
- [ ] Long press BACK exits track editing

**Notes**: _________________________________________________

### 5.4 Track Duration Editing
- [ ] Duration editing mode appears
- [ ] Shows "Duration:" label
- [ ] Accepts numbers only (0-9)
- [ ] UP/DOWN changes number selection
- [ ] OK button adds selected digit
- [ ] BACK button deletes last digit or switches to Title field
- [ ] Duration stored as seconds (numeric)
- [ ] Can switch between Title and Duration with LEFT/RIGHT at field boundaries

**Notes**: _________________________________________________

---

## 6. Statistics View

### 6.1 Statistics Display
- [ ] Statistics view opens without crash
- [ ] Shows "Statistics" title
- [ ] Displays "Albums: X" (count of occupied slots)
- [ ] Displays "Tracks: X" (total track count)
- [ ] Displays "Time: Xh Xm Xs" or "Time: Xm Xs" or "Time: Xs" (total duration)
- [ ] Footer text visible
- [ ] No crashes or errors

**Notes**: _________________________________________________

### 6.2 Statistics Accuracy
- [ ] Album count matches number of occupied slots
- [ ] Track count matches sum of all tracks
- [ ] Time calculation appears correct (sum of all durations)
- [ ] Statistics update when CDs are added/removed

**Notes**: _________________________________________________

### 6.3 Statistics Navigation
- [ ] BACK button returns to main menu
- [ ] Long press BACK exits app

**Notes**: _________________________________________________

---

## 7. Settings View

### 7.1 Settings Display
- [ ] Settings view opens without crash
- [ ] Shows "Settings" title
- [ ] Displays "Slot Count: X" (current value)
- [ ] Shows "Range: 3-200" hint
- [ ] Footer text visible and readable
- [ ] No crashes or errors

**Notes**: _________________________________________________

### 7.2 Slot Count Editing
- [ ] OK button starts editing slot count
- [ ] Slot count field highlights when editing
- [ ] UP button increments slot count
- [ ] DOWN button decrements slot count
- [ ] Value stays within range (3-200)
- [ ] Value wraps/clamps correctly at boundaries
- [ ] Cursor appears at end of number
- [ ] Footer shows editing instructions

**Notes**: _________________________________________________

### 7.3 Settings Save
- [ ] Long press BACK saves new slot count
- [ ] Returns to main menu after save
- [ ] New slot count persists after app restart
- [ ] Slot list reflects new slot count
- [ ] Short press BACK cancels editing (doesn't save)

**Notes**: _________________________________________________

### 7.4 Settings Navigation
- [ ] BACK button returns to main menu (when not editing)
- [ ] Long press BACK exits app (when not editing)

**Notes**: _________________________________________________

---

## 8. Data Persistence

### 8.1 Save Functionality
- [ ] CD data saves when Save button pressed
- [ ] Track data saves with CD
- [ ] Settings (slot count) saves when changed
- [ ] No data loss on app exit

**Notes**: _________________________________________________

### 8.2 Load Functionality
- [ ] App loads saved data on startup
- [ ] CD information persists after restart
- [ ] Track information persists after restart
- [ ] Slot count setting persists after restart
- [ ] Statistics reflect saved data

**Notes**: _________________________________________________

### 8.3 File Operations
- [ ] JSON file created on SD card
- [ ] File path: `/ext/apps/Tools/flipchanger_data.json`
- [ ] File updates when data changes
- [ ] No file corruption

**Notes**: _________________________________________________

---

## 9. UI/UX Issues

### 9.1 Help & Instructions
- [ ] Help accessible from Main menu (5th option)
- [ ] R key opens Help in Slot list, Slot details, Settings, Statistics
- [ ] Help shows key bindings (U/D, K, B, LB, R, LPU/LPD)
- [ ] B or K closes Help and returns to previous view
- [ ] No footer overlap (footers removed)

**Notes**: _________________________________________________

### 9.2 Menu Scrolling
- [ ] Slot details shows 3 items at a time
- [ ] Add/Edit shows 4 fields at a time
- [ ] Scrolling works smoothly
- [ ] Selected item always visible

**Notes**: _________________________________________________

### 9.3 Text Scrolling
- [ ] Long text fields scroll horizontally
- [ ] Cursor stays visible when scrolling
- [ ] Text doesn't overwrite (scrolls instead)

**Notes**: _________________________________________________

---

## 10. Crash Testing

### 10.1 Navigation Crashes
- [ ] No crash when navigating between views
- [ ] No crash when pressing BACK repeatedly
- [ ] No crash when pressing buttons rapidly
- [ ] No crash when exiting app

**Notes**: _________________________________________________

### 10.2 Input Crashes
- [ ] No crash when editing fields
- [ ] No crash when adding/deleting characters
- [ ] No crash when editing tracks
- [ ] No crash with long text input

**Notes**: _________________________________________________

### 10.3 Edge Cases
- [ ] No crash with empty fields
- [ ] No crash with maximum values
- [ ] No crash with minimum values (3 slots)
- [ ] No crash with maximum slots (200)
- [ ] No crash when file doesn't exist
- [ ] No crash with corrupted data

**Notes**: _________________________________________________

---

## 11. Performance

### 11.1 Responsiveness
- [ ] App responds quickly to button presses
- [ ] No noticeable lag when navigating
- [ ] No lag when loading views
- [ ] Statistics calculation is fast

**Notes**: _________________________________________________

### 11.2 Memory
- [ ] No "out of memory" errors
- [ ] App doesn't slow down over time
- [ ] No memory leaks

**Notes**: _________________________________________________

---

## 12. Overall Assessment

### 12.1 Functionality
- [ ] All core features work
- [ ] All menus are accessible
- [ ] Data persists correctly
- [ ] Settings work correctly

**Notes**: _________________________________________________

### 12.2 Usability
- [ ] App is easy to navigate
- [ ] Instructions are clear
- [ ] UI is readable
- [ ] Controls are intuitive

**Notes**: _________________________________________________

### 12.3 Stability
- [ ] App doesn't crash during normal use
- [ ] App handles errors gracefully
- [ ] App exits cleanly

**Notes**: _________________________________________________

---

## Critical Issues Found

List any critical issues (crashes, data loss, etc.):

1. _________________________________________________
2. _________________________________________________
3. _________________________________________________

---

## Minor Issues / Suggestions

List any minor issues or suggestions for improvement:

1. _________________________________________________
2. _________________________________________________
3. _________________________________________________

---

## Test Summary

**Total Tests**: _____  
**Passed**: _____  
**Failed**: _____  
**Partial**: _____  
**Skipped**: _____

**Overall Status**: 
- [ ] ✅ Ready for use
- [ ] ⚠️ Has minor issues
- [ ] ❌ Has critical issues

**Recommendation**: _________________________________________________

---

**Testing completed on**: _______________  
**Tester signature**: _______________
