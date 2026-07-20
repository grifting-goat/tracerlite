#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>
#include <stdlib.h>

typedef struct World {

    uint8_t* voxels;
    uint32_t world_size;



} World;

World create_world(uint32_t size);


#endif //WORLD_H