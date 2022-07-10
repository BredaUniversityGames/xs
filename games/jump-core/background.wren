
import "xs" for Configuration, Input, Render, Registry, File
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation, Label
import "random" for Random
import "globals" for Globals
import "tags" for Team, Tag
import "debug" for DebugColor

class Parallax is Component {
    construct new(speed, width, repeat) {
        _speed = speed
        _width = width
        _repeat = repeat
    }

    update(dt) {        
        var t = owner.getComponent(Transform)
        t.position.x = t.position.x - dt * _speed

        if(t.position.x < Configuration.width * -0.5 -_width) {
            t.position.x = t.position.x + _width * _repeat
        }
    }
}

class Background {
    static create() {
        __levels = ["daytime", "night", "abandoned"]
        __random = Random.new()
        __layers = [
            "[games]/jump-core/images/backgrounds/[level]/background.png",
            "[games]/jump-core/images/backgrounds/[level]/cloud01.png",
            "[games]/jump-core/images/backgrounds/[level]/cloud02.png",
            "[games]/jump-core/images/backgrounds/[level]/cloud03.png",
            "[games]/jump-core/images/backgrounds/[level]/cloud04.png",
            "[games]/jump-core/images/backgrounds/[level]/cloud05.png",
            "[games]/jump-core/images/backgrounds/[level]/cloud06.png",
            "[games]/jump-core/images/backgrounds/[level]/cloud07.png",
            "[games]/jump-core/images/backgrounds/[level]/cloud08.png",
            "[games]/jump-core/images/backgrounds/[level]/buildingsback.png",
            "[games]/jump-core/images/backgrounds/[level]/buildingsfront.png",
            "[games]/jump-core/images/backgrounds/[level]/building01.png",
            "[games]/jump-core/images/backgrounds/[level]/building02.png",
            "[games]/jump-core/images/backgrounds/[level]/building03.png",
            "[games]/jump-core/images/backgrounds/[level]/building04.png",
            "[games]/jump-core/images/backgrounds/[level]/building05.png",
            "[games]/jump-core/images/backgrounds/[level]/building06.png",
            "[games]/jump-core/images/backgrounds/[level]/building07.png",
            "[games]/jump-core/images/backgrounds/[level]/building08.png",
            "[games]/jump-core/images/backgrounds/[level]/train.png",
            "[games]/jump-core/images/backgrounds/[level]/traintracks.png",            
            "[games]/jump-core/images/backgrounds/[level]/trees.png",
        ]

        var widths = [
            320,    // bg
            520,    // cl 1
            520,    // cl 2
            520,    // cl 3
            520,    // cl 4
            520,    // cl 5
            520,    // cl 6
            520,    // cl 7
            520,    // cl 8
            320,    // bl bk
            320,    // bl f
            520,    // b1
            520,    // b2
            520,    // b3
            520,    // b4
            520,    // b5
            520,    // b6
            520,    // b7
            520,    // b8
            720,    // train
            320,    // train tr
            320,    // trees
        ]

        var speeds = [
            0,    // bg
            25.0,   // cl 1
            25.0,   // cl 2
            22.0,   // cl 3
            16.0,   // cl 4
            12.0,   // cl 5
            13.0,   // cl 6
            15.0,   // cl 7
            15.0,   // cl 8
            40,   // bl bk
            80,   // bl f
            100,    // b1
            100,    // b2
            100,    // b3
            100,    // b4
            100,    // b5
            100,    // b6
            100,    // b7
            100,    // b8
            120,  // train t
            120,  // train
            140   // trees
        ]

        var heights = [
            0,    // bg
            220,  // cl 1
            130,  // cl 2
            230,  // cl 3
            170,  // cl 4
            120,  // cl 5
            120,  // cl 6
            170,  // cl 7
            170,  // cl 8
            0,    // bl bg    
            0,    // bl f
            0,      // b1
            0,      // b2
            0,      // b3
            0,      // b4
            0,      // b5
            0,      // b6
            0,      // b7
            0,      // b8
            26.0,   // train 
            0,    // train tr
            0     // trees
        ]

        var starts = [
            0,    // bg
            30,    // cl 1
            -10,    // cl 2
            280,    // cl 3
            130,    // cl 4
            170,    // cl 5
            220,    // cl 6
            90,    // cl 7
            290,    // cl 8
            0,    // bl bg
            0,    // bl front
            100,      // b1
            150,      // b2
            200,      // b3
            250,      // b4
            300,      // b5
            350,      // b6
            400,      // b7
            500,      // b8
            0,    // train 
            0,    // train tr
            0,    // trees
        ]

        var w = Configuration.width * 0.5
        var h = Configuration.height * 0.5

        __stuff = []
        for(i in 0...22) {
            for(j in 0..3) {
                var parallax = Entity.new()
                __stuff.add(parallax)
                var t = Transform.new(
                    Vec2.new(-w + widths[i] * j + starts[i],
                    -h + heights[i]))

                var fileName = __layers[i].replace("[level]", "abandoned")
                var s = Sprite.new(fileName, 0, 0, 1.0, 1.0)
                parallax.addComponent(s)

                var p = Parallax.new(speeds[i], widths[i], 3)
                parallax.addComponent(t)
                parallax.addComponent(p)
            }
        }

        setLevel("daytime")
    }

    static setLevel(level) {
        var idx = 0
        for(i in 0...22) {
            var fileName = __layers[i].replace("[level]", level)
            for(j in 0..3) {
                if(File.exists(fileName)) {                
                    var s = Sprite.new(fileName, 0, 0, 1.0, 1.0)
                    __stuff[idx].addComponent(s)
                }
                idx = idx + 1
            }
        }
    }

    static setRandomLevel() {
        var i = __random.int(0, __levels.count)
        var l = __levels[i]
        Background.setLevel(l)
    }
}
