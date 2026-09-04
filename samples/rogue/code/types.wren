class Type {
    // Basic level elements
    static empty    { 0 << 0 }
    static floor    { 1 << 0 }
    static wall     { 1 << 1 }

    // Characters
    static player   { 1 << 2 }
    static mage     { 1 << 3 }
    static skeleton { 1 << 4 }
    static ghost    { 1 << 5 }
    static knight   { 1 << 6 }
    static crusader { 1 << 7 } 
    static slime    { 1 << 8 }
    static dragon   { 1 << 9 }
    static ogre     { 1 << 10 }
    static tome     { 1 << 11 }    

    // Items
    static door     { 1 << 12 }
    static key      { 1 << 13 }
    static chest    { 1 << 14 }
    static pot      { 1 << 15 }
    static helmet   { 1 << 16 }
    static armor    { 1 << 17 }
    static sword    { 1 << 18 }
    static food     { 1 << 19 }
    static entrance { 1 << 20 }
    static exit     { 1 << 21 }
    
    // Combine multiple types
    static monster { mage | skeleton | ghost | knight | crusader | slime | dragon | ogre | tome }
    static enemy   { monster }
    static item    { door | key | chest | pot | helmet | armor | sword | food }
    static block   { wall | player | enemy | door }
    static attackable   { enemy | item }
    static blocking     { wall | enemy | door | player | chest | entrance | exit }
    static monsterBlock { wall | pot | chest | item }
}