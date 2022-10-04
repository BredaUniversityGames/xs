---
layout: default
title: Home
nav_order: 1
---

![image](./img/icon.png)
# An extra small game engine, made with 🧡 at Breda University of Applied Sciences 

**xs** an educational game engine, made with the goal of being the smallest game making tool possible.
It can also be a tool to teach introduction to programming for game developers, who might not necessarily be programmers.
- xs uses the [Wren](https://wren.io/) scripting language
- xs has minimal interface
- xs uses 2D rendering and is best suited for pixel-perfect sprite graphics
- xs has basic audio support (using Fmod) for playing sound effects and background music
- xs can be built for PC, Nintendo Switch, and PlayStation 5


{: .warning }
> xs is in a very early alpha (and  open for contribution from anyone at games@buas)
> Features might be missing or not be stable.


## Running an example
Download the latest [release](https://github.com/BredaUniversityGames/xs/releases) and run the file _xs.exe_. The xs engine will try to read a file named _games/.ini_. If that file does not exist, xs will create one and fill it with _hello_, so that it will automatically run the game in the _games/hello_ folder (a simple 'hello world' example). To run a different game, find the _games/.ini_ file and replace its contents by the name of another game (let's say _yourgame_). Upon execution, xs will then try to run the game inside the _games/yourgame/_ folder.

## An *xs* script
Every xs game folder should contain a main script named *game.wren*. It always has the following basic structure:

```csharp
import "xs" for Data, Input, Render, Audio    // These are the parts of the xs we will be using

// The entry point (main) is the Game class
class Game {
    static config() {       
        // Config gets called before the engine is initialilzed

        // Configure the window in xs
        Data.setNumber("Width", 640, Data.system)
        Data.setNumber("Height", 360, Data.system)
        Data.setNumber("Multiplier", 1, Data.system)
    }

    static init() {        
        // You can initilize you game specific data here.    
    }    
                        
    static update(dt) {
        // The uddate method is called once per tick, gameplay code goes here.    
    }
    
    static render() {
        // The render method is called once per tick, right after update.
    }
}
```
The *hello/game.wren* example shows a bit more of the engine usage. The engine ships with other examples as well.

## Contributors
<ul class="list-style-none">
{% for contributor in site.github.contributors %}
  <li class="d-inline-block mr-1">
     <a href="{{ contributor.html_url }}"><img src="{{ contributor.avatar_url }}" width="32" height="32" alt="{{ contributor.login }}"/></a>
  </li>
{% endfor %}
</ul>