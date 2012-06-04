
/** int32 atomics are enabled and supported by all opencl1.1 devices. */
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable


// TODO: REMOVE BUFF_SIZE
#ifndef BUFF_SIZE
#define BUFF_SIZE (1024)
#endif

typedef unsigned int bitset_t;


// TODO: REMOVE BUFF_SIZE
//__private char buffer[BUFF_SIZE];

typedef struct{
    int index;
    int listed;
    unsigned int pred_count;
    unsigned int succ_count;
	__global volatile int semaphore;
} vertex_t;



void bitset_set_bit(__global bitset_t* arr, int index, unsigned int bit, int bitset_size){
    unsigned int bit_offset = (bit / (sizeof(unsigned int) * 8));
    unsigned int bit_local_index = (unsigned int) (bit % (sizeof(unsigned int) * 8));
    //printf("\nSET\tbit = %d\tbit_offset = %d\tbit_local_index = %d\n", bit, bit_offset, bit_local_index);
	unsigned offset = bitset_size * index + bit_offset;
    arr[offset] |= (1 << bit_local_index);
}


int bitset_get_bit(__global bitset_t* arr, int index, unsigned int bit, int bitset_size){
    unsigned int bit_offset = (bit / (sizeof(unsigned int) * 8));
    unsigned int bit_local_index = (unsigned int) (bit % (sizeof(unsigned int) * 8));
    //printf("\nbit_offset = %d\tbit_local_index = %d\n", bit_offset, bit_local_index);
    return ((arr[bitset_size * index + bit_offset]) & (1 << bit_local_index));
}


inline
void bitset_copy(__global bitset_t* dest, __global bitset_t* src, int bitset_size){
    for(unsigned int i = 0; i < bitset_size; ++i){
        dest[i] = src[i];
    }
}


inline
int bitset_equals(__global bitset_t* bs1, __global bitset_t* bs2, unsigned int bitset_size){
    for(unsigned int i = 0; i < bitset_size; ++i){
        if(bs1[i] != bs2[i]){
            return 0;
        }
    }
    return 1;
}


inline
void bitset_or(__global bitset_t* bs1, __global bitset_t* bs2, unsigned int bitset_size){
    for(unsigned int i = 0; i < bitset_size; ++i){
        bs1[i] |= bs2[i];
    }
}


inline
void bitset_and_not(__global bitset_t* bs1, __global bitset_t* bs2, unsigned int bitset_size){
    for(unsigned int i = 0; i < bitset_size; ++i){
        unsigned int tmp = bs1[i] & bs2[i];
        tmp = ~tmp;
        bs1[i] = tmp & bs1[i];
    }
}

inline
void bitset_clear(__global bitset_t* bs, unsigned int bitset_size)
{
	for(unsigned int i = 0; i < bitset_size; ++i){
		bs[i] = 0;
	}
}


int acquire_locks(__global vertex_t* v, int maxpred, int maxsucc, __global vertex_t* vertices, __global int* pred_list,
                  __global int* succ_list){

    int s;
    int p;
    int v_index = v->index;
    int index;
    int fail = 0;

    if(atomic_xchg(&(v->semaphore), 1)){
        atomic_xchg(&(v->semaphore), 0);
        return 0;
    }

    for(s = 0; s < v->succ_count; ++s){
        index = succ_list[v_index * maxsucc + s];
        if(atomic_xchg(&(vertices[index].semaphore), 1)){
            fail = 1;
            break;
        }
    }

    for(int i = 0; i < s && fail; ++i){
        index = succ_list[v_index * maxsucc + i];
        atomic_xchg(&(vertices[index].semaphore), 0);
    }

    if(fail) return 0;

    for(p = 0; p < v->pred_count; ++p){
        index = pred_list[v_index * maxpred + p];
        if(atomic_xchg(&(vertices[index].semaphore), 1)){
            fail = 1;
            break;
        }
    }

    for(int i = 0; i < p && fail; ++i){
        index = pred_list[v_index * maxpred + i];
        atomic_xchg(&(vertices[index].semaphore), 0);
    }

    if(fail) return 0;

    return 1;
}


int acquire_next(unsigned int myid, int nvertex, int maxpred, int maxsucc, __global vertex_t* vertices, __global int* pred_list,
                 __global int* succ_list){

    int c = myid;
    __global vertex_t* v;
    int listed_left = 0;

    while(1){

        if(c == nvertex){
            c = 0;
        }

        v = &vertices[c];

        if(v->listed && !v->semaphore && acquire_locks(v, maxpred, maxsucc, vertices, pred_list, succ_list)){
            v->listed = 0;
            return v->index;
        } else if(v->listed){
            listed_left = 1;
        }

        c++;

        if(c == myid && !listed_left){
            return -1;
        } else if(c == myid){
            listed_left = 0;
        }

    }

}


void free_locks(__global vertex_t* v, __global vertex_t* vertices, int maxpred, int maxsucc){

    int v_index = v->index;

    for(int i = 0; i < v->succ_count; ++i){
        atomic_xchg(&vertices[v_index * maxsucc + i].semaphore, 0);
    }
    for(int i = 0; i < v->pred_count; ++i){
        atomic_xchg(&vertices[v_index * maxpred + i].semaphore, 0);
    }

    atomic_xchg(&vertices[v_index].semaphore, 0);
}


void acquire_lock(__global vertex_t* x) {
   int occupied = atomic_xchg(&x[0].semaphore, 1);
   while(occupied > 0)
   {
     occupied = atomic_xchg(&x[0].semaphore, 1);
   }
}



void release_lock(__global vertex_t* x)
{
   int prev_val = atomic_xchg(&x[0].semaphore, 0);
}


void computeIn(unsigned int myid, int nvertex, int maxpred, int maxsucc, unsigned int bitset_size, __global vertex_t* vertices,
               __global int* pred_list, __global int* succ_list,  __global bitset_t* in, __global bitset_t* out,
               __global bitset_t* use, __global bitset_t* def, __global bitset_t* bitset_alloc){

    int u_index = acquire_next(myid, nvertex, maxpred, maxsucc, vertices, pred_list, succ_list);
    if(u_index == -1){
        return;
    }

    __global vertex_t* u = &vertices[u_index];
//    u->listed = 0;

    __global vertex_t* v;
    int v_index;
    int bs_u_index = u_index * bitset_size;
    int bs_v_index;

    for(int i = 0; i < u->succ_count; ++i){
        v_index = succ_list[u_index * maxsucc + i];
        v = &vertices[v->index];
        bs_v_index = v_index * bitset_size;
        bitset_or(&out[bs_u_index], &in[bs_v_index], bitset_size);
    }

	__global bitset_t* old = &(bitset_alloc[bs_u_index]);
	bitset_copy(old, &(in[bs_u_index]), bitset_size);

    bitset_clear(&(in[bs_u_index]), bitset_size);
    bitset_or(&(in[bs_u_index]), &(out[bs_u_index]), bitset_size);
    bitset_and_not(&(in[bs_u_index]), &(def[bs_u_index]), bitset_size);
    bitset_or(&(in[bs_u_index]), &(use[bs_u_index]), bitset_size);

    if(!bitset_equals(&(in[bs_u_index]), old, bitset_size)){
        for(int i = 0; i < u->pred_count; ++i){
            v_index = pred_list[u_index * maxpred + i];
            v = &vertices[v_index];
            if(!v->listed){
                v->listed = 1;
            }
        }
    }

    free_locks(u, vertices, maxpred, maxsucc);
}

__kernel void liveness(unsigned int nvertex, unsigned int maxpred, unsigned int maxsucc, 
                       unsigned int bitset_size, __global vertex_t* vertices, __global unsigned int *pred_list, 
                       __global unsigned int *succ_list, __global bitset_t *in, __global bitset_t *out, 
                       __global bitset_t *use, __global bitset_t *def, __global bitset_t *bitset_alloc){

	unsigned int myid = get_global_id(0);

	computeIn(myid, nvertex, maxpred, maxsucc, bitset_size, vertices, pred_list, succ_list, in, out, use, def, bitset_alloc);
}

