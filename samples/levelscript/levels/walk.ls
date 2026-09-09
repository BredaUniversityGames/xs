// @seed 21
// Incremental growth: each application re-collects and sees prior writes,
// so the blob feeds on itself — the drunkard's-walk family. Under the
// default snapshot policy the same rule could only ring the first seed.
tag algo { S }

layers {
    algo: grid of algo
}

rule plant {
    algo[.]
    =>
    algo[S]
}

rule grow(rotation=all) {
    algo[S .]
    =>
    algo[* S]
}

program {
    resize(13, 9)
    one plant
    some(max=45, policy=incremental) grow
}
