class Type {
    static empty    { 0 << 0 }
    static floor    { 1 << 0 }
    static wall     { 1 << 1 }
    static player   { 1 << 2 }
    static door     { 1 << 4 }
    static lever    { 1 << 5 }
    static spikes   { 1 << 6 }
    static chest    { 1 << 7 }
    static crate    { 1 << 8 }
    static pot      { 1 << 9 }
    static stairs   { 1 << 10 }
    static light    { 1 << 11 }
    static bat      { 1 << 12 }
    static spider   { 1 << 13 }
    static ghost    { 1 << 14 }
    static boss     { 1 << 15 }
    static scorpion { 1 << 16 }
    static snake    { 1 << 17 }
    static helmet   { 1 << 18 }
    static armor    { 1 << 19 }
    static sword    { 1 << 20 }
    static food     { 1 << 21 }
        
    // Combine multiple types
    static monster { (bat | spider | ghost | boss | scorpion | snake) }
    static enemy   { monster }
    static item    { (helmet | armor | sword | food) }
    static block   { (wall | player | enemy | door) }
    static attackable   { (enemy | item) }
    static blocking     { (wall | enemy | door | player) }
    static monsterBlock { (wall | light | pot | chest | item) }
}