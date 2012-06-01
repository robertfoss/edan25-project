#define STRINGIFY(A) #A
std::string kernel_source = STRINGIFY(


/** int32 atomics are enabled and supported by all opencl1.1 devices. */
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable

struct uint_2{
	unsigned int a;
	unsigned int b;
	int semaphor;
};
typedef struct uint_2 uint_2;

typedef struct{
    int index;
    int listed;
    unsigned int pred_count;
    unsigned int succ_count;
	int semaphor;
} vertex_t;


double sec(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double) tv.tv_sec + (double)tv.tv_usec / 1000000;
}


void bitset_set_bit(unsigned int* arr, int index, unsigned int bit){
    unsigned int bit_offset = (bit / (sizeof(unsigned int) * 8));
    unsigned int bit_local_index = (unsigned int) (bit % (sizeof(unsigned int) * 8));
    //printf("\nSET\tbit = %d\tbit_offset = %d\tbit_local_index = %d\n", bit, bit_offset, bit_local_index);
    arr[bitset_size * index + bit_offset] |= (1 << bit_local_index);
}

int bitset_get_bit(unsigned int* arr, int index, unsigned int bit){
    unsigned int bit_offset = (bit / (sizeof(unsigned int) * 8));
    unsigned int bit_local_index = (unsigned int) (bit % (sizeof(unsigned int) * 8));
    //printf("\nbit_offset = %d\tbit_local_index = %d\n", bit_offset, bit_local_index);
    return ((arr[bitset_size * index + bit_offset]) & (1 << bit_local_index));
}


unsigned int* bitset_copy(unsigned int* bs, int index){
    unsigned int* new_bs;
    new_bs = (unsigned int*) malloc(bitset_size);
    for(unsigned int i = 0; i < (sizeof(unsigned int) * (nsym / (sizeof(unsigned int) * 8) + 1)); ++i){
        new_bs[i] = bs[index * bitset_size + i];
    }
    return new_bs;
}

int bitset_equals(unsigned int* bs1, unsigned int* bs2){
    for(unsigned int i = 0; i < (sizeof(unsigned int) * (nsym / (sizeof(unsigned int) * 8) + 1)); ++i){
        if(bs1[i] != bs2[i]){
            return 0;
        }
    }
    return 1;
}

void bitset_or(unsigned int* bs1, unsigned int* bs2){
    for(unsigned int i = 0; i < (sizeof(unsigned int) * (nsym / (sizeof(unsigned int) * 8) + 1)); ++i){
        bs1[i] |= bs2[i];
    }
}

void bitset_and_not(unsigned int* bs1, unsigned int* bs2){
    for(unsigned int i = 0; i < (sizeof(unsigned int) * (nsym / (sizeof(unsigned int) * 8) + 1)); ++i){
        unsigned int tmp = bs1[i] & bs2[i];
        tmp = ~tmp;
        bs1[i] = tmp & bs1[i];
    }
}
yu7yu7tr5tr45r5t4r45r45r4r45rr45r45r45yu7hyu7hu7tftrftr5hyu7uhyuygftrfrhyu7ju
void acquire_lock(__global uint_2* x) {
   int occupied = atomic_xchg(&x[0].semaphor, 1);
   while(occupied > 0)
   {
     occupied = atomic_xchg(&x[0].semaphor, 1);
   }
}


void release_lock(__global uint_2* x)
{
   int prevVal = atomic_xchg(&x[0].semaphor, 0);
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


void computeIn(vertex_t* vertices){

    /*Vertex* v;
    acquire_locks(u, u->succ_list, u->pred_list);

	list_t* tmp_list = u->succ_list;
	do{
        tmp_list = tmp_list->next;
		v = tmp_list->data;
        if(v != NULL){
		    bitset_or(u->out, v->in);
        }
	}while(tmp_list->next != tmp_list);

	unsigned int* old = bitset_copy(u->in);
	u->in = calloc( alloc_size, sizeof(unsigned int));//bitset_create();
	bitset_or(u->in, u->out);
	bitset_and_not(u->in, u->def);
	bitset_or(u->in, u->use);

	if(!bitset_equals(u->in, old)){
		tmp_list = u->pred_list;
		do{
            tmp_list = tmp_list->next;
			v = tmp_list->data;
			if(v != NULL){
                if(!v->listed){
				    add_last(worklist, create_node(v));
				    v->listed = true;
                }
			}
		}while(tmp_list->next != tmp_list);
	}
    free(old);
    unlock_locks(u, u->succ_list, u->pred_list);
*/
}


__kernel void liveness(__global vertex_t* vertices)
{
    unsigned int i = get_global_id(0);

    c[i] = gcd(x[i].a, x[i].b);
}


/*__kernel void liveness(__global uint_2* x, __global unsigned int* c)
{
    unsigned int i = get_global_id(0);

    c[i] = gcd(x[i].a, x[i].b);
}*/




);
