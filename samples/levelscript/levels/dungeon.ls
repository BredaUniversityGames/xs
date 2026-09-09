// @seed 42
// Nuclear Throne style dungeon generator.
tag items     { chest, heart, potion, sword, shield }
tag entities  { player, skeleton_knife, skeleton_axe, skull, necromancer }
tag geometry  { wall, floor, sentinel, seed }

layers {
    level:   grid of geometry
    tiles:   grid of number
    entities: grid of entities
    items:   grid of items
}

// Initialize the level with a single seed cell in the middle
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
        wall wall wall wall wall
        wall wall wall wall wall
        wall wall seed wall wall
        wall wall wall wall wall
        wall wall wall wall wall ]
}

// Place walk heads inside the solid mass.
rule start {
    level[seed] => level[sentinel]
}

// Random walk: a sentinel eats an adjacent wall cell, leaving floor behind.
// same weight for all 4 directions, but only orthogonal turns (no U-turns).
rule walk2(rotation=all) {
    all
    level[sentinel wall]  => level[floor sentinel]
    level[sentinel floor] => level[floor sentinel]
}

// Thin out dense 2x2 sentinel clusters that form after upscale.
// (v0.3.1: rotation=all maps the single-corner RHS to all four corners;
//  reflections-only symmetry=all would miss one corner.)
rule reduce(rotation=all) {
    level[
        sentinel sentinel
        sentinel sentinel ]
   =>
   level[
        sentinel floor
        floor    floor ]
}

rule place_player {
    level[sentinel] => entities[player]
}

// Place a reward on every surviving sentinel cell (walk-head / room centre).
rule reward {
    level[sentinel]
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
    level[sentinel] => level[floor]
    level[seed]     => level[floor]
}

rule hack {
    tiles[.] => tiles[58]
}

// Sized down from the upstream example (60x40 -> 30x20) for easier
// inspection in the xs sample (console dump + on-screen tile size).
program {
    resize(5, 5)
    all init
    upscale(3, 2)
    some(max=8) start
    some(max=200, policy=incremental) walk2
    upscale(2, 2)
    all reduce
    one place_player
    all reward
    all clean
}
