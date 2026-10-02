
// Import the room tags and layers
use "room_schema.ls"

// Import the visual representation rules
use "visual.ls" 

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
// Main
///////////////////////////////////////////////////////////////////////////////////////////////////
sequence main {
    // Create the initial seed room somewhere in the middle(ish) of the map
    resize(16, 7)
    one seed_room
    pad(6)

    //some(max=4) build_dungeon
    all build_dungeon
    some(percent=50) connect_rooms
    all mark_hallways
    all remove_walls_and_doors
    all recreate_walls
    all colorize
    
    one hero
    //one steps
    some(max=12) rewards
    some(max=15) enemies
    some(max=12) obstacles
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
