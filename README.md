# xs - an extra small game engine

xs is an experiment into making the smallest game making tool possible. It can also be a tool to teach introduction to programming for game developers, who might not be necessary programmers.
- xs has no graphical interface beyond the reload script and pause/play buttons
- xs is using the [Wren](https://wren.io/) scripting language
- xs is best run from the command line, passing it your main .wren script as a parameter
- xs has not audio support at current
- xs is in a very early alpha (but open for contribution from anyone at games@buas)


## running an example
Download the latest [release](https://github.com/BredaUniversityGames/xs/releases) and unzip it. You can run xs from the command line by giving it the game script:

```
C:\the\path\to\xs> .\xs.exe .\games\examples\pong.wren 
```

## xs script
The xs main script always has this basic structure


```csharp
import "xs" for Configuration, Input, Render    // These are the parts of the xs we will be using

// The entry point (main) is the Game class
class Game {
    static init() {       
        // init gets called by our engine once, when the scipt is initialilzed
        
        // Configure the window in xs
        Configuration.width = 360
        Configuration.height = 240
        Configuration.title = "Window Title"
    }        
    
    static update(dt) {
        // udptate gets called by our engine every frame
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

