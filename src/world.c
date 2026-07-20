#include "world.h"

World create_world(uint32_t size) {
    
    size = size * size * size;

    World wrld;
    wrld.world_size = size;

    wrld.voxels = calloc(size, sizeof(uint8_t));

    for (uint32_t i = 0; i < size; i++) {
        wrld.voxels[i] = rand() % 20 ? 0 : 1;
    }

    return wrld;

}