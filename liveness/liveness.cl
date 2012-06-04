
/** int32 atomics are enabled and supported by all opencl1.1 devices. */
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable

#pragma OPENCL EXTENSION cl_khr_fp64: enable

#ifndef BUFF_SIZE
#define BUFF_SIZE (1024)
#endif

/*
typedef unsigned int bitset_t;

__constant int nvertex;
__constant int bitset_size;

__constant char buffer[BUFF_SIZE];

typedef struct{
    int index;
    int listed;
    unsigned int pred_count;
    unsigned int succ_count;
	int semaphore;
} vertex_t;

unsigned int myid;
vertex_t* vertices;
int maxpred;
int maxsucc;
int* pred_list;
int* succ_list;
int bitset_size;
bitset_t* in;
bitset_t* out;
bitset_t* use;
bitset_t* def;*/


/*
void bitset_set_bit(bitset_t* arr, int index, unsigned int bit, int bitset_size){
    unsigned int bit_offset = (bit / (sizeof(unsigned int) * 8));
    unsigned int bit_local_index = (unsigned int) (bit % (sizeof(unsigned int) * 8));
    //printf("\nSET\tbit = %d\tbit_offset = %d\tbit_local_index = %d\n", bit, bit_offset, bit_local_index);
	unsigned offset = bitset_size * index + bit_offset;
    arr[offset] |= (1 << bit_local_index);
}

int bitset_get_bit(bitset_t* arr, int index, unsigned int bit, int bitset_size){
    unsigned int bit_offset = (bit / (sizeof(unsigned int) * 8));
    unsigned int bit_local_index = (unsigned int) (bit % (sizeof(unsigned int) * 8));
    //printf("\nbit_offset = %d\tbit_local_index = %d\n", bit_offset, bit_local_index);
    return ((arr[bitset_size * index + bit_offset]) & (1 << bit_local_index));
}



bitset_t* bitset_copy(bitset_t* bs, int bitset_size){
    bitset_t* new_bs;
    new_bs = (bitset_t*) buffer;
    for(unsigned int i = 0; i < bitset_size; ++i){
        new_bs[i] = bs[i];
    }
    return new_bs;
}

int bitset_equals(bitset_t* bs1, bitset_t* bs2, int bitset_size){
    for(unsigned int i = 0; i < bitset_size; ++i){
        if(bs1[i] != bs2[i]){
            return 0;
        }
    }
    return 1;
}

void bitset_or(bitset_t* bs1, bitset_t* bs2, int bitset_size){
    for(unsigned int i = 0; i < bitset_size; ++i){
        bs1[i] |= bs2[i];
    }
}


void bitset_and_not(bitset_t* bs1, bitset_t* bs2, int bitset_size){
    for(unsigned int i = 0; i < bitset_size; ++i){
        unsigned int tmp = bs1[i] & bs2[i];
        tmp = ~tmp;
        bs1[i] = tmp & bs1[i];
    }
}


int acquire_locks(vertex_t* v, vertex_t* vertices, int maxpred, int maxsucc, int* pred_list, int* succ_list){

    int s;
    int p;
    int v_index = v->index;
    int index;
    int fail = 0;

    if(atomic_xchg(&v.semaphore, 1)){
        atomic_xchg(&v.semaphore, 0);
        return 0;
    }

    for(s = 0; s < v->succ_count; ++s){
        index = succ[v_index * maxsucc + s];
        if(atomic_xchg(&vertices[index].semaphore, 1)){
            fail = 1;
            break;
        }
    }

    for(int i = 0; i < s && fail; ++i){
        index = succ[v_index * maxsucc + i];
        atomic_xchg(&vertices[index].semaphore, 0);
    }

    if(fail) return 0;

    for(p = 0; p < v->pred_count; ++p){
        index = pred[v_index * maxpred + p];
        if(atomic_xchg(&vertices[index].semaphore, 1)){
            fail = 1;
            break;
        }
    }

    for(int i = 0; i < p && fail; ++i){
        index = pred[v_index * maxpred + i];
        atomic_xchg(&vertices[index].semaphore, 0);
    }

    if(fail) return 0;

    return 1;
}


int acquire_next(unsigned int myid, vertex_t* vertices, int nvertex, int maxpred, int maxsucc, int* pred_list, int* succ_list){

    int c = myid;
    vertex_t* v;
    int listed_left = 0;

    while(1){

        if(c == nvertex){
            c = 0;
        }

        v = vertices[c];

        if(v->listed && !v->semaphore && acquire_locks(v, vertices, maxpred, maxsucc, pred_list, succ_list)){
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


void free_locks(vertex_t* v, vertex_t* vertices, int maxpred, int maxsucc){

    int v_index = v->index;

    for(int i = 0; i < v->succ_count; ++i){
        atomic_xchg(&vertices[v_index * maxsucc + i].semaphore, 0);
    }
    for(int i = 0; i < v->pred_count; ++i){
        atomic_xchg(&vertices[v_index * maxpred + i].semaphore, 0);
    }

    atomic_xchg(&vertices[v_index].semaphore, 0);
}

void acquire_lock(__global uint_2* x) {
   int occupied = atomic_xchg(&x[0].semaphore, 1);
   while(occupied > 0)
   {
     occupied = atomic_xchg(&x[0].semaphore, 1);
   }
}



void release_lock(__global uint_2* x)
{
   int prevVal = atomic_xchg(&x[0].semaphore, 0);
}


unsigned int gcd(unsigned int a, unsigned int b)
{
    while(1)
    {
        a = a % b;
		if( a == 0 )
			return b;
		b = b % a;

        if( b == 0 )
			return a;
    }
}


void computeIn(unsigned int myid, vertex_t* vertices, int nvertex, int maxpred, int maxsucc, int* pred_list, int* succ_list, int bitset_size,  bitset_t* in, bitset_t* out, bitset_t* use, bitset_t* def){

    int u_index = acquire_next(myid, vertices, nvertex, mexpred, maxsucc, pred_list, succ_list);
    if(u_index == -1){
        return;
    }

    vertex_t* u = vertices[u_index];
//    u->listed = 0;

    vertex_t* v;
    int v_index;
    int bs_u_index = u_index * bitset_size;
    int bs_v_index;

    for(int i = 0; i < u->succ_count; ++i){
        v_index = succs[u_index * maxsucc + i];
        v = vertices[v->index];
        bs_v_index = v_index * bitset_size;
        bitset_or(out[bs_u_index], in[bs_v_index]);
    }

    bitset_t* old = bitset_copy(in[bs_u_index]);

    memset(in[bs_u_index], 0, bitset_size);
    bitset_or(in[bs_u_index], out[bs_u_index]);
    bitset_and_not(in[bs_u_index], def[bs_u_index]);
    bitset_or(in[bs_u_index], use[bs_u_index]);

    if(!bitset_equals(in[bs_u_index], old)){
        for(int i = 0; i < u->pred_count; ++i){
            v_index = preds[u_index * maxpred + i];
            v = vertices[v_index];
            if(!v->listed){
                v->listed = 1;
            }
        }
    }

    free(old);
    free_locks(u, vertices, maxpred, maxsucc);
}

__kernel void liveness(__global vertex_t* vertices, __global int nvertex, int maxpred, int maxsucc, __global int* pred_list, __global int* succ_list, int bitset_size, __global bitset_t* in, __global bitset_t* out, __global bitset_t* use, __global bitset_t* def) {

    unsigned int myid = get_global_id(0);

    // vertices = vs;
    // nvertex = nv;
    // maxpred = mpred;
    // maxsucc = msucc;
    // pred_list = pred;
    // succ_list = succ;
    // bitset_size = bs_size;
    // in = i;
    // out = o;
    // use = u;
    // def = d;

   // computeIn(myid, vertices, nvertex, maxpred, maxsucc, pred_list, succ_list, bitset_size, in, out, use, def);

}
*/

__kernel void liveness( __global double* input, __global double* output, const unsigned int count)
{
   int i = get_global_id(0);
   if(i < count)
       output[i] = input[i] * input[i];

}

