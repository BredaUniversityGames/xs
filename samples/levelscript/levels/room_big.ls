
tag algo {
    R,  // Room
    H,  // Hall
    W   // Wall
}

tag room {
    Start,
    End,
    Reward,
    Challenge
    Hub
}

layers {
    level:    grid of algo      // The algorithm and the collision geometry
    tiles:    grid of number    // The ground tiles
    rooms:    grid of room      // The room types
}

// Pick a starting tile (when the grid is empty and small)
rule init {
    level[.] => level[R]
}

// Start with a small room that we can grow
rule start {
    level[
        R R R
        R R R
        R R R ]
    => level[
        W H W
        H R H
        W H W ]
}

rule grow(rotation = all, symmetry = all) {
    level[
        * . . .
        H . . .
        * . . .
    ]
    =>
    { any
        (weight=2) level[  // Fully open room
            * W H W
            H H R H
            * W H W
        ]
        (weight=2) level[  // Partially closed room
            * W W W
            H H R H
            * W H W
        ]
        (weight=2) level[  // Another variation
            * W H W
            H H R W
            * W H W
        ]
        (weight=3) level[ // Partially open room
            * W W W
            H H R H
            * W W W
        ]
        (weight=3) level[ // Partially open room
            * W H W
            H H R W
            * W W W
        ]
        (weight=4) level[ // Closed room
            * W W W
            H H R W
            * W W W
        ]
    }
}

rule close(rotation = all) {
    level[H .] => level[W .]
}

rule roomfy(rotation = all) { ordered
    //level[* R] => level[R R]
    level[W R] => level[R R]
    level[H R] => level[R R]

    // level[
    //     W W
    //     W R
    // ]
    // => level[
    //     R W
    //     W R
    // ]
    
    // level[
    //     W H
    //     H R
    // ]
    // => level[
    //     R R
    //     R R
    // ]
}

rule corner(rotation = all) {
    level[
        R !R
        R R
    ]
    => level[
        R R
        R R
    ]
}

rule merge(rotation = all) {
    level[
        W W H H H W W
        W W H H H W W ]
    => level[
        R R R R R R R
        R R R R R R R
    ]
}

rule clean(rotation = all) {
    level[
        W W H H H W W
        W W W W W W W ]
    => level[
        W W W W W W W
        W W W W W W W
    ]
}

rule hallways(rotation = all) {
    level[
        H H H
        H H H ]
    => level[
        H H W 
        H H W ]
}

rule assign_start(rotation = all) {
    { all
        level[
            W W W
            H R W
            W W W ]

        rooms[
            . . .
            . . .
            . . . ]
    }
    => rooms[
        Start Start Start
        Start Start Start
        Start Start Start ]
}

rule assign_end(rotation = all) {
    { all
        level[
            W W W
            H R W
            W W W ]
        rooms[
            . . .
            . . .
            . . . ]
    }
    => rooms[
        End End End
        End End End
        End End End ]
}

rule assign_rooms(rotation = all) { any
    { all   
        level[ // Closed room -> Reward
            W W W
            H R W
            W W W ]
        rooms[
            . . .
            . . .
            . . . ]
    }
    => rooms[
        Reward Reward Reward
        Reward Reward Reward
        Reward Reward Reward ]
    
    { all   
        level[ 
            W * W
            * R *
            W * W ]
        rooms[
            . . .
            . . .
            . . . ]
    }
    => rooms[
        Challenge Challenge Challenge
        Challenge Challenge Challenge
        Challenge Challenge Challenge ]
}



program {
    resize(4, 4)
    one init
    pad(2)
    upscale(3,3)
    one start  
    some(max=8, policy=incremental) grow
    pad(1)
    all close    
    trim()
    one assign_start
    one assign_end
    all assign_rooms
    upscale(2,2)
    //all roomfy
    //all (policy=stabilize) corner
    //all roomfy
    //all (policy=stabilize) corner
    //some (max=2) merge
    //all clean
    //all hallways
    pad(1)
}
