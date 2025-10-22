# Archive System Refactor Summary

## Session Overview
Refactored the game archive export/import system to be cross-platform compatible using cereal serialization library.

## Problems Identified

The original archive system in `exporter.cpp` and `resource_pipeline.cpp` had critical cross-platform issues:

### 🔴 Critical Issues
1. **Direct struct serialization without format control**
   - Used `memcpy` to serialize `content_header` struct directly
   - Struct padding/alignment varies between compilers
   - Archives created on Windows wouldn't work on Linux

2. **No endianness handling**
   - All integers written in native byte order
   - x86 (little-endian) ≠ ARM/PowerPC (potentially big-endian)

3. **Platform-dependent types**
   - Used `size_t` (4 bytes on 32-bit, 8 bytes on 64-bit)
   - Used `unsigned long` (4 bytes on Win64, 8 bytes on Linux64)

4. **Absolute path storage**
   - Stored full system paths like `C:\Users\...`
   - Won't work across different operating systems

5. **Over-engineered implementation**
   - Two-pass filesystem iteration (count, then process)
   - Complex `root`/`sub_dirs` API
   - Manual memory management with `blob_builder`

## Solution: Migration to Cereal

### Changes Made

#### 1. Added cereal library
- Location: `external/cereal/`
- Header-only serialization library
- Automatically handles endianness and cross-platform compatibility
- Added to include path in `properties/xs.props`

#### 2. Created new archive format (`archive_v2` namespace)

**Files modified:**
- `code/exporter.hpp` - Added new structures and API
- `code/exporter.cpp` - Implemented new export/load functions

**New structures:**
```cpp
struct ContentEntry {
    std::string relative_path;       // Relative path (cross-platform)
    uint64_t uncompressed_size;      // Fixed-size type
    std::vector<std::byte> data;     // File data (compressed if needed)
    bool is_compressed;              // Compression flag

    template<class Archive>
    void serialize(Archive& ar) {
        ar(relative_path, uncompressed_size, data, is_compressed);
    }
};

struct ArchiveData {
    std::vector<ContentEntry> entries;

    template<class Archive>
    void serialize(Archive& ar) {
        ar(entries);
    }
};
```

**New API:**
```cpp
// Export multiple source directories to single archive
bool export_archive(const std::vector<std::string>& source_dirs,
                   const std::string& output_path);

// Load archive into memory
bool load_archive(const std::string& archive_path,
                 archive_v2::ArchiveData& out_archive);

// Decompress entry data
blob decompress_entry(const ContentEntry& entry);
```

#### 3. Updated inspector.cpp
- Switched from `export_content_to_path()` to `export_archive()`
- Now exports both game content and shared resources in one call
- Lines 311-326 in `code/inspector.cpp`

#### 4. Updated fileio.cpp
- Replaced old hash map approach with in-memory archive
- `load_game_content_headers()` - Now loads entire archive at startup
- `read_binary_file()` / `read_text_file()` - Fast hash map lookups
- Much simpler: ~30 lines vs ~80 lines
- Lines 26-97, 103-160 in `code/fileio.cpp`

#### 5. Removed old code
- Deleted entire `archive_generator` namespace (~320 lines)
- Removed `blob_builder` class
- Removed `export_content()` and `export_content_to_path()`
- Removed two-pass filesystem iteration
- Removed manual struct serialization

## Key Improvements

### ✅ Cross-Platform Compatibility
- **Endianness**: Cereal handles automatically
- **Fixed-size types**: Uses `uint64_t` instead of `size_t`/`unsigned long`
- **Path separators**: Normalized to forward slashes
- **Struct padding**: No direct struct serialization
- **Relative paths**: Stored relative to source directory

### ✅ Simplified Architecture
- **Single-pass**: Reads files once during export
- **Cleaner API**: Just pass source directories and output path
- **No pre-calculation**: No need to count entries or calculate sizes
- **In-memory loading**: Archive loaded once, fast lookups

### ✅ More Flexible
- **Multiple source dirs**: Can export from different locations
- **No filesystem requirements**: Sources don't need to be under same root
- **Explicit naming**: Output path specified directly, no magic naming

## Code Statistics

### Lines Changed
- **Removed**: ~400 lines of old code
- **Added**: ~200 lines of new code
- **Net**: -200 lines (50% reduction)

### Files Modified
1. `properties/xs.props` - Added cereal include path
2. `code/exporter.hpp` - New API and structures
3. `code/exporter.cpp` - New implementation
4. `code/inspector.cpp` - Updated export call
5. `code/fileio.cpp` - Updated loading system

## Archive Format Comparison

### Old Format (Incompatible)
```
[size_t entry_count]                    // Platform-dependent size
[content_header]                         // Struct padding issues
[compressed/raw data]
[content_header]
[compressed/raw data]
...
```

### New Format (Cross-Platform)
```
[cereal binary header]
[ArchiveData]
  [vector<ContentEntry>]
    [string relative_path]               // Fixed serialization
    [uint64_t uncompressed_size]         // Fixed 64-bit
    [vector<byte> data]                  // Compressed if text
    [bool is_compressed]
```

## Testing Checklist

### ✅ Completed
- [x] Code compiles successfully
- [x] Cereal library integrated
- [x] Export function implemented
- [x] Load function implemented
- [x] Inspector updated
- [x] Fileio updated
- [x] Old code removed

### ⚠️ Needs Testing
- [ ] Export a game archive from inspector
- [ ] Load and run a game from archive
- [ ] Verify text files decompress correctly
- [ ] Verify binary files load correctly
- [ ] Test with shared resources
- [ ] Build archive on Windows, test on Linux (if applicable)
- [ ] Build archive on 64-bit, test on 32-bit (if applicable)

## Known Issues / TODOs

1. **Archive versioning**: Consider adding version number to detect format changes
2. **Checksums**: No validation that archive isn't corrupted
3. **Large files**: All data loaded into memory (could be issue for huge archives)
4. **Error recovery**: Limited error messages if archive is corrupted

## Example Usage

### Exporting an Archive
```cpp
std::vector<std::string> source_dirs = {
    "C:/projects/mygame/content",
    fileio::get_path("[shared]")  // Engine shared resources
};

if (exporter::export_archive(source_dirs, "C:/output/game.xs")) {
    log::info("Archive exported successfully");
}
```

### Loading an Archive
```cpp
archive_v2::ArchiveData archive;
if (exporter::load_archive("game.xs", archive)) {
    log::info("Loaded {} entries", archive.entries.size());

    // Access individual entries
    for (const auto& entry : archive.entries) {
        blob data = archive_v2::decompress_entry(entry);
        // Use data...
    }
}
```

## Migration Notes

### For Other Developers
- Old `.xs` archives are **not compatible** with new format
- Need to re-export all game archives
- API changed: use `export_archive()` instead of `export_content()`
- Archives now include both game content and shared resources automatically

### Backward Compatibility
There is **no backward compatibility**. This is a breaking change to the archive format.

If you need to support old archives temporarily:
1. Keep old code in a separate branch
2. Detect archive format by checking for cereal header
3. Route to appropriate loader

## References

- **Cereal documentation**: https://uscilab.github.io/cereal/
- **Original issue**: Archives not cross-platform compatible
- **Branch**: `package`
- **Session date**: October 22, 2025

## Next Steps

1. **Test thoroughly** - Build and run games with new archive system
2. **Performance testing** - Compare load times vs old system
3. **Cross-platform test** - Build on Windows, test on Mac/Linux
4. **Documentation** - Update any user-facing docs about archive format
5. **Consider optimizations**:
   - Lazy loading for large archives
   - Archive compression (entire archive, not just text files)
   - Memory-mapped file support
   - Archive validation/checksums

---

**Summary**: Successfully migrated from brittle platform-specific archive format to robust cross-platform solution using cereal. Code is simpler, safer, and more maintainable. Ready for testing phase.
