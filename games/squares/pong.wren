import "xs" for Configuration, Input, Render
import "random" for Random

class Game {

    static init() {
        Configuration.width = 360
        Configuration.height = 240

        Configuration.title = "Pong"
        Configuration.multiplier = 2   

        __player = 0.0     
        __ai = 0.0

        __player_score = 0
        __ai_score = 0

        __random = Random.new()
        initBall()
    }

    static initBall() {
        __ball_x = 0.0
        __ball_y = 0.0
        var up_down = __random.float(-1, 1)
        __ball_vy = __random.float(-100, 100)
        __ball_vx = 100
        if(up_down > 0) {
            __ball_vx = -__ball_vx
        }
    }    
    
    static update(dt) {
        var w = Configuration.width
        var h = Configuration.height
        var hh = h / 2 - 10
        var hw = w / 2 - 10
        var maxVel = 100

        Render.setColor(1.0, 1.0, 1.0)

        var plVel = 0.0
        if(Input.getKey(Input.keyUp)) {
            plVel = maxVel
        } else if(Input.getKey(Input.keyDown)) {
            plVel = -maxVel
        }
        __player = __player + plVel * dt
        Render.rect(-hw, __player - 20, -hw + 5, __player + 20)

        var aiVel = (__ball_y - __ai) * 5
        if(aiVel.abs > maxVel) {
            aiVel = aiVel.sign * maxVel
        }
        __ai = __ai + aiVel * dt
        Render.rect(hw, __ai - 20, hw - 5, __ai + 20)

        __ball_x = __ball_x + __ball_vx * dt
        __ball_y = __ball_y + __ball_vy * dt

        if(__ball_y >= hh || __ball_y <= -hh) {
            __ball_vy = -__ball_vy
        }

        if(__ball_x >= hw - 10 && (__ball_y - __ai).abs < 20) {
            __ball_vx = -__ball_vx
        }

        if(__ball_x <= -hw + 10 && (__ball_y - __player).abs < 20) {
            __ball_vx = -__ball_vx
        }

        if(__ball_x >= hw) {
            initBall()
            __player_score = __player_score + 1
        }

        if(__ball_x <= -hw) {
            initBall()
            __ai_score = __ai_score + 1
        }        

        Render.square(__ball_x, __ball_y, 8)

        for(i in 0..22) {
            var y = i * 10 - hh
            Render.square(0.0, y, 4)
        }

        Render.setColor(1.0, 0.0, 0.0)
        Render.text(__player_score.toString, -40, 110, 4)
        Render.text(__ai_score.toString, 25, 110, 4)
    }
}
