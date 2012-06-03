#ifndef _VERTEX_H_INCLUDED
#define _VERTEX_H_INCLUDED

typedef unsigned int bitset_t;

typedef struct {
        int index;
        int listed;
        unsigned int pred_count;
        unsigned int succ_count;
        int semaphor;
} vertex_t;


vertex_t* create_vertices(int nsym, int nvertex, int maxsucc, int nactive, int print_input, vertex_t *vertices,
                          unsigned int *bitset_size, unsigned int* pred_list, unsigned int* succ_list, bitset_t* in, bitset_t *out,
                          bitset_t* use, bitset_t* def);


#endif
