typedef struct{
    int w;
    int z;
} random_t;

random_t* new_random();
void set_seed(random_t* r, int seed);
int next_rand(random_t* r);
