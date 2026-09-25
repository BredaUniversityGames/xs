
tag algo {
    R,  // Room
    H,  // Hall
    W   // Wall
    F = R | H   // Floor (room or hall)
}

tag room {
    Start,
    End,
    Reward,
    Challenge
    Hub
}

tag stuff {
    Hero,
    Steps,
    SkeletonAxe,
    SkeletonSword,
    Necromancer,
    Skull,
    Potion,
    Key,
    Coin,
    Treasure,
    Fruit,
    Crystal,
    Obstacle
}


layers {
    level:    grid of algo      // The algorithm and the collision geometry
    tiles:    grid of number    // The ground tiles
    rooms:    grid of room      // The room types
    entities: grid of stuff      // The items and characters in the level
    enemies_viz:    grid of number    // Just a visual representation of enemies
    items_viz:      grid of number    // Just a visual representation of items
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

rule expand_rooms(rotation = all) {
    level[
        * * * *
        * R R *
        * R R *
        * * * * ]
    => level[
        R R R R
        R R R R
        R R R R
        R R R R ]
}

rule merge(rotation = all) {
    level[
        W H H W
        W H H W ]
    => level[
        R R R R 
        R R R R
    ]
}

rule clean(rotation = all) {
    level[
        W H H W 
        W W W W ]
    => level[
        W W W W
        W W W W
    ]
}

rule hallways(rotation = all, symmetry = all) {
    level[
       W H H W
       W H H W ]
    => { any 
        level[
            W W H H 
            W W H H ]
        level[
            H H W W
            H H W W ]
    }
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

rule assign_rewards(rotation = all) {
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
        Reward Reward Reward
        Reward Reward Reward
        Reward Reward Reward ]
}

rule assign_rooms(rotation = all) { any
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

rule hero {
    { all level[R] entities[.] rooms[Start] } => entities[Hero]    
}

rule steps {
    { all level[R] entities[.] rooms[End] } => entities[Steps]    
}

rule rewards {
    { all level[R] entities[.] } =>
    { any
        entities[Crystal] 
        entities[Potion]
        entities[Key]
        entities[Coin]
        entities[Treasure]
        entities[Fruit]    
    }
    
}

rule enemies {
    { all level[R] rooms[Challenge] entities[.] } =>
    { any
        entities[SkeletonAxe] 
        entities[SkeletonSword]
        entities[Necromancer]
        entities[Skull]        
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Visual representation of wall tiles
///////////////////////////////////////////////////////////////////////////////////////////////////

rule wall_tiles { ordered
    // Bottom left corner
    level[
        W F
        W W ]
     => tiles[
        *  31 
        40 * ]
    
    // Bottom right corner
    level[
        F W
        W W ]
     => tiles[        
        34  * 
        * 45 ]
    
    // Top right corner
    level[
        W W
        F W ]
     => tiles[
        * 5 
        14 * ]
    
    // Top left corner
    level[
         W W
         W F ]
     => tiles[
         0 * 
         * 11 ]
        
    // Hallway corners
    level[
        F F
        F W ]
    => tiles[
        * *
        * 70 ]
    // Another hallway corner
    level[
        F F
        W F ]
    => tiles[
        *  *
        71 * ]
    
    // Top
    level[
        W
        F ]
    => tiles[
        1
        * ]
    
    // Bottom
    level[
        F
        W ]
    => tiles[
        *
        41]
        

    // Left
    level[W F] => tiles[0 *]
         
    // Right
    level[F W] => tiles[* 5]
}

rule floor_tiles { ordered
    // Top
    { all
    level[
        W
        F ]
    tiles[
        *
        . ]
    } => tiles[
        *
        12 ]
    
    // Bottom
    { all
    level[
        F
        W ]
    tiles[
        *
        .]
    }
    => tiles[
        32
        *]
        
    // Left
    { all level[W F] tiles[* .] } => tiles[* 21]
         
    // Right
    { all level[F W] tiles[. *] } => tiles[24 *]
    
    { all level[F] tiles[.] } => tiles[22]    
}

rule decorate { ordered
    tiles[
        22 22 
        22 22 ] =>
    { any
        tiles [
            6  7
            16 17 ]
        tiles [
            8  9
            18 19 ]
    }
    
    tiles[
        2 2 2
        22 22 ] =>
    

    tiles[2] => { any tiles[2] tiles[3] }
    tiles[0] => { any tiles[0] tiles[10] tiles[20] tiles[30] }
    tiles[5] => { any tiles[5] tiles[15] tiles[25] tiles[35] }
    tiles[42] => { any tiles[42] tiles[43] }
}

rule show_entities { any
    entities[Crystal] => items_viz[(random(15, 22))]
    entities[Potion] => {any items_viz[32] items_viz[33] items_viz[44] items_viz[45] }
    entities[Key] => items_viz[57]
    entities[Coin] => {any items_viz[39] items_viz[51] }
    entities[Treasure] => items_viz[53]
    entities[Fruit] => {any items_viz[34] items_viz[46] }    
    entities[Necromancer] => enemies_viz[0]
    entities[SkeletonSword] => enemies_viz[1]    
    entities[Skull] => enemies_viz[2]
    entities[SkeletonAxe] => enemies_viz[3]
    entities[Steps] => tiles[94]
    entities[Hero] => tiles[85]
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Program (main)
///////////////////////////////////////////////////////////////////////////////////////////////////

program {
    resize(4, 4)
    one init
    pad(2)
    upscale(3,3)
    one start  
    some(max=7, policy=incremental) grow
    pad(1)
    all close        
    one assign_start
    one assign_end
    all assign_rewards
    all assign_rooms    
    upscale(2,2)
    all expand_rooms
    some (max=3) merge
    all clean
    some (max=2) hallways
    trim()
    pad(1)
    
    one hero
    one steps
    some(max=8) rewards
    some(max=12) enemies

    
    all wall_tiles
    all floor_tiles
    all show_entities
    all decorate
}
