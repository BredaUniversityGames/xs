import "xs" for Profiler, Data
import "xs_math" for Vec2

class Game {
    static initialize() {        
        System.print("init")
    }    

    static update(dt) {
        var n = Data.getNumber("n")
        Profiler.begin("Vec2 Creation")
        var vec2s = []
        for(i in 0...n) {
            vec2s.add(Vec2.new(i % 3.14, i % 2.78))
        }
        Profiler.end("Vec2 Creation")

        Profiler.begin("Vec2 Math")
        for(i in 0...n) {
            var a = vec2s[i % 37]
            var b = vec2s[i*3 % 41]

            var c = a + b
            var d = a.dot(b)
        }
        Profiler.end("Vec2 Math")
    }

    static render() {
   }
}