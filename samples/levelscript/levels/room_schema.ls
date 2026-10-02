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