# FlipChanger - Flipper App Store Submission Roadmap

**Last Updated**: February 7, 2025  
**Status**: In Progress - Preparing for Submission

---

## Overview

This document tracks our progress toward submission to the official Flipper Zero App Store (Flipper Apps Catalog). It includes compliance requirements, current status, and action items.

---

## App Store Requirements Checklist

### ✅ Repository & Licensing
- [x] **Open Source Repository**: ✅ GitHub repository created and active
- [x] **License File**: ✅ MIT License included
- [x] **Repository Structure**: ✅ Clean, organized structure

### ✅ Application Manifest
- [x] **Manifest File**: ✅ `application.fam` exists
- [x] **App ID**: ✅ `flipchanger` (unique, lowercase)
- [x] **App Type**: ✅ `EXTERNAL` (correct for user apps)
- [x] **Entry Point**: ✅ `flipchanger_main` defined
- [x] **Category**: ✅ `Tools` (appropriate category)
- [x] **Dependencies**: ✅ `gui`, `storage` declared
- [x] **Stack Size**: ✅ 3072 bytes (optimized)

### ✅ Documentation
- [x] **README.md**: ✅ Main README with description
- [x] **Build Instructions**: ✅ Clear build/deploy instructions
- [x] **Usage Instructions**: ✅ Navigation guide included
- [x] **App-Specific README**: ✅ `flipchanger-app/README.md` exists

### 🚧 Code Quality
- [x] **Compiles Without Warnings**: ✅ No warnings/errors
- [x] **Memory Safety**: ✅ Comprehensive bounds checking
- [x] **Error Handling**: ✅ Robust safety checks
- [ ] **Code Comments**: 🚧 Needs improvement (documentation)
- [ ] **Code Style**: ⚠️ Review against Flipper standards

### 🚧 Feature Completeness
- [x] **Core Functionality**: ✅ Working (CD tracking, metadata storage)
- [x] **Data Persistence**: ✅ Save/load working
- [x] **User Interface**: ✅ Functional UI
- [ ] **Error Recovery**: 🚧 Basic (could improve)
- [ ] **Edge Cases**: 🚧 Mostly handled (needs testing)

### 📋 App Store Specific
- [x] **App Icon**: ✅ 10×10 PNG in `flipchanger-app/images/flipchanger.png`, `fap_icon` in manifest
- [ ] **App Description**: 🚧 Needs finalization
- [ ] **Version Number**: ⚠️ Need to add version field to manifest
- [ ] **Author Information**: ⚠️ Need to add author field
- [ ] **Screenshots**: ❌ Need to create
- [ ] **Video Demo**: ❌ Optional but recommended

### 📋 Compliance
- [x] **No Unauthorized Protocols**: ✅ No restricted frequencies
- [x] **Proper API Usage**: ✅ Using official APIs correctly
- [x] **Trademark Compliance**: ✅ No unauthorized use of trademarks
- [ ] **App Store Guidelines Review**: 📋 Need to review official guidelines

---

## Current Status Assessment

### ✅ Completed Requirements (Ready for Submission)

1. **Repository Setup**: Complete
   - GitHub repository: ✅
   - License: ✅ MIT
   - Documentation: ✅

2. **Application Structure**: Complete
   - Manifest: ✅ Valid
   - Build system: ✅ Working
   - Core functionality: ✅ Working

3. **Basic Requirements**: Complete
   - Open source: ✅
   - Free: ✅
   - Functional: ✅

### 🚧 Needs Improvement (Before Submission)

1. **App Metadata**: Missing/Incomplete
   - App icon: ✅ Done (10×10 PNG, fap_icon in manifest)
   - Version field: ⚠️ Need to add to manifest
   - Author field: ⚠️ Need to add to manifest
   - Description: 🚧 Needs finalization

2. **Code Quality**: Needs Polish
   - Code comments: 🚧 Sparse
   - Documentation: 🚧 Needs more inline docs

3. **User Experience**: Improved
   - Track editing: ✅ Complete (title, duration)
   - Field display: ✅ 4 fields visible, scrollable
   - UI/UX: ✅ Full-screen layout, Help menu, wrap-around scroll

4. **Testing**: Needs More Coverage
   - Edge cases: 🚧 Mostly handled
   - Error scenarios: 🚧 Basic handling
   - Data corruption: 🚧 Basic handling

### ❌ Missing Requirements (Critical)

1. **App Icon**: ✅ Done (10×10 PNG)
2. **Version Number**: ❌ Should be in manifest
3. **Author Information**: ❌ Should be in manifest

---

## Submission Process

Based on Flipper Zero documentation, the submission process is:

1. **Prepare Your App**
   - ✅ App builds successfully
   - ✅ App runs on device
   - ✅ All requirements met

2. **Fork/Clone Apps Catalog Repository**
   - 📋 Need to locate official Flipper Apps Catalog repo
   - 📋 Clone/fork the repository

3. **Add Your App**
   - 📋 Add app entry to catalog
   - 📋 Include metadata, description, screenshots

4. **Submit Pull Request**
   - 📋 Submit PR to official repository
   - 📋 Wait for review

5. **Review Process**
   - 📋 Manual review
   - 📋 Automated checks
   - 📋 Testing by maintainers

6. **Publication**
   - 📋 Once approved, available in Flipper Mobile/Lab
   - 📋 Listed in official app catalog

---

## Action Items for Submission

### Priority 1: Critical (Must Have)
- [x] **Create app icon** ✅ (10×10 PNG in manifest)
- [x] **Add version to manifest** ✅ (`fap_version="1.2.0"`)
- [x] **Add author to manifest** ✅ (`fap_author="FlipChanger Contributors"`)
- [x] **Finalize app description** ✅ (fap_description in manifest)
- [ ] **Review official submission guidelines** (find exact requirements)

### Priority 2: Important (Should Have)
- [x] **Improve code comments** ✅ (architecture, sections, key logic)
- [ ] **Create screenshots** (app in action)
- [ ] **Test all edge cases** (comprehensive testing)
- [x] **Add version history** ✅ (CHANGELOG.md)
- [ ] **Improve error messages** (user-friendly)

### Priority 3: Nice to Have (Optional)
- [ ] **Create video demo** (showcase app)
- [ ] **Add more inline documentation**
- [ ] **Optimize code style** (match Flipper standards)
- [ ] **Add unit tests** (if framework available)

---

## Version History

### v1.0.0–v1.0.3 (Current)
- ✅ Core CD tracking functionality
- ✅ JSON storage (save/load)
- ✅ Add/Edit CD interface (4 fields visible)
- ✅ Track management (add/delete/edit title+duration)
- ✅ Data persistence
- ✅ Memory optimization
- ✅ Full-screen layout (5 slots, 4 fields, 5 tracks)
- ✅ Help menu, wrap-around scroll, long-press skip by 10
- ✅ Custom app icon

### v1.1.0 (Feb 2025) – Complete
- Multi-Changer support (Name, Location, Slots per Changer)
- Changer select at top menu; header shows Changer name; main menu scrollable
- Changer admin (Add, Edit, Delete); persist last-used; migration from legacy
- Splash screen on launch

### v1.2.0 (Feb 2025) – Complete
- Enhanced fields: Disc Number (0=unset, 1–999), Album Artist (for compilations)
- Add/Edit form: Artist, Album Artist, Album, Disc #, Year, Genre, Notes, Tracks
- Slot details: shows album artist and disc number when set
- App manifest: fap_version, fap_author, fap_description for App Store

---

## Research Needed

### Flipper Apps Catalog Repository
- [ ] Locate official repository URL
- [ ] Review submission format/requirements
- [ ] Check example submissions
- [ ] Understand metadata format

### App Icon Specifications
- [ ] Icon size requirements
- [ ] Icon format (PNG, ICO, etc.)
- [ ] Icon design guidelines

### Manifest Requirements
- [ ] Version field format
- [ ] Author field format
- [ ] Additional optional fields

---

## Estimated Timeline

### Phase 1: Preparation (1-2 weeks)
- Create app icon
- Add version/author to manifest
- Improve code documentation
- Comprehensive testing

### Phase 2: Submission Prep (1 week)
- Create screenshots
- Finalize description
- Review guidelines
- Prepare catalog entry

### Phase 3: Submission (1 week)
- Submit to catalog
- Address review feedback
- Fix any issues found

### Phase 4: Publication
- Wait for approval
- Respond to feedback
- Finalize submission

**Total Estimated Time**: 3-5 weeks from current state

---

## Resources

### Official Documentation
- [ ] Flipper Zero Developer Docs (find exact URL)
- [ ] Flipper Apps Catalog repository (locate)
- [ ] Submission guidelines (find)
- [ ] Manifest documentation (review)

### Community Resources
- [ ] Example app submissions (study format)
- [ ] Community forums (ask questions)
- [ ] Flipper Discord/Telegram (get help)

---

## Repository Management

### ✅ Repository Setup (Complete)
- [x] **GitHub Repository**: ✅ Moved to Dolphin-Developers organization
- [x] **Repository URL**: ✅ https://github.com/Dolphin-Developers/FlipChanger
- [x] **Remote Configuration**: ✅ Git remote updated and verified
- [x] **License**: ✅ MIT License included
- [x] **Documentation**: ✅ Comprehensive docs in place

### 📋 Branch Protection (Recommended)
- [ ] **Branch Protection Rules**: 📋 Set up for `main` branch
  - **Recommended Settings**:
    - ✅ Require pull request reviews before merging (1 approval minimum)
    - ✅ Require conversation resolution before merging
    - ✅ Include administrators (applies rules to all)
    - ❌ Do not allow force pushes
    - ❌ Do not allow deletions
  - **Optional Settings**:
    - Require status checks (if CI/CD is set up)
    - Require signed commits (if GPG signing is used)
    - Require linear history (if rebase-only workflow preferred)
  - **Setup Location**: GitHub → Settings → Branches → Add rule
  - **Note**: Branch protection is optional for solo projects but recommended for team collaboration and preventing accidental data loss

### Repository Best Practices
- ✅ Clean commit history
- ✅ Clear commit messages
- ✅ Organized file structure
- 📋 Consider adding CODEOWNERS file (for future team collaboration)
- 📋 Consider adding GitHub Actions for CI/CD (optional)

---

## Notes

- App is functionally complete and stable
- Core features work as intended
- Need to focus on polish and compliance
- Icon complete; version/author in manifest still needed
- Submission process seems straightforward (PR-based)
- Repository successfully migrated to Dolphin-Developers organization

---

**Status**: Ready for final polish, then submission!
