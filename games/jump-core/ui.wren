import "xs" for Configuration, Input, Render, Registry, File
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation, Label

class Menu {
    construct new(items) {
        _items = items
        _selected = 0
        _actions = {}
        _font = Render.loadFont("[games]/jump-core/fonts/FutilePro.ttf", 18)
        {
            var e = Entity.new()
            var t = Transform.new(Vec2.new(0,0))
            // var r = Relation.new(__title)
            _s = Sprite.new("games/jump-core/images/backgrounds/grey.png", 0, 0, 1, 1)
            _s.layer = 10.0
            _s.scale = 20.0
            _s.flags = Render.spriteCenter            
            e.name = "Bar"
            e.addComponent(t)
            e.addComponent(_s)
            // e.addComponent(r)
        }
    }

    update(dt) {
        if(Input.getButtonOnce(13) == true || Input.getKeyOnce(Input.keyDown)) {
            _selected = (_selected + 1) % _items.count
        } else if (Input.getButtonOnce(11) == true || Input.getKeyOnce(Input.keyUp)) {
            _selected = (_selected - 1) % _items.count
        } else if (Input.getButtonOnce(0) == true || Input.getKeyOnce(Input.keySpace)) {
            var item = _items[_selected]
            var action = _actions[item]
            if(action != null) {
                action.call()
            }
        }

        _s.mul = Registry.getColor("Mul Color")
        _s.add = Registry.getColor("Add Color")
    }

    render(x, y) {
        Render.renderText(_font, "======== SubOptimal v0.1 ========", x, y, 0xFFFFFFFF, 0x00000000, 0)
        var i = 0
        for(item in _items) {
            y = y - 20
            if(_selected == i) {
                Render.renderText(_font, ">" + item + "<", x, y, 0xFFFFFFFF, 0x00000000, 0)
            } else {
                Render.renderText(_font, item, x, y, 0xFFFFFFFF, 0x00000000, 0)
            }            
            i = i + 1
        }
        y = y - 18
        Render.renderText(_font, "================================", x, y, 0xFFFFFFFF, 0x00000000, 0)
    }

    addAction(name, action) {
        _actions[name] = action
    }
}

class ImGuiRenderItem_ {
    construct new(text, x, y, color) {
        _text = text
        _x = x
        _y = y
        _color = color
    }

    render(font) {
        Render.renderText(font, _text, _x, _y, _color, 0x00000000, 0)
    }
}

class ImGui is Renderable {
    construct new(font, size) {
        _x = 0.0
        _y = 0.0
        _dy = size * 1.2
        _active = 0
        _guard = 0
        _font = Render.loadFont("[games]/jump-core/fonts/FutilePro.ttf", 18)
    }

    begin() { 
        _x = 0
        _y = 0
        _guard = _guard + 1
        _count = 0
        _queue = []
    }

    begin(x, y) { 
        _x = x
        _y = y
        _guard = _guard + 1
        _count = 0
        _queue = []
    }

    end() {
        _guard = _guard - 1
    }

    text(text) {
        var mul = 0xFFFFFFFF
        var action = false
        if(_active == _count) {
            mul = 0xFFFF00FF
            if (Input.getButtonOnce(0) == true || Input.getKeyOnce(Input.keySpace)) {
                action = true
            }
        }

        var t = owner.getComponent(Transform)
        _queue.add(ImGuiRenderItem_.new(
            text,
            _x + t.position.x,
            _y + t.position.y,
            mul))

        //Render.renderText(_font, text, _x, _y, mul, 0x00000000, 0)

        _count = _count + 1
        _y = _y - _dy
        return action
    }

    slider() {
    }

    update(dt) {
        if(Input.getButtonOnce(Input.gamepadDPadDown) == true || Input.getKeyOnce(Input.keyDown)) {
            _active = (_active + 1)
        } else if (Input.getButtonOnce(Input.gamepadDPadUp) == true || Input.getKeyOnce(Input.keyUp)) {
            _active = (_active - 1)
        }
    }

    render() {
        for(i in _queue) {
            i.render(_font)
        }
        _queue.clear()
    }
}

class MainMenu is Component {
    construct new() {
        super()
        _imgui = null
        _state = "main"
    }

    update(dt) {
        if(_imgui == null) {
            _imgui = owner.getComponent(ImGui)            
        }

        _imgui.begin(-55.0, 20.0)

        // if(_imgui.text("JumpC  ore")) {
        // }
        if(_state == "main") {
            if(_imgui.text("Start")) {
                Game.startPlay()
            }
            if(_imgui.text("Options")) {
                _state = "options"
                System.print("Options")
            }
            if(_imgui.text("Credits")) {
                System.print("Credits")
            }
            if(_imgui.text("Quit")) {
                System.print("Quit")
            }
        } else if(_state == "options") {
            if(_imgui.text("Back")) {
                _state = "main"
            }
        }
        _imgui.end()
    }

    static create() {
        var mainMenu = Entity.new()
        { // Main menu
            var t = Transform.new(Vec2.new(0,0))
            var mm = MainMenu.new()
            var imgui = ImGui.new("[games]/jump-core/fonts/FutilePro.ttf", 18)
            imgui.layer = 12.0
            mainMenu.name = "Main Menu"
            mainMenu.addComponent(t)
            mainMenu.addComponent(mm)
            mainMenu.addComponent(imgui)
        }
        { // Background
            var e = Entity.new()
            var t = Transform.new(Vec2.new(0,0))
            var r = Relation.new(mainMenu)
            var s = Sprite.new("[games]/jump-core/images/backgrounds/small-title.png")
            s.layer = 10.0
            s.flags = Render.spriteCenter            
            e.name = "Title"
            e.addComponent(t)
            e.addComponent(s)
            e.addComponent(r)
        }
        { // Core part
            var e = Entity.new()
            var t = Transform.new(Vec2.new(0, 0))
            var r = Relation.new(mainMenu)
            r.offset = Vec2.new(29, 52)
            var s = AnimatedSprite.new("[games]/jump-core/images/vfx/Electric_Effect_05_small.png", 4, 4, 15)
            s.layer = 10.1
            s.flags = Render.spriteCenter
            s.addAnimation("play", [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14])
            s.playAnimation("play")
            e.name = "Core"
            //bullet.tag = Tag.player | Tag.bullet
            e.addComponent(t)
            e.addComponent(s)
            e.addComponent(r)
        }
        return mainMenu        
    }
}

import "game" for Game
