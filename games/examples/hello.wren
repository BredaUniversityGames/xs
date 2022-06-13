System.print("Hello from script")

class Game {
    static config() {
        System.print("config")
    }

    static init() {        
        System.print("init")
        
    }    
    
    static update(dt) {
        System.print("update")
    }

    static render() {
        System.print("render")
    }
}
