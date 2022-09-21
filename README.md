# xs - an extra small game engine

xs is an experiment into making the smallest game making tool possible. It can also be a tool to teach introduction to programming for game developers, who might not necessarily be programmers.
- xs is using the [Wren](https://wren.io/) scripting language
- xs has no graphical interface beyond the reload script and pause/play buttons
- xs has no audio support, except in the 'audio' branch of this repository
- xs can be built for PC, Nintendo Switch, and PlayStation 5
- xs is in a very early alpha (but open for contribution from anyone at games@buas)

## running an example
Download the latest [release](https://github.com/BredaUniversityGames/xs/releases) and run the file _xs.exe_.

- Starting with version 0.2.0, _xs.exe_ will try to read a file named _games/.ini_. If that file does not exist, xs will create one and fill it with _hello_, so that it will automatically run the game in the _games/hello_ folder (a simple 'hello world' example). To run a different game, find the _games/.ini_ file and replace its contents by the name of another game (let's say _yourgame_). Upon execution, wren will then try to run the game inside the _games/yourgame/_ folder.
- Before xs version 0.2.0, _xs.exe_ needed a command-line parameter to specify which game to run. 

## xs script
Every xs game folder should contain a main script named _game.wren_. It always has the following basic structure:

```csharp
import "xs" for Data, Input, Render    // These are the parts of the xs we will be using

// The entry point (main) is the Game class
class Game {
    static config() {       
        // init gets called by our engine once, when the scipt is initialilzed
        
        // Configure the window in xs
        Data.setNumber("Width", 640, Data.system)
        Data.setNumber("Height", 360, Data.system)
        Data.setNumber("Multiplier", 1, Data.system)
    }        
    
    static update(dt) {
        // Update gets called by our engine every frame
    }
}
```

## tools
A nice code editor can make you more productive. [Visual Studio Code](https://code.visualstudio.com/) has a extension for Wren and built in command line (terminal)

## Generate build files with CMake
> ⚠️ Currently only tested on Windows

Alongside the `xs.sln` provided in the root folder you can build the project files by opening a terminal in the root folder and running:
```bash
mkdir build
cd build
cmake ..
```
the generated files will populate `/build`, where you can find the generated `xs.sln`. 

## NDA
Before browsing the xs source code, please sign the BUas umbrella NDA. This so that you can browse console code freely. 

