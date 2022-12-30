class Tag {
    static None             { 0 << 0 }
    static Unit             { 1 << 1 }
    static Bullet           { 1 << 2 }
    static Player           { 1 << 3 }
    static Computer         { 1 << 4 }
    static Deflect          { 1 << 5 }
}

class Team {
    static Player   { 1 }
    static Computer { 2 }
}

class Size {
    static S { 1 }
    static M { 2 }
    static L { 3 }
 }
