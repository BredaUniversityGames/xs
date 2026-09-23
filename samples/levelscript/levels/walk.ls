// Random Walk
// skeleton (same tags/layers, same init/start/reduce/place_player/clean),
// but pushes the result closer to the source game's look: the walk is
// allowed to cross its own trail (tangled, looping tunnels rather than a
// clean maze), a widen pass caves in tunnel Ws so corridors swell into
// the rounded, irregular rooms Nuclear Throne is known for, and the result
// is populated with low-tier mobs (skeleton_knife/skeleton_axe/skull)
// instead of being loot-only.

tag items     {
    chest,
    heart,
    potion,
    sword,
    shield
}

// Hero, monsters and decoration
tag entities  {
    hero,
    skeleton,
    ghost,
    scorpion,
    spider,
    bat,
    snake,
    bear,
    rat,
    ghul,
    buffy
}

tag geometry {
    B,  // Block
    F,  // Floor
    H,  // Head
    S   // Seed
    W,  // Wall
}

layers {
    level:    grid of geometry  // The algorithm and the collision geometry
    tiles:    grid of number    // The ground tiles
    entities: grid of entities  // All the entities
    items:    grid of items     // The items
}

// Initialize the level with a single S cell in the middle
// This will get expanded the next step(s)
rule init {
    level[
        * * * * *
        * * * * *
        * * * * *
        * * * * *
        * * * * * ]
    =>
    level[
        B B B B B
        B B B B B
        B B S B B
        B B B B B
        B B B B B ]
}

// "Clears" empty tiles with Ball 
rule clear {
    level[.] => level[B]
}

// Place walk heads inside the solid mass.
rule start {
    level[S] => level[H]
}

// Random walk: a H eats an adjacent B cell, leaving F behind.
// Same weight for all 4 directions - and, unlike dungeon.ls's walk2, free
// to step back onto its own trail, so paths loop and cross like Nuclear
// Throne's tangled tunnels instead of staying a clean orthogonal maze.
//rule walk(rotation=all) {
//    all
//    level[H W]  => level[F H]
//    level[H F] => level[F H]
//}

rule walk(rotation=all) {
    all
    level[H B]  => level[F H]
}

// Cave a B in next to an existing tunnel - fattens the 1-cell-wide
// walk into the irregular, rounded-room blobs the source game is known
// for, instead of a thin corridor maze.
rule widen(rotation=all) {
    level[F B] => level[F F]
}

// Thin out dense 2x2 H clusters that form after upscale.
rule reduce(rotation=all) {
    level[
        H H
        H H ]
   =>
   level[
        H F
        F F ]
}

rule place_hero {
    level[H] => entities[hero]
}

// Populate surviving F with the source game's low-tier mobs - most
// cells stay clear, a fraction get a skeleton or a skull.
rule spawn_enemies {
    level[F]
    =>
    { any
      (weight=12) level[F]
      (weight=3)  entities[skeleton]
      (weight=2)  entities[ghost]
      (weight=2)  entities[scorpion]
      (weight=2)  entities[spider]
      (weight=2)  entities[bat]
      (weight=2)  entities[snake]
      (weight=1)  entities[bear]
      (weight=1)  entities[rat]
      (weight=1)  entities[ghul]
      (weight=1)  entities[buffy]      
    }
}

// Place a reward on every surviving H cell (walk-head / room centre).
rule reward {
    { all
        level[H]
        entities[.]
    }
    =>
    { any
      items[chest]
      items[heart]
      items[potion]
      items[sword]
      items[shield]
    }
}

rule clean {
    all
    level[.] => level[B]
    level[H] => level[F]
    level[S] => level[F]    
}

rule walls(rotation=all) {
    ordered // Do the cored wall patterns first
    level[
        * F
        B * ]
    => level[
        * F
        W *]

    level[B F] => level[W F]
}

rule wall_tiles { ordered
    // Bottom left corner
    level[
         W F
         W W ]
     => tiles[
         *  * 
         40 * ]
    
    // Bottom right corner
    level[
         F W
         W W ]
     => tiles[
         *  * 
         * 45 ]
    
    // Left
    level[B W] => { any
        tiles[* 0]
        tiles[* 10]
        tiles[* 20]
        tiles[* 30]
    }    

    // Right
    level[W B] => { any
        tiles[5 *]
        tiles[15 *]
        tiles[25 *]
        tiles[35 *]
    }
    
    // Top
    level[  B
            W] => { any
        tiles[  *
                1]
        tiles[  *
                2]
        tiles[  *
                3]
        tiles[  *
                4]
    }
    
    // Bottom
    level[
        W
        B] => { any
        tiles[  41
                *]
        tiles[  42
                *]
        tiles[  43
                *]
        tiles[  44
                *]
    }


    // level[
    //      * F *
    //      F W F
    //      * F *]
    //  => tiles[
    //      * *   *
    //      * 79 *
    //      * *   *]

    //  level[
    //      * F *
    //      F W F
    //      * F *]
    //  => tiles[
    //      * *   *
    //      * 850 *
    //      * *   *]

    // level[
    //     * W *
    //     F W F
    //     * F *]
    // => tiles[
    //     * *   *
    //     * 891 *
    //     * *   *]

    //  level[
    //     * F *
    //     F W W
    //     * F *]
    // => tiles[
    //     * *   *
    //     * 658 *
    //     * *   *]

    // level[
    //     * W *
    //     B W W
    //     * B *]
    // => tiles[
    //     * *   *
    //     * 898 *
    //     * *   *]

    // level[
    //     * F *
    //     F W F
    //     * W *]
    // => tiles[
    //     * *   *
    //     * 793 *
    //     * *   *]

    // level[
    //     * W *
    //     F W F
    //     * W *]
    // => tiles[
    //     * *   *
    //     * 842 *
    //     * *   *]

    // level[
    //     * B *
    //     B W W
    //     * W *]        
    // => tiles[
    //     * *   *
    //     * 800 *
    //     * *   *]

    // level[
    //     * W *
    //     W W B
    //     * B *]
    // => tiles[
    //     * *   *
    //     * 900 *
    //     * *   *]
    
    // level[B W F] => tiles[* 849 *]

    // level[F W B] => tiles[* 851 *]
    
    // level[
    //     B
    //     W
    //     F ]
    // => tiles[
    //     *
    //     801
    //     * ]
    
    // level[
    //     F
    //     W
    //     B ]
    // => tiles[
    //     *
    //     899
    //     * ]
    
    // level[
    //     F
    //     W
    //     F ]
    // => tiles[
    //     *
    //     659
    //     * ]
    
    // level[F] => tiles[1]

//     level[
//         * W *
//         F W W
//         * W *]
//     => tiles[
//         * *   *
//         * 857 *
//         * *   *]

//     level[
//         * F *
//         W W F
//         * F *]
//     => tiles[
//         * *   *
//         * 858 *
//         * *   *]

//     level[
//         * W *
//         W W F
//         * F *]
//     => tiles[
//         * *   *
//         * 859 *
//         * *   *]

//     level[
//         * F *
//         W W W
//         * F *]
//     => tiles[
//         * *   *
//         * 860 *
//         * *   *]

//     level[
//         * W *
//         W W W
//         * F *]
//     => tiles[
//         * *   *
//         * 861 *
//         * *   *]

//     level[
//         * F *
//         W W F
//         * W *]
//     => tiles[
//         * *   *
//         * 862 *
//         * *   *]

//     level[
//         * W *
//         W W F
//         * W *]
//     => tiles[
//         * *   *
//         * 863 *
//         * *   *]

//     level[
//         * F *
//         W W W
//         * W *]
//     => tiles[
//         * *   *
//         * 864 *
//         * *   *]

//     level[
//         * W *
//         W W W
//         * W *]
//     => tiles[
//         * *   *
//         * 865 *
//         * *   *]
}

rule floor_tiles {
    all
    level[F] => tiles[22]
}

rule empty_tile {
    tiles[.] => tiles[99]
}

//rule decorate {
//    all
//    level[W] => tiles[637]
//    entities[player] => tiles[28]
//     items[chest] => tiles[292]
//     items[heart] => tiles[529]
//     items[potion] => tiles[578]
//     items[sword] => tiles[379]
//     items[shield] => tiles[233]
// }

rule item_tiles {
    all
    items[chest] => tiles[292]
    items[heart] => tiles[529]
    items[potion] => tiles[578]
    items[sword] => tiles[379]
    items[shield] => tiles[233]
}

rule entity_tiles {
    all
    entities[hero] => tiles[28]
    entities[skeleton] => tiles[232]
    entities[ghost] => tiles[320]    
    entities[scorpion] => tiles[321]
    entities[spider] => tiles[322]
    entities[bat] => tiles[323]
    entities[snake] => tiles[324]
    entities[bear] => tiles[325]
    entities[rat] => tiles[326]
    entities[ghul] => tiles[327]
    entities[buffy] => tiles[328]
}


program {
    resize(5, 5)
    all init
    //all clear
    upscale(3, 2)
    some(max=8) start
    some(max=80, policy=incremental) walk
    // some(percent=12) widen
    // some(percent=12) widen
    all reduce
    one place_hero
    all reward
    some(max=40) spawn_enemies
    pad(2)
    all clean
    all(policy=stabilize) walls
    all wall_tiles
    all floor_tiles
    //all item_tiles
    //all entity_tiles
    all empty_tile
}
