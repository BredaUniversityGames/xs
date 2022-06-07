System.print("start run enemy.wren")

class Enemy {
    construct new() {
        _time = 0.0
        System.print("Enemy new")
    }

    update(dt) {
        _time = _time + dt
        if(_time > 2.0) {
            System.print("about the import game from enemy")
            import "game" for GameClass, Game
            Game.addScore(10)
            System.print("Adding score =%(Game.score)")
            System.print("Game initialized =%(Game.initialized)")
            _time = 0.0
        }        
    }
}

System.print("end run enemy.wren")
