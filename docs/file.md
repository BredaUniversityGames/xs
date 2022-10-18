---
layout: default
title: Render
nav_order: 4
---

# File
{: .no_toc }
Provides a small interface for working with files
###  Wildcards

The file paths used the xs should be relative and they should make use of the built in wildcards. 
- `"[game]"` - Expands to the directory of the game (rom). Read only location at when deployed.
- `"[save]"` - Expands to a directory for storing player data. Will cloud sync when possible.
- `"[games]"` - Expands to the root directory of all games. Read only location at when deployed.

Example
```csharp
File.read("[game]/assets/textures/flower.png")
// -> "C:/users/your_user/Documents/xs/games/your_game/assets/textures/flower.png" on Windows when developing
// -> "C:/Program Files (x86)/Steam/steamapps/common/your_game/assets/textures/flower.png" on Windows + Steam

File.write("[save]/save_game.dat")
// -> "C:/users/your_user/AppData/Roaming/xs/your_game/save_game.dat" on Windows when developing
// -> "save:save_game.dat" on consoles
```

## read(src)
Reads a whole file to a string

## write(text, dst)
Writes a text string to a file

## exists(src)
Checks if the file exsists


