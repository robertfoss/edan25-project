#include <stdio.h>
#include <stdlib.h>


#include "vertex.h"
#include "rand.h"

void print_bs(unsigned int* bs);


void bitset_set_bit(unsigned int* arr, int index, unsigned int bit, unsigned int bitset_size)
{
        unsigned int bit_offset = (bit / (sizeof(unsigned int) * 8));
        unsigned int bit_local_index = (unsigned int) (bit % (sizeof(unsigned int) * 8));
        //printf("\nSET\tbit = %d\tbit_offset = %d\tbit_local_index = %d\n", bit, bit_offset, bit_local_index);
        arr[bitset_size * index + bit_offset] |= (1 << bit_local_index);
}

int bitset_get_bit(unsigned int* arr, int index, unsigned int bit, unsigned int bitset_size)
{
        unsigned int bit_offset = (bit / (sizeof(unsigned int) * 8));
        unsigned int bit_local_index = (unsigned int) (bit % (sizeof(unsigned int) * 8));
        //printf("\nbit_offset = %d\tbit_local_index = %d\n", bit_offset, bit_local_index);
        return ((arr[bitset_size * index + bit_offset]) & (1 << bit_local_index));
}


unsigned int* bitset_copy(unsigned int* bs, int index, unsigned int bitset_size)
{
        unsigned int* new_bs;
        new_bs = (unsigned int*) malloc(bitset_size);
        for(unsigned int i = 0; i < bitset_size; ++i) {
                new_bs[i] = bs[index * bitset_size + i];
        }
        return new_bs;
}

int bitset_equals(unsigned int* bs1, unsigned int* bs2, unsigned int bitset_size)
{
        for(unsigned int i = 0; i < bitset_size; ++i) {
                if(bs1[i] != bs2[i]) {
                        return 0;
                }
        }
        return 1;
}

void bitset_or(unsigned int* bs1, unsigned int* bs2, unsigned int bitset_size)
{
        for(unsigned int i = 0; i < bitset_size; ++i) {
                bs1[i] |= bs2[i];
        }
}

void bitset_and_not(unsigned int* bs1, unsigned int* bs2, unsigned int bitset_size)
{
        for(unsigned int i = 0; i < bitset_size; ++i) {
                unsigned int tmp = bs1[i] & bs2[i];
                tmp = ~tmp;
                bs1[i] = tmp & bs1[i];
        }
}


void print_vertex(vertex_t* v, int nsym, int* in, int* out, int* use, int* def, int bitset_size)
{
        int i;
        int index = v->index;

        printf("use[%d] = { ", index);
        for (i = 0; i < nsym; ++i) {
                unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
                unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));

                if(use[index * bitset_size + bit_offset] & (1 << bit_local_index)) {
                        //if ((v->use[bit_offset] & (1 << bit_local_index))){
                        printf("%d ", i);
                }
        }
        printf("}\n");
        printf("def[%d] = { ", index);

        for (i = 0; i < nsym; ++i) {
                unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
                unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));
                if(def[index * bitset_size + bit_offset] & (1 << bit_local_index)) {
                        //if ((v->def[bit_offset] & (1 << bit_local_index))){
                        printf("%d ", i);
                }
        }
        printf("}\n\n");
        printf("in[%d] = { ", index);

        for (i = 0; i < nsym; ++i) {
                unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
                unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));
                if(in[index * bitset_size + bit_offset] & (1 << bit_local_index)) {
                        //if ((v->in[bit_offset] & (1 << bit_local_index))){
                        printf("%d ", i);
                }
        }
        printf("}\n");
        printf("out[%d] = { ", index);

        for (i = 0; i < nsym; ++i) {
                unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
                unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));
                if(out[index * bitset_size + bit_offset] & (1 << bit_local_index)) {
                        //if ((v->out[bit_offset] & (1 << bit_local_index))){
                        printf("%d ", i);
                }
        }
        printf("}\n\n");
}


void print_vertices(int nvertex, int maxsucc, vertex_t* vertices, unsigned int* pred_list, unsigned int* succ_list){
    printf("print_vertices:\n");
    for(int i = 0; i < nvertex; ++i){
        printf("%d: %d succ[%d] = {", i, vertices[i].succ_count, vertices[i].index);
        for(unsigned int j = 0; j < vertices[i].succ_count; ++j){
            printf(" %d", succ_list[i * maxsucc + j]);
        }
        printf("}\n");
    }
}


void connect(vertex_t* pred, vertex_t* succ, int maxpred, int maxsucc, unsigned int* pred_list, unsigned int* succ_list)
{

        int pred_i = pred->index;
        int succ_i = succ->index;
        //printf("\npred_i = %d, pred->succ_count = %d\tsucc_i = %d, succ->pred_count = %d\n", pred_i, pred->succ_count, succ_i, succ->pred_count);

        // printf(" setting succ_list[%d * %d + %d = %d] = %d\n", pred_i, maxsucc, pred->succ_count, pred_i * maxsucc + pred->succ_count, succ_i);

        succ_list[pred_i * maxsucc + pred->succ_count] = succ_i;
        pred_list[succ_i * maxpred + succ->pred_count] = pred_i;

        pred->succ_count++;
        succ->pred_count++;
}


void generateCFG(vertex_t* vertex_list, int nvertex, random_t* r, int maxpred, int maxsucc, unsigned int* pred_list, unsigned int* succ_list, int print_input)
{
        //printf("generateCfg\n");

        int s;
        int k;

        connect(&vertex_list[0], &vertex_list[1], maxpred, maxsucc, pred_list, succ_list);
        connect(&vertex_list[0], &vertex_list[2], maxpred, maxsucc, pred_list, succ_list);

        for(int i = 2; i < nvertex; ++i) {
                if(print_input) {
                        printf("[%d] succ = {", i);
                }
                s = next_rand(r) % maxsucc + 1;
                for(int j = 0; j < s; ++j) {
                        k = abs(next_rand(r)) % nvertex;
                        if(print_input) {
                                printf(" %d", k);
                        }
                        connect(&vertex_list[i], &vertex_list[k], maxpred, maxsucc, pred_list, succ_list);
                }
                if(print_input) {
                        printf("}\n");
                }
        }

        //print_cfg(vertex_list);

}


/*void print_bs_set(unsigned int* bs){
    //printf("print_bs_set:\n");
    unsigned int* tmp;
    int s = sizeof(unsigned int) * 8;

    for(int i = 0; i < nvertex; ++i){
        tmp = bitset_copy(bs, i);
        printf("bs[%d] = {", i);
        for(int j = 0; j < sizeof(tmp); ++j){
            unsigned int b = tmp[j];
            for(int i = 0; i < s; ++i){
                if(b & (1 << i)){
                    printf(" %d", i);
                }
            }
        }
        printf("}\n");
    }
}


void print_bs(unsigned int* bs){
    //printf("print_bs:\n");
    unsigned int* tmp;
    int s = sizeof(unsigned int) * 8;

    for(int i = 0; i < nvertex; ++i){
        tmp = bitset_copy(bs, i);
        printf("bs[%d] = \n", i);
        for(int j = 0; j < sizeof(tmp); ++j){
            unsigned int b = tmp[j];
            for(int i = 0; i < s; ++i){
                if(b & (1 << i)){
                    printf("1");
                } else {
                    printf("0");
                }
            }
            printf(" |\n");
        }
    }
}*/


void generateUseDef(vertex_t* vertex_list, int nvertex, int nactive, int nsym, bitset_t* use, bitset_t* def, random_t* r, int bitset_size, int print_input)
{
        //printf("generateUseDef\n");
        int sym;

        for(int i = 0; i < nvertex; ++i) {
                if(print_input) {
                        printf("[%d] usedef = {", vertex_list[i].index);
                }

                for(int j = 0; j < nactive; ++j) {
                        sym = abs(next_rand(r)) % nsym;
                        //printf("sym = %d\t rand = %d\t abs(rand) = %d\n", sym, rand, abs(rand));

                        if(j % 4 != 0) {
                                if(!bitset_get_bit(def, i, sym, bitset_size)) {

                                        if(print_input) {
                                                printf(" u %d", sym);
                                        }
                                        bitset_set_bit(use, i, sym, bitset_size);
                                }
                        } else {
                                if(!bitset_get_bit(use, i, sym, bitset_size)) {
                                        if(print_input) {
                                                printf(" d %d", sym);
                                        }
                                        bitset_set_bit(def, i, sym, bitset_size);
                                }

                        }
                }
                if(print_input) {
                        printf("}\n");
                }
        }

        /*printf("printing use:\t");
        print_bs_set(use);
        printf("printing def:\t");
        print_bs_set(def);*/
}


void create_vertices(int nsym, int nvertex, int maxsucc, int nactive, int print_input, vertex_t **vertices,
                          unsigned int *bitset_size, unsigned int **pred_list, unsigned int **succ_list, bitset_t **in,
                          bitset_t **out, bitset_t **use, bitset_t **def)
{
        printf("Creating vertices..\n");

        *bitset_size = 50*(nsym / (sizeof(unsigned int) * 8)) + 1;
        printf("Defined bitset_size = %u bytes.\n", *bitset_size);

        unsigned int data_size = 0;

        *vertices = (vertex_t*) malloc(sizeof(vertex_t) * nvertex);
        data_size += sizeof(vertex_t)*nvertex;

        *in  = (bitset_t*) calloc(nvertex * (*bitset_size), sizeof(bitset_t));
        *out = (bitset_t*) calloc(nvertex * (*bitset_size), sizeof(bitset_t));
        *use = (bitset_t*) calloc(nvertex * (*bitset_size), sizeof(bitset_t));
        *def = (bitset_t*) calloc(nvertex * (*bitset_size), sizeof(bitset_t));

        data_size = sizeof(bitset_t ) * (*bitset_size) * nvertex * 4;

        *pred_list = (unsigned int*) malloc((sizeof(int) * nvertex) * nvertex);
        data_size += (sizeof(int) * nvertex) * nvertex;
        *succ_list = (unsigned int*) malloc((sizeof(int) * maxsucc) * nvertex);
        data_size += (sizeof(int) * nvertex) * maxsucc;

        printf("Vertex data size: %.2f KB.\n", ((float)data_size/1024));

        random_t *rand = new_random();
        set_seed(rand, 1);

        for(int i = 0; i < nvertex; ++i) {
                (*vertices)[i].index = i;
                (*vertices)[i].listed = 1; // "in worklist"
                (*vertices)[i].pred_count = 0;
                (*vertices)[i].succ_count = 0;
        }

        generateCFG(*vertices, nvertex, rand, nvertex, maxsucc, *pred_list, *succ_list, print_input);
        generateUseDef(*vertices, nvertex, nactive, nsym, *use, *def, rand, *bitset_size, print_input);


}
