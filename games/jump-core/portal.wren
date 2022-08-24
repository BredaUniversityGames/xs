import "xs" for Configuration, Input, Render, Registry, File
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation, Label
import "debug" for DebugColor
import "tags" for Tag
import "background" for Background
import "random" for Random

class Portal is Component {

    construct new() {
        super()
        _state = Portal.created
        _time = 0.0
        _size = 1.0
        _random = Random.new()        
    }

    update(dt) {
        _time = _time + dt * 10.0
        if(_state == Portal.created) {            
            moveRand()
            _time = 0.0
            _state = Portal.idle
        } else if(_state == Portal.idle) {
            if(Game.checkCollision(owner, Game.playerShip)) {                
                _state = Portal.expand
                _time = 0.0
            }                        
        } else if(_state == Portal.expand) {
            _size = Math.lerp(1.0, 40.0, _time)
            if(_time > 1.0) {
                _time = 0.0
                _state = Portal.shrink                
                Game.jump()
                moveRand()
            }
        } else if(_state == Portal.shrink) {            
            _size = Math.lerp(40.0, 1.0, _time) 
            if(_time > 1.0) {
                _time = 0.0
                _state = Portal.idle
                _size = 1.0
            }
        }        

        var s = owner.getComponent(AnimatedSprite)
        s.scale = _size
    }

    jump() {
        _time = 0.0
        _state = Portal.expand        
    }

    moveRand() {
        var t = owner.getComponent(Transform)
        t.position = Vec2.new(
            _random.float() * 500 - 250,
            _random.float() * 300 - 150)
    }


    static create() {
        var portal = Entity.new()
        var t = Transform.new(Vec2.new(0, 0)) 
        var b = Body.new(Registry.getNumber("Portal Size"), Vec2.new(0, 0))
        var c = DebugColor.new(Registry.getColor("Portal Debug Color"))
        var s = AnimatedSprite.new("[games]/jump-core/images/vfx/Portal_100x100px.png", 7, 7, 45)
        var p = Portal.new()
        s.layer = 0.9
        s.flags = Render.spriteCenter
        var frames = []
        frames.addAll(0..40)
        s.addAnimation("play", frames)
        s.playAnimation("play")
        portal.addComponent(t)
        portal.addComponent(b)
        portal.addComponent(c)
        portal.addComponent(s)
        portal.addComponent(p)
        portal.name = "Portal"
        portal.tag = Tag.portal
        return portal
    }

    static created  { 0 }
    static idle     { 1 }
    static expand   { 2 }
    static shrink   { 3 }
}

import "game" for Game
