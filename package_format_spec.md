# XS Package Format Specification

## Overview

The XS package format (`.xs` files) is a binary format inspired by GLTF, consisting of two main sections:

1. **Metadata Section**: Contains serialized package metadata using Cereal binary archive format
2. **Data Section**: Contains raw binary data blobs referenced by the metadata

This two-section design enables efficient random access and lazy loading of package contents.

## File Structure

```
┌─────────────────────────────────────┐
│     Metadata Section                │
│  (Cereal Binary Archive)            │
│                                     │
│  - Package entry count              │
│  - Entry 1 metadata                 │
│  - Entry 2 metadata                 │
│  - ...                              │
│  - Entry N metadata                 │
├─────────────────────────────────────┤
│     Data Section                    │
│  (Raw Binary Blobs)                 │
│                                     │
│  - Entry 1 data blob                │
│  - Entry 2 data blob                │
│  - ...                              │
│  - Entry N data blob                │
└─────────────────────────────────────┘
```

## Metadata Section Format

The metadata section uses [Cereal](https://uscilab.github.io/cereal/) binary serialization (portable binary archive format).

### Package Structure

```cpp
struct package {
    std::vector<package_entry> entries;
};
```

### Package Entry Structure

```cpp
struct package_entry {
    std::string relative_path;      // File path with wildcard prefix (e.g., "[game]/script.wren")
    uint64_t uncompressed_size;     // Original file size before compression
    uint64_t data_offset;           // Offset in data section where this entry's data starts
    uint64_t data_length;           // Length of data in the data section
    bool is_compressed;             // Whether data is zlib compressed
};
```

### Cereal Binary Format Details

Cereal's portable binary archive uses the following format:

1. **String Encoding**:
   - `uint64_t size` (8 bytes, little-endian)
   - `char data[size]` (UTF-8 encoded string data, no null terminator)

2. **uint64_t Encoding**:
   - 8 bytes, little-endian

3. **bool Encoding**:
   - 1 byte (0x00 = false, 0x01 = true)

4. **Vector Encoding**:
   - `uint64_t size` (8 bytes, little-endian) - number of elements
   - Elements serialized sequentially

### Metadata Section Layout

```
Offset | Type              | Description
-------|-------------------|------------------------------------------
0      | uint64_t          | Number of package entries
8      | package_entry[]   | Array of package entries
```

### Per-Entry Layout

Each `package_entry` is serialized as:

```
Offset | Type      | Field             | Description
-------|-----------|-------------------|----------------------------------
0      | uint64_t  | path_size         | Length of relative_path string
8      | char[]    | path_data         | relative_path string bytes
+N     | uint64_t  | uncompressed_size | Original file size
+8     | uint64_t  | data_offset       | Offset in data section
+8     | uint64_t  | data_length       | Length in data section
+8     | uint8_t   | is_compressed     | Compression flag (0 or 1)
```

## Data Section Format

The data section begins immediately after the metadata section ends. It contains raw binary data blobs stored sequentially.

- Each entry's data can be located using: `data_section_start + entry.data_offset`
- Each entry's data length is: `entry.data_length`
- If `entry.is_compressed` is true, the data is zlib-compressed and must be decompressed to `entry.uncompressed_size` bytes

### Compression

Text files (`.wren`, `.frag`, `.vert`, `.json`, `.txt`) are compressed using zlib (RFC 1950).

Binary files (`.ttf`, `.otf`, `.png`, `.bank`, `.wav`, `.mp3`) are stored uncompressed.

## Reading the Package Format

### Finding the Data Section Start

After reading the metadata section, the current file position is the start of the data section. Save this position for later use when reading data blobs.

### TypeScript/Node.js Implementation Guide

```typescript
import * as fs from 'fs';
import * as zlib from 'zlib';

interface PackageEntry {
    relativePath: string;
    uncompressedSize: bigint;
    dataOffset: bigint;
    dataLength: bigint;
    isCompressed: boolean;
}

interface Package {
    entries: PackageEntry[];
}

function readPackageMetadata(filePath: string): Package {
    const fd = fs.openSync(filePath, 'r');
    let offset = 0;

    // Helper to read uint64_t (little-endian)
    function readUInt64(): bigint {
        const buffer = Buffer.alloc(8);
        fs.readSync(fd, buffer, 0, 8, offset);
        offset += 8;
        return buffer.readBigUInt64LE(0);
    }

    // Helper to read string
    function readString(): string {
        const length = Number(readUInt64());
        const buffer = Buffer.alloc(length);
        fs.readSync(fd, buffer, 0, length, offset);
        offset += length;
        return buffer.toString('utf8');
    }

    // Helper to read bool
    function readBool(): boolean {
        const buffer = Buffer.alloc(1);
        fs.readSync(fd, buffer, 0, 1, offset);
        offset += 1;
        return buffer[0] !== 0;
    }

    // Read package
    const entryCount = Number(readUInt64());
    const entries: PackageEntry[] = [];

    for (let i = 0; i < entryCount; i++) {
        entries.push({
            relativePath: readString(),
            uncompressedSize: readUInt64(),
            dataOffset: readUInt64(),
            dataLength: readUInt64(),
            isCompressed: readBool()
        });
    }

    // offset now points to the start of the data section
    const dataSectionStart = offset;

    fs.closeSync(fd);

    return { entries };
}
```

## Cross-Platform Compatibility

The format is designed to be fully cross-platform:

- **Endianness**: Cereal portable binary archive handles endianness conversion
- **Integer Sizes**: Uses fixed-size types (`uint64_t`)
- **String Encoding**: UTF-8
- **Path Separators**: Normalized to forward slashes `/`
- **Compression**: Standard zlib (RFC 1950)

## Wildcard Paths

Package entries store paths with wildcard prefixes that correspond to the engine's virtual filesystem:

- `[game]/` - Game-specific content
- `[shared]/` - Shared content across projects

Example paths:
- `[game]/scripts/player.wren`
- `[shared]/fonts/roboto.ttf`
- `[game]/images/sprite.png`

## Version History

- **v2 (Current)**: GLTF-style two-section format with Cereal serialization
- **v1 (Deprecated)**: Platform-specific serialization with sequential layout
