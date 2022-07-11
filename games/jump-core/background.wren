
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
        __levels = ["daytime", "night", "abandoned", "snow-rain", "sunset"]
        __random = Random.new()
        __layers = [
            // file                                                             width   speeds  height  starts
            // 0                                                                1       2       3       4       
            ["[games]/jump-core/images/backgrounds/[level]/background.png",     320,    0,      0,      0],
            ["[games]/jump-core/images/backgrounds/[level]/cloud01.png",        520,    25,     220,    30],
            ["[games]/jump-core/images/backgrounds/[level]/cloud02.png",        520,    25,     130,    -10],
            ["[games]/jump-core/images/backgrounds/[level]/cloud03.png",        520,    22,     230,    280],
            ["[games]/jump-core/images/backgrounds/[level]/cloud04.png",        520,    16,     170,    130],
            ["[games]/jump-core/images/backgrounds/[level]/cloud05.png",        520,    12,     120,    170],
            ["[games]/jump-core/images/backgrounds/[level]/cloud06.png",        520,    13,     120,    220],
            ["[games]/jump-core/images/backgrounds/[level]/cloud07.png",        520,    15,     170,    90],
            ["[games]/jump-core/images/backgrounds/[level]/cloud08.png",        520,    15,     170,    290],
            ["[games]/jump-core/images/backgrounds/[level]/buildingsback.png",  320,    40,     0,      0],
            ["[games]/jump-core/images/backgrounds/[level]/buildingsfront.png", 320,    80,     0,      0],
            ["[games]/jump-core/images/backgrounds/[level]/building01.png",     520,    100,    0,      100],
            ["[games]/jump-core/images/backgrounds/[level]/building02.png",     520,    100,    0,      150],
            ["[games]/jump-core/images/backgrounds/[level]/building03.png",     520,    100,    0,      200],
            ["[games]/jump-core/images/backgrounds/[level]/building04.png",     520,    100,    0,      250],
            ["[games]/jump-core/images/backgrounds/[level]/building05.png",     520,    100,    0,      300],
            ["[games]/jump-core/images/backgrounds/[level]/building06.png",     520,    100,    0,      350],
            ["[games]/jump-core/images/backgrounds/[level]/building07.png",     520,    100,    0,      400],
            ["[games]/jump-core/images/backgrounds/[level]/building08.png",     520,    100,    0,      500] ,
            ["[games]/jump-core/images/backgrounds/[level]/train.png",          720,    120,    26,     0],
            ["[games]/jump-core/images/backgrounds/[level]/traintracks.png",    320,    120,    0,      0],            
            ["[games]/jump-core/images/backgrounds/[level]/trees.png",          320,    140,    0,      0],
        ]

        var w = Configuration.width * 0.5
        var h = Configuration.height * 0.5

        __stuff = []
        for(i in 0...22) {
            for(j in 0..3) {
                var parallax = Entity.new()
                __stuff.add(parallax)
                var t = Transform.new(
                    Vec2.new(-w + __layers[i][1] * j + __layers[i][4],
                    -h + __layers[i][3]))

                var p = Parallax.new(__layers[i][2], __layers[i][1], 3)
                parallax.addComponent(t)
                parallax.addComponent(p)
            }
        }

        setLevel("daytime")
    }

    static setLevel(level) {
        var idx = 0
        for(i in 0...22) {
            var fileName = __layers[i][0].replace("[level]", level)
            for(j in 0..3) {
                if(File.exists(fileName)) {                         
                    var s = Sprite.new(fileName, 0, 0, 1.0, 1.0)
                    __stuff[idx].addComponent(s)
                } else {
                    __stuff[idx].deleteComponent(Sprite)
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
