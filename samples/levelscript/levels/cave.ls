// @seed 42
// Cellular-automaton cave generator.
// Fill randomly, then erode isolated walls and fill isolated floors.
// @expect grid level count(f) >= 100
// @expect grid level count(w) >= 100

tag terrain { wall, floor }
layers { level: grid of terrain }

rule fill {
    level[.]
    =>
    { any
      (weight=45) level[wall]
      (weight=55) level[floor]
    }
}

// A wall completely surrounded by floor is removed.
rule erode(symmetry=all, rotation=all) {
    level[
        floor floor floor
        floor wall floor
        floor floor floor ]
    =>
    level[
        floor floor floor
        floor floor floor
        floor floor floor ]
}

// A floor completely surrounded by wall is filled in.
rule grow(symmetry=all, rotation=all) {
    level[
        wall wall wall
        wall floor wall
        wall wall wall ]
    =>
    level[
        wall wall wall
        wall wall wall
        wall wall wall ]
}

// Sized down from the upstream example (60x25 -> 24x14) for easier
// inspection in the xs sample. Note: this generator only removes
// single-cell islands (its rules match an exact 3x3 all-floor/all-wall
// surround) - it stays noisy by design, it's a compiler test fixture,
// not a "smooth cave" showcase. Compare against levelscript's own
// examples/cave.expected.
program {
    resize(24, 14)
    all fill
    all erode
    all grow
    all erode
}
