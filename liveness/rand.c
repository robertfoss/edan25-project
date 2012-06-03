#include <stdio.h>
#include <stdlib.h>

#include "rand.h"

random_t* new_random(){
    random_t* r = malloc(sizeof(random_t));
    r->w = 0;
    r->z = 0;
    return r;
}

void set_seed(random_t* r, int seed){
    r->w = seed + 1;
    r->z = seed * seed + seed + 2;
}

int next_rand(random_t* r){
    r->z = 36969 * (r->z & 65535) + (r->z >> 16);
    r->w = 18000 * (r->w & 65535) + (r->w >> 16);
    return (r->z << 16) + r->w;
}
