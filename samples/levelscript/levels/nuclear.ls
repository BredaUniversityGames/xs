// @S 42
// Nuclear Throne inspired generator. Starts from dungeon.ls's random-walk
// skeleton (same tags/layers, same init/start/reduce/place_player/clean),
// but pushes the result closer to the source game's look: the walk is
// allowed to cross its own trail (tangled, looping tunnels rather than a
// clean maze), a widen pass caves in tunnel Ws so corridors swell into
// the rounded, irregular rooms Nuclear Throne is known for, and the result
// is populated with low-tier mobs (skeleton_knife/skeleton_axe/skull)
// instead of being loot-only.
tag items     { chest, heart, potion, sword, shield }
tag entities  {
    player,
    skeleton_knife,
    skeleton_axe,
    skull,
    necromancer
}
tag geometry  { W, F, H, S } // wall, floor, head, seed

layers {
    level:    grid of geometry
    tiles:    grid of number
    entities: grid of entities
    items:    grid of items
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

rule place_player {
    level[H] => entities[player]
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
    level[H]
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
    level[H] => level[F]
    level[S]     => level[F]
}

program {
    resize(6, 6)
    all init
    //all clear
    upscale(3, 2)
    some(max=8) start
    some(max=200, policy=incremental) walk
    some(percent=12) widen
    some(percent=12) widen
    all reduce
    one place_player
    all reward
    some(max=40) spawn_enemies
    all clean
}
