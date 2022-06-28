import "xs" for Configuration, Input, Render, Registry, Color, File
import "xs_ec"for Entity, Component

class Menu {
    construct new(items) {
        _items = items
        _selected = 0
        _actions = {}
        _font = Render.loadFont("[games]/jump-core/fonts/FutilePro.ttf", 18)
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
    }

    render(x, y) {
        Render.renderText(_font, "======== SubOptimal v0.1 ========", x, y, 0, 0, 0)
        var i = 0
        for(item in _items) {
            y = y - 20
            if(_selected == i) {
                Render.renderText(_font, ">" + item + "<", x, y, 0, 0, 0)
            } else {
                Render.renderText(_font, item, x, y, 0, 0, 0)
            }            
            i = i + 1
        }
        y = y - 18
        Render.renderText(_font, "================================", x, y, 0, 0, 0)
    }

    addAction(name, action) {
        _actions[name] = action
    }
}