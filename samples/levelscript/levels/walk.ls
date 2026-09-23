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
    W,  // Wall
    F,  // Floor
    H,  // Head
    S   // Seed
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
        W W W W W
        W W W W W
        W W S W W
        W W W W W
        W W W W W ]
}

// "Clears" empty tiles with Wall 
rule clear {
    level[.] => level[W]
}

// Place walk heads inside the solid mass.
rule start {
    level[S] => level[H]
}

// Random walk: a H eats an adjacent W cell, leaving F behind.
// Same weight for all 4 directions - and, unlike dungeon.ls's walk2, free
// to step back onto its own trail, so paths loop and cross like Nuclear
// Throne's tangled tunnels instead of staying a clean orthogonal maze.
rule walk(rotation=all) {
    all
    level[H W]  => level[F H]
    level[H F] => level[F H]
}

// Cave a W in next to an existing tunnel - fattens the 1-cell-wide
// walk into the irregular, rounded-room blobs the source game is known
// for, instead of a thin corridor maze.
rule widen(rotation=all) {
    level[F W] => level[F F]
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
      (weight=2)  entities[skeleton_knife]
      (weight=2)  entities[skeleton_axe]
      (weight=1)  entities[skull]
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
    level[.] => level[W]
    level[H] => level[F]
    level[S] => level[F]    
}

rule walls {
    all
    level[
        * W *
        W W *
        * W *]
    =>
    tiles[
        * 12 *
        * 12 *
        * *  *
    ]
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

program {
    resize(5, 5)
    all init
    //all clear
    upscale(3, 2)
    some(max=8) start
    some(max=100, policy=incremental) walk
    // some(percent=12) widen
    // some(percent=12) widen
    all reduce
    one place_player
    all reward
    some(max=40) spawn_enemies
    pad(1)
    all clean
    all decorate
}
