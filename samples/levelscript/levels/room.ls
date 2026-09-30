
tag algo {
    R,  // Room
    H,  // Hall
    W   // Wall
    D   // Door
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
    Door,
    Flag,
    Spikes,
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

tag colors {
    Red,
    Blue
}

layers {
    level:    grid of algo      // The algorithm and the collision geometry
    tiles:    grid of number    // The ground tiles
    rooms:    grid of room      // The room types
    entities: grid of stuff     // The items and characters in the level
    colors:    grid of colors   // The color layer
    enemies_viz:    grid of number    // Just a visual representation of enemies
    items_viz:      grid of number    // Just a visual representation of items
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Dungeon creation
///////////////////////////////////////////////////////////////////////////////////////////////////

rule seed_room {any
    // 5x5 room
    level[
        . . . . . . . 
        . . . . . . . 
        . . . . . . . 
        . . . . . . . 
        . . . . . . . 
        . . . . . . . 
        . . . . . . . ] 
    =>
    { all
    level[
        W W D D D W W 
        W R R R R R W 
        D R R R R R D 
        D R R R R R D 
        D R R R R R D 
        W R R R R R W 
        W W D D D W W ]
    
    entities[
        . . . .     . . . 
        . . . .     . . . 
        . . . .     . . . 
        . . . Steps . . . 
        . . . .     . . . 
        . . . .     . . . 
        . . . .     . . . ] 
    }
}

rule extend_door(rotation = all, symmetry = all) {any
    level[
        W D W 
        . . . ]
        
    => { any
        level[
            W D W 
            . D . ]
        level[
            W W W 
            . . . ]
    }

    level[
        W D D W 
        . . . .]        
    => { any
        level[
            W D D W 
            . D D . ]
        level[
            W D W W 
            . D . . ]
        level[
            W W W W 
            . . . . ]
    }
    level[
        W D D D W
        . . . . .]
    => {any
        level[
            W D D D W
            . D D D . ]                        
        level[
            W D D W W
            . D D . . ]
        level[
            W W D D W
            . . D D . ]
        level[
            W W W D W
            . . . D . ]
        level[
            W W D W W
            . . D . . ]
        level[
            W D W W W
            . D . . . ]
        level[
            W W W W W 
            . . . . . ]
    }
}

rule grow_room(rotation = all) {any
    // 3x3 room
    level[
        . . . . . 
        . . . . . 
        D . . . . 
        . . . . . 
        . . . . . ]
    =>
    level[
        W W D W W 
        W R R R W 
        D R R R D 
        W R R R W 
        W W D W W ]

    // 4x3 room
    level[
        . . . . . . 
        . . . . . . 
        D . . . . . 
        . . . . . . 
        . . . . . . ]
    =>
    level[
        W W D D W W 
        W R R R R W 
        D R R R R D 
        W R R R R W 
        W W D D W W ]

    // 5x3 room
    level[
        . . . . . . . 
        . . . . . . . 
        D . . . . . . 
        . . . . . . . 
        . . . . . . . ]
    =>
    level[
        W W D D D W W 
        W R R R R R W 
        D R R R R R D 
        W R R R R R W 
        W W D D D W W ]    
    
    // 4x4 room
    level[
        . . . . . . 
        . . . . . . 
        D . . . . . 
        D . . . . . 
        . . . . . . 
        . . . . . . ]
    =>
    level[
        W W D D W W 
        W R R R R W 
        D R R R R D 
        D R R R R D 
        W R R R R W 
        W W D D W W ]
    
    // 5x5 room
    level[
        . . . . . . . 
        . . . . . . . 
        D . . . . . . 
        D . . . . . . 
        D . . . . . . 
        . . . . . . . 
        . . . . . . . ] 
    =>
    level[
        W W D D D W W 
        W R R R R R W 
        D R R R R R D 
        D R R R R R D 
        D R R R R R D 
        W R R R R R W 
        W W D D D W W ]
}

sequence build_dungeon {
    one extend_door
    one grow_room    
}

rule connect_rooms(rotation = all) { any
    // 4x4
    level[
        R W W R  
        R D D R 
        R D D R 
        R W W R ]
    =>
    level[
        R R R R  
        R R R R 
        R R R R 
        R R R R ]
    
    // 4x3
    level[
        R W W R  
        R D D R 
        R W W R ]
    =>
    level[
        R R R R  
        R R R R 
        R R R R ]
}

rule mark_hallways(rotation = all) { all
    level[
        * W W * 
        R D D R 
        * W W * ]
    =>
    level[
        * W W * 
        R H H R 
        * W W * ]

    level[
        * W W * 
        R D D R 
        R D D R 
        * W W * ]
    =>
    level[
        * W W * 
        R H H R 
        R H H R 
        * W W * ]
    
    level[
        * W W * 
        R D D R 
        R D D R 
        R D D R 
        * W W * ]
    =>
    level[
        * W W * 
        R H H R 
        R H H R 
        R H H R 
        * W W * ]
}

rule remove_walls_and_doors { all
    level[D] => level[.]
    level[W] => level[.]
}

rule recreate_walls(rotation=all, symmetry=all) { all
    level[F . ] => level[* W ]
    level[
        F * 
        * . ]
    =>
    level[
        * * 
        * W ]
}

rule seed_color { any
    { all colors[.] level[R] } => colors[Red]
    { all colors[.] level[R] } => colors[Blue]
}

rule expand_colors(rotation=all) { all
    { all colors[Red . ] level[R R ] } => colors[Red Red ]
    { all colors[Red . ] level[R W ] } => colors[Red Red ]    
    { all colors[Blue . ] level[R R ] } => colors[Blue Blue ]
    { all colors[Blue . ] level[R W ] } => colors[Blue Blue ]
}

sequence colorize {
    one seed_color
    all(policy=stabilize) expand_colors
}

rule assign_start(rotation = all) {
    { all
        level[
            W W W
            H R W
            W W W]

        rooms[
            . . .
            . . .
            . . .]
    }
    => rooms[
        Start Start Start
        Start Start Start
        Start Start Start]
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
        End End End]
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
        Reward Reward Reward]
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
        Challenge Challenge Challenge]
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
    //{ all level[R] rooms[Challenge] entities[.] } =>
    { all level[R] entities[.] } =>
    { any
        entities[SkeletonAxe] 
        entities[SkeletonSword]
        entities[Necromancer]
        entities[Skull]        
    }
}

rule obstacles {
    { all level[R] entities[.] } => entities[Obstacle]
}

rule doors { any
    level[
        W H W 
        * R * ]
    =>
    entities[
        * Door * 
        * *    *]

    level[
        W H H W 
        * R R * ]
    =>
    entities[
        * Door Door * 
        * *    *    * ]
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
        34 * 
        *  45]
    
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
        2 
        * ]
    
    // Bottom
    level[
        F 
        W ]
    => tiles[
        * 
        42]
        

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
        F]
    tiles[
        *
        .]
    } => tiles[
        *
        12]
    
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

rule colored_corners { ordered
// Red corner decorations (up-right)
    { all
        level[
            F W 
            F F ] 
        colors[
            *   * 
            Red * ]
    }
    =>
    tiles[
        * 52
        * * ]
    
    // Red corner decorations (up-left)
    { all
        level[
            W F 
            F F ] 
        colors[
            * *  
            * Red]
    }
    =>
    tiles[
        53 * 
        *  * ]

    // Red corner decorations (down-right)    
    { all
        level[
            F F 
            W F ] 
        colors[
            * Red
            * *  ]
    }
    =>
    tiles[
        *  * 
        62 * ]

    // Red corner decorations (down-left)
    { all
        level[
            F F 
            F W ] 
        colors[
            Red * 
            *   * ]
    }
    =>
    tiles[
        * * 
        * 63]
    
    { all
        level[
            W F 
            F F ] 
        colors[
            Red Red 
            *   *   ]
    }
    =>
    tiles[
        53 * 
        * * ]

    // Blue corner decorations (up-right)
    { all
        level[
            F W 
            F F ] 
        colors[
            *    * 
            Blue *   ]
    }
    =>
    tiles[
        * 50
        * * ]
    
    // Blue corner decorations (up-left)
    { all
        level[
            W F 
            F F ] 
        colors[
            * *    
            * Blue ]
    }
    =>
    tiles[
        51* 
        * * ]

    // Blue corner decorations (down-right)
    { all
        level[
            F F 
            W F ] 
        colors[
            * Blue
            * *  ]
    }
    =>
    tiles[
        *  * 
        61 * ]

    // Blue corner decorations (down-left)
    { all
        level[
            F F 
            F W ] 
        colors[
            Blue * 
            *   * ]
    }
    =>
    tiles[
        * * 
        * 60]
}

rule colored_top_wall { ordered
    { all
        tiles[2 2 2 2 2]
        colors[Blue * * * Blue ]
    } =>
    { any
        tiles[2 54 55 56 2 ]
        // tiles[54 74 55 75 56]
        tiles[2 74 55 75 2 ]
        
    }

    { all
        tiles[2 2 2 2 ]
        colors[Blue Blue Blue Blue]
    } =>
    { any
        tiles[2 55 55 2 ]
        tiles[* *  *  * ]        
    }

    { all
        tiles[2 2 2 ]
        colors[Blue Blue Blue]
    } =>
    { any
        tiles[2  55 2  ]
        tiles[54 55 56 ]
        tiles[*  *  *  ]
    }

    { all
        tiles[2 2 2 2 2]
        colors[Red Red Red Red Red ]
    } =>
    { any
        tiles[2  76 83 77 2 ]        
        tiles[83 2  2  2  83]        
    }

    { all
        tiles[2 2 2 2]
        colors[Red Red Red Red ]
    } =>
    { any
        tiles[83 2  2  83]
        tiles[*  *  *  * ]
    }

    { all
        tiles[2 2 2]
        colors[Red Red Red ]
    } =>
    { any
        tiles[2  83 2  ]
        tiles[76 83 77 ]
        tiles[*  *  *  ]
    }
}
 rule portals_light { all
    tiles[53 * 52] => tiles[80 * 81]
    tiles[53 * * 52] => tiles[80 * * 81]
}

rule large_floor_area { all
    { all
         level[
            F F F F
            F F F F
            F F F F
            F F F F]

        tiles[
            *  *  *  * 
            *  22 22 * 
            *  22 22 * 
            *  *  *  * ]        
    } => 
    { any
        tiles [
            *  *  *  * 
            *  6  7  * 
            *  16 17 * 
            *  *  *  * ]
        
        tiles [
            *  *  *  * 
            *  57 58 * 
            *  67 68 * 
            *  *  *  * ]
    }
}

rule reflections { ordered
    where[
        (tiles == 80 || tiles == 81 || tiles == 83)
        (tiles == 11)]
    =>
    tiles[
        *
        90]

    where[
        (tiles == 80 || tiles == 81 || tiles == 83)
        (tiles == 14)]
    =>
    tiles[
        *
        91]

    where[
        (tiles == 80 || tiles == 81 || tiles == 83)
        (tiles == 12)]
    =>
    tiles[
        *
        92]

    // Only one blue one
    tiles[
        55
        12]
    => tiles[
        55
        65]
}

rule variations { ordered    
    // Left corner light
    // tiles[
    //     2 
    //     11]
    // =>
    // tiles[
    //     80
    //     90]
    
    // // Right corner light
    // tiles[
    //     2 
    //     14]
    // =>    
    // tiles[
    //     81
    //     91]
    
    // Wall
    tiles[0 2] => tiles[0 1]
    tiles[2 5] => tiles[4 5]
    tiles[2] => { any tiles[2] tiles[3] }
    tiles[0] => { any tiles[0] tiles[10] tiles[20] tiles[30] }
    tiles[5] => { any tiles[5] tiles[15] tiles[25] tiles[35] }
    tiles[42] => { any tiles[42] tiles[43] }

    // Floor
    tiles[22] => { any
        tiles[37] tiles[38] tiles[57] tiles[58] tiles[59]
        tiles[67] tiles[68] tiles[69] tiles[22] }
    tiles[11] => { any tiles[11] tiles[26] }
    tiles[14] => { any tiles[14] tiles[29] }
    tiles[12] => { any tiles[12] tiles[13] tiles[27] tiles[28] }
    tiles[21] => { any tiles[21] tiles[36] }
    tiles[24] => { any tiles[24] tiles[39] }
    tiles[31] => { any tiles[31] tiles[46] }
    tiles[32] => { any tiles[32] tiles[47] tiles[33] tiles[48] }    
    tiles[34] => { any tiles[34] tiles[49] }
        
}

rule show_entities { ordered
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
    entities[Hero] => tiles[85]
    entities[Door Door] => items_viz[41 42]
    entities[Door] => { any items_viz[41] items_viz[42] }    
    { all entities[Steps] colors[Red] } => tiles[84]
    { all entities[Steps] colors[Blue] } => tiles[85]    
    entities[Obstacle] => { any
        items_viz[52]
        items_viz[55]
        items_viz[38]
        items_viz[50]
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Program (main)
///////////////////////////////////////////////////////////////////////////////////////////////////
program {
    // Create the initial seed room somewhere in the middle(ish) of the map
    resize(16, 7)
    one seed_room
    pad(5)

    //some(max=4) build_dungeon
    all build_dungeon
    some(percent=50) connect_rooms
    all mark_hallways
    all remove_walls_and_doors
    all recreate_walls
    all colorize
    
    one hero
    //one steps
    some(max=8) rewards
    some(max=12) enemies
    some(max=8) obstacles
    some(percent=50) doors
    
    all wall_tiles
    all colored_corners
    all colored_top_wall
    some(percent=50) portals_light
    all floor_tiles
    all reflections
    some(max=3) large_floor_area
    all variations

    all show_entities    
}
