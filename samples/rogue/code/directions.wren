import "xs/math" for Vec2

class Directions {
    static upIdx    { 0 }
    static rightIdx { 1 }
    static downIdx  { 2 }
    static leftIdx  { 3 }
    static noneIdx  { 4 }

    static [i] {
        if(i == 0) {
            return Vec2.new(0, 1)   // Up
        } else if(i == 1) {
            return Vec2.new(1, 0)   // Right
        } else if(i == 2) {
            return Vec2.new(0, -1)  // Down
        } else if(i == 3) {
            return Vec2.new(-1, 0)  // Left
        } else if(i == 4) {
            return Vec2.new(0, 0)   // None
        }
    } 
}