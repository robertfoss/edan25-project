#include <pthread.h>
#include <math.h>
#include <sys/times.h>
#include <sys/time.h>
#include <ctype.h>
#include <stdbool.h>

#include "list.h"
#include "opencl.h"
#include "rand.h"
#include "util.h"

bool print_input;
int nvertex;
int	nsym;
unsigned int alloc_size;

unsigned int bitset_subsets;

cl_device_id device_id;				// device id running computation
cl_context context;					// compute context
cl_command_queue queue;				// compute command queue
cl_kernel or_kernel;				// compute kernel
cl_kernel nand_kernel;				// compute kernel
cl_kernel megaop_kernel;			// compute kernel
cl_mem buf_or_bs1, buf_or_bs2;
cl_mem buf_nand_bs1, buf_nand_bs2;
cl_mem buf_megaop_in, buf_megaop_out, buf_megaop_use, buf_megaop_def;
size_t cl_or_local_workgroup_size;
size_t cl_nand_local_workgroup_size;
size_t cl_megaop_local_workgroup_size;

void bitset_or(unsigned int* bs1, unsigned int* bs2, cl_mem *mem1, cl_mem *mem2);
void bitset_and_not(unsigned int* bs1, unsigned int* bs2, cl_mem *mem1, cl_mem *mem2);
void bitset_megaop(unsigned int* in, unsigned int* out, unsigned int* use, unsigned int* def,
                      cl_mem *buf_in, cl_mem *buf_out, cl_mem *buf_use, cl_mem *buf_def);

void cl_bitset_or(unsigned int* bs1, unsigned int* bs2, cl_mem *buf_bs1, cl_mem *buf_bs2);
void cl_bitset_nand(unsigned int* bs1, unsigned int* bs2, cl_mem *buf_bs1, cl_mem *buf_bs2);
void cl_bitset_megaop(unsigned int* in, unsigned int* out, unsigned int* use, unsigned int* def,
                      cl_mem *buf_in, cl_mem *buf_out, cl_mem *buf_use, cl_mem *buf_def);

#ifdef USE_OPENCL
void (*bitset_or_ptr)(unsigned int*, unsigned int*, cl_mem*, cl_mem*) = &cl_bitset_or;
void (*bitset_nand_ptr)(unsigned int*, unsigned int*, cl_mem*, cl_mem*) = &cl_bitset_nand;
void (*bitset_megaop_ptr)(unsigned int*, unsigned int*, unsigned int*, unsigned int*, cl_mem*, cl_mem*, cl_mem*, cl_mem*) = &cl_bitset_megaop;
#else
void (*bitset_or_ptr)(unsigned int*, unsigned int*, cl_mem*, cl_mem*) = &bitset_or;
void (*bitset_nand_ptr)(unsigned int*, unsigned int*, cl_mem*, cl_mem*) = &bitset_and_not;
void (*bitset_megaop_ptr)(unsigned int*, unsigned int*, unsigned int*, unsigned int*, cl_mem*, cl_mem*, cl_mem*, cl_mem*) = &cl_bitset_megaop;
#endif

static double sec(void){
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double) tv.tv_sec + (double)tv.tv_usec / 1000000;
}

typedef struct{
	int index;
	bool listed;
	list_t* pred_list;
	list_t* succ_list;
	unsigned int* in;
	unsigned int* out;
	unsigned int* use;
	unsigned int* def;
	pthread_mutex_t mutex;
    pthread_cond_t cond;
} Vertex;

Vertex* new_vertex(int i){
	Vertex* v = malloc(sizeof(Vertex));
	v->index = i;
	v->listed = false;
	v->pred_list = create_node(NULL); //First element = NULL
	v->succ_list = create_node(NULL); //First element = NULL

	v->in = calloc( alloc_size, sizeof(unsigned int));
	v->out = calloc( alloc_size, sizeof(unsigned int));
	v->use = calloc( alloc_size, sizeof(unsigned int));
	v->def = calloc( alloc_size, sizeof(unsigned int));
    if(v->in == NULL || v->out == NULL || v->use == NULL || v->def == NULL)
        printf("calloc returned null\n");
    pthread_mutex_init(&v->mutex, NULL);
    pthread_cond_init(&v->cond, NULL);
	return v;
}

void acquire_locks(Vertex* u, list_t* succ_list, list_t* pred_list){

    unsigned int acquired_locks = 0;
    unsigned int needed_locks = 1; // We know of Vertex* u.
    unsigned int saved_locks = 0;
    list_t* tmp_list;
    Vertex* v;

    tmp_list = succ_list;
	while(tmp_list->next != tmp_list){
		tmp_list = tmp_list->next;
        ++needed_locks;
	}
    tmp_list = pred_list;
	while(tmp_list->next != tmp_list){
		tmp_list = tmp_list->next;
        ++needed_locks;
	}
    pthread_mutex_t mutexes[needed_locks];
    pthread_cond_t conds[needed_locks];
    mutexes[0] = u->mutex;
    conds[0] = u->cond;
    ++saved_locks;
    

    tmp_list = succ_list;
	while(tmp_list->next != tmp_list){
		tmp_list = tmp_list->next;
        v = tmp_list->data;
        if(v != NULL){
            mutexes[saved_locks] = v->mutex;
            conds[saved_locks] = v->cond;
            ++saved_locks;
        }
	}
    tmp_list = pred_list;
	while(tmp_list->next != tmp_list){
		tmp_list = tmp_list->next;
        v = tmp_list->data;
        if(v != NULL){
            mutexes[saved_locks] = v->mutex;
            conds[saved_locks] = v->cond;
            ++saved_locks;
        }
	}

    // Acquire all or no locks.
    while(1){
        //printf("while(1)\n");
        acquired_locks = 0;
        for(unsigned int i = 0; i < needed_locks; ++i){
            if(pthread_mutex_trylock(&mutexes[i])){
                ++acquired_locks;
                if(acquired_locks == needed_locks){
                    //printf("Acquired all locks.\n");
                    return;
                }
            } else {
                //printf("Failed to acquire lock.\n");
                for(unsigned int j = 0; j < acquired_locks; ++j){
                    //printf("Freeing a lock.\n");
                    pthread_mutex_unlock(&mutexes[j]);
                    pthread_cond_signal(&conds[j]);
                }
                acquired_locks = 0;
                //pthread_cond_wait(&conds[i], &mutexes[i]);
                break; // Break out of for-loop.
            }
        }
    }
}


void unlock_locks(Vertex* u, list_t* succ_list, list_t* pred_list){
    Vertex* v;
    list_t* tmp_list = succ_list;

	while(tmp_list->next != tmp_list){
		tmp_list = tmp_list->next;
        v = tmp_list->data;
        if(v != NULL){
            pthread_mutex_unlock(&v->mutex);
            pthread_cond_signal(&v->cond);
        }
	}
    //printf("Unlocked succ_list.\n");
    tmp_list = pred_list;
	while(tmp_list->next != tmp_list){
		tmp_list = tmp_list->next;
        v = tmp_list->data;
        if(v != NULL){
            pthread_mutex_unlock(&v->mutex);
            pthread_cond_signal(&v->cond);
        }
	}
    //printf("Unlocked pred_list.\n");
    pthread_mutex_unlock(&u->mutex);
    pthread_cond_signal(&u->cond);
    //printf("Unlocked vertex.\n");
}

unsigned int* bitset_copy(unsigned int* bs){
    unsigned int* new_bs = calloc(  alloc_size, sizeof(unsigned int));
    for(unsigned int i = 0; i < (sizeof(unsigned int) * (nsym / (sizeof(unsigned int) * 8) + 1)); ++i){
        new_bs[i] = bs[i];
    }
    return new_bs;
}

bool bitset_equals(unsigned int* bs1, unsigned int* bs2){
    for(unsigned int i = 0; i < (sizeof(unsigned int) * (nsym / (sizeof(unsigned int) * 8) + 1)); ++i){
        if(bs1[i] != bs2[i]){
            return false;
        }
    }
    return true;
}

void bitset_or(unsigned int* bs1, unsigned int* bs2, cl_mem *mem1, cl_mem *mem2){
    for(unsigned int i = 0; i < (sizeof(unsigned int) * (nsym / (sizeof(unsigned int) * 8) + 1)); ++i){
        bs1[i] |= bs2[i];
    }
}

void bitset_and_not(unsigned int* bs1, unsigned int* bs2, cl_mem *mem1, cl_mem *mem2){
    for(unsigned int i = 0; i < (sizeof(unsigned int) * (nsym / (sizeof(unsigned int) * 8) + 1)); ++i){
        unsigned int tmp = bs1[i] & bs2[i];
        tmp = ~tmp;
        bs1[i] = tmp & bs1[i];
    }
}

void bitset_megaop(unsigned int* in, unsigned int* out, unsigned int* use, unsigned int* def,
                      cl_mem *buf_in, cl_mem *buf_out, cl_mem *buf_use, cl_mem *buf_def)
{
    for(unsigned int i = 0; i < (sizeof(unsigned int) * (nsym / (sizeof(unsigned int) * 8) + 1)); ++i){
		in[i] |= out[i];
		in[i]  = in[i] & (~(in[i] & def[i]));
		in[i] |= use[i];
	}
}

void computeIn(Vertex* u, list_t* worklist, cl_mem *buf_or_bs1, cl_mem *buf_or_bs2, cl_mem *buf_nand_bs1, cl_mem *buf_nand_bs2){
	/* silence gcc */
	buf_or_bs1 = buf_or_bs2;
	buf_or_bs2 = buf_nand_bs1;
	buf_nand_bs1 = buf_nand_bs2;
	buf_nand_bs2 = buf_or_bs1;

	//BitSet_struct* old;
	Vertex* v;
    acquire_locks(u, u->succ_list, u->pred_list);

	list_t* tmp_list = u->succ_list;
	do{
        tmp_list = tmp_list->next;
		v = tmp_list->data;
        if(v != NULL){
		    bitset_or(u->out, v->in, buf_or_bs1, buf_or_bs2);
        }
	}while(tmp_list->next != tmp_list);

	unsigned int* old = bitset_copy(u->in);

	bitset_megaop(u->in, u->out, u->use, u->def, &buf_megaop_in, &buf_megaop_out, &buf_megaop_use, &buf_megaop_def);

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
}

void print_vertex(Vertex* v){
	int i;

	printf("use[%d] = { ", v->index);
	for (i = 0; i < nsym; ++i){
        unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
        unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));
		if ((v->use[bit_offset] & (1 << bit_local_index))){//bitset_get_bit(v->use, i)){
			printf("%d ", i);
		}
	}
	printf("}\n");
	printf("def[%d] = { ", v->index);

	for (i = 0; i < nsym; ++i){
        unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
        unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));
		if ((v->def[bit_offset] & (1 << bit_local_index))){
//		if (bitset_get_bit(v->def, i)){
			printf("%d ", i);
		}
	}
	printf("}\n\n");
	printf("in[%d] = { ", v->index);

	for (i = 0; i < nsym; ++i){
        unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
        unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));
		if ((v->in[bit_offset] & (1 << bit_local_index))){
//		if (bitset_get_bit(v->in, i)){
			printf("%d ", i);
		}
	}
	printf("}\n");
	printf("out[%d] = { ", v->index);

	for (i = 0; i < nsym; ++i){
        unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
        unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));
		if ((v->out[bit_offset] & (1 << bit_local_index))){
//		if (bitset_get_bit(v->out, i)){
			printf("%d ", i);
		}
	}
	printf("}\n\n");
}

void connect(Vertex* pred, Vertex* succ){
	add_last(pred->succ_list, create_node(succ));
	add_last(succ->pred_list, create_node(pred));
}

void generateCFG(list_t* vertex_list, int maxsucc, random_t* r){
    //printf("in generateCFG\n");
	int i = 2;
	int j;
	int k;
	int s;
    Vertex* tmp_v;
    Vertex* tmp_s;
	list_t* tmp_list = vertex_list->next;
    list_t* tmp_list_s;

    tmp_v = tmp_list->next->data;
	connect(tmp_list->data, tmp_v);

    tmp_v = tmp_list->next->next->data;
	connect(tmp_list->data, tmp_v);
	tmp_list = tmp_list->next;
	int rand;

	while(tmp_list->next != tmp_list){
		tmp_list = tmp_list->next;
        tmp_v = tmp_list->data; //vertex[i]
		if(print_input){
			printf("[%d] succ = {", i);
		}

		rand = next_rand(r);		
        s = rand % maxsucc +1;
//		printf("rand = %d\n", rand);

		for (j = 0; j < s; ++j) {
			rand = next_rand(r);
			k = abs(rand) % nvertex;
//			printf("rand = %d\n", rand);

			if(print_input){
				printf(" %d", k);
			}
            tmp_list_s = vertex_list->next;
            tmp_s = tmp_list_s->data;
            while(tmp_s->index != k){
                tmp_list_s = tmp_list_s->next;
                tmp_s = tmp_list_s->data;
            }

			connect(tmp_v, tmp_s);
		}
		if(print_input){
			printf("}\n");
		}
		++i;
	}
}

void bitset_set_bit(unsigned int* arr, unsigned int bit){
    unsigned int bit_offset = (bit / (sizeof(unsigned int) * 8));
    unsigned int bit_local_index = (unsigned int) (bit % (sizeof(unsigned int) * 8));
    //printf("bit: %d\tbit_offset: %d \tbit_local_index: %d\n",bit,bit_offset,bit_local_index);
    arr[bit_offset] |= (1 << bit_local_index);
}

bool bitset_get_bit(unsigned int* arr, unsigned int bit){
    unsigned int bit_offset = (bit / (sizeof(unsigned int) * 8));
    unsigned int bit_local_index = (unsigned int) (bit % (sizeof(unsigned int) * 8));
    return (arr[bit_offset]) & (1 << bit_local_index);
}

void generateUseDef(list_t* vertex_list, int nsym, int nactive, random_t* r){
    //printf("in generateUseDef\n");
	int j;
	int sym;
	list_t* tmp_list = vertex_list;
	Vertex* v;

	do{
        tmp_list = tmp_list->next;
		v = tmp_list->data;

		if(print_input){
			printf("[%d] usedef = {", v->index);
		}
		for (j = 0; j < nactive; ++j) {

			int rand = next_rand(r);
            sym = abs(rand) % nsym;

			if (j % 4 != 0) {
				if(!bitset_get_bit(v->def, sym)){//!bitset_get_bit(v->def, sym)){
					if(print_input){
						printf(" u %d", sym);
					}
					bitset_set_bit(v->use, sym);//bitset_set_bit(v->use, sym, true);
				}
			}else{
				if(!bitset_get_bit(v->use, sym)){//!bitset_get_bit(v->use, sym)){
					if(print_input){
						printf(" d %d", sym);
					}
					bitset_set_bit(v->def, sym);//bitset_set_bit(v->def, sym, true);
				}
			}
		}
		if(print_input){
			printf("}\n");
		}
	}while(tmp_list->next != tmp_list);
}


void cl_bitset_or(unsigned int* bs1, unsigned int* bs2, cl_mem *buf_bs1, cl_mem *buf_bs2)
{
	cl_int err;
	cl_event events[3];

    // Write our data set into the input array in device memory
    err  = clEnqueueWriteBuffer(queue, *buf_bs1, CL_FALSE, 0, sizeof(unsigned int) * bitset_subsets, bs1, 0, NULL, &(events[0]));
    err |= clEnqueueWriteBuffer(queue, *buf_bs2, CL_FALSE, 0, sizeof(unsigned int) * bitset_subsets, bs2, 0, NULL, &(events[1]));
    if (err != CL_SUCCESS) {
            printf("Error: Failed to write to GPU memory: %s\n", ocl_error_string(err));
            exit(err);
    }

	err = clWaitForEvents(2, events);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to wait for memory transfers: %s\n", ocl_error_string(err));
            exit(err);
    }

    // Execute the kernel over the entire range of our 1-dim input data set
    // using the maximum number of work group items for this device
	size_t global = bitset_subsets;
    err = clEnqueueNDRangeKernel(queue, or_kernel, 1, NULL, &global, &cl_nand_local_workgroup_size, 0, NULL, &(events[2]));
    if (err != CL_SUCCESS) {
            printf("Error: Failed to execute kernel: %s\n", ocl_error_string(err));
            exit(err);
    }

	err = clWaitForEvents(1, &events[2]);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to wait for memory transfers: %s\n", ocl_error_string(err));
            exit(err);
    }

    // Read back the results from the device to verify the output
    err  = clEnqueueReadBuffer(queue, *buf_bs1, CL_TRUE, 0, nvertex * bitset_subsets, bs1, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to read output array: %s\n", ocl_error_string(err));
            exit(1);
    }
}


void cl_bitset_nand(unsigned int* bs1, unsigned int* bs2, cl_mem *buf_bs1, cl_mem *buf_bs2)
{
	cl_int err;
	cl_event events[3];

    // Write our data set into the input array in device memory
    err  = clEnqueueWriteBuffer(queue, *buf_bs1, CL_FALSE, 0, sizeof(unsigned int) * bitset_subsets, bs1, 0, NULL, &(events[0]));
    err |= clEnqueueWriteBuffer(queue, *buf_bs2, CL_FALSE, 0, sizeof(unsigned int) * bitset_subsets, bs2, 0, NULL, &(events[1]));
    if (err != CL_SUCCESS) {
            printf("Error: Failed to write to GPU memory: %s\n", ocl_error_string(err));
            exit(err);
    }

	err = clWaitForEvents(2, events);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to wait for memory transfers: %s\n", ocl_error_string(err));
            exit(err);
    }

    // Execute the kernel over the entire range of our 1-dim input data set
    // using the maximum number of work group items for this device
	size_t global = bitset_subsets;
    err = clEnqueueNDRangeKernel(queue, nand_kernel, 1, NULL, &global, &cl_nand_local_workgroup_size, 0, NULL, &(events[2]));
    if (err != CL_SUCCESS) {
            printf("Error: Failed to execute kernel: %s\n", ocl_error_string(err));
            exit(err);
    }

	err = clWaitForEvents(1, &events[2]);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to wait for memory transfers: %s\n", ocl_error_string(err));
            exit(err);
    }

    // Read back the results from the device to verify the output
    err  = clEnqueueReadBuffer(queue, *buf_bs1, CL_TRUE, 0, nvertex * bitset_subsets, bs1, 0, NULL, &(events[1]));
    if (err != CL_SUCCESS) {
            printf("Error: Failed to read output array: %s\n", ocl_error_string(err));
            exit(1);
    }
}

void cl_bitset_megaop(unsigned int* in, unsigned int* out, unsigned int* use, unsigned int* def,
                      cl_mem *buf_in, cl_mem *buf_out, cl_mem *buf_use, cl_mem *buf_def)
{
	cl_int err;
	cl_event events[3];

    // Write our data set into the input array in device memory
    //err  = clEnqueueWriteBuffer(queue, *in, CL_FALSE, 0, sizeof(unsigned int) * bitset_subsets, buf_in, 0, NULL, &(events[0]));
    err  = clEnqueueWriteBuffer(queue, *buf_out, CL_FALSE, 0, sizeof(unsigned int) * bitset_subsets, out, 0, NULL, &(events[0]));
    err |= clEnqueueWriteBuffer(queue, *buf_use, CL_FALSE, 0, sizeof(unsigned int) * bitset_subsets, use, 0, NULL, &(events[1]));
    err |= clEnqueueWriteBuffer(queue, *buf_def, CL_FALSE, 0, sizeof(unsigned int) * bitset_subsets, def, 0, NULL, &(events[2]));
    if (err != CL_SUCCESS) {
            printf("Error: Failed to write to GPU memory: %s\n", ocl_error_string(err));
            exit(err);
    }

	err = clWaitForEvents(3, events);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to wait for memory transfers: %s\n", ocl_error_string(err));
            exit(err);
    }


    // Execute the kernel over the entire range of our 1-dim input data set
    // using the maximum number of work group items for this device
	size_t global = bitset_subsets;
    err = clEnqueueNDRangeKernel(queue, nand_kernel, 1, NULL, &global, &cl_nand_local_workgroup_size, 0, NULL, &(events[0]));
    if (err != CL_SUCCESS) {
            printf("Error: Failed to execute kernel: %s\n", ocl_error_string(err));
            exit(err);
    }

	err = clWaitForEvents(1, &events[0]);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to wait for memory transfers: %s\n", ocl_error_string(err));
            exit(err);
    }

    // Read back the results from the device to verify the output
    err  = clEnqueueReadBuffer(queue, *buf_in, CL_TRUE, 0, nvertex * bitset_subsets, in, 0, NULL, &(events[1]));
    if (err != CL_SUCCESS) {
            printf("Error: Failed to read output array: %s\n", ocl_error_string(err));
            exit(1);
    }
}

typedef struct thread_struct {
    unsigned int index;
    list_t* worklist;
} thread_struct;

void* thread_func(void* ts){
    list_t* worklist = ((thread_struct*) ts)->worklist;
    unsigned int index = ((thread_struct*) ts)->index;

	Vertex* u;
    unsigned int work_counter = 0;
	while(worklist->next != worklist){ // while (!worklist.isEmpty())
        u = remove_node(worklist->next);
        //printf("u->index = %d\n", u->index);
		u->listed = false;
		computeIn(u, worklist, &buf_or_bs1, &buf_or_bs2, &buf_nand_bs1, &buf_nand_bs2);
        work_counter++;
	}
    printf("Thread[%u] worked %u iterations.\n", index, work_counter);
    return NULL;
}

void liveness(list_t* vertex_list, int nthread){
    //printf("in liveness\n");
	Vertex* v;

//    int status[nthread];
 	pthread_t thread[nthread];
	list_t* worklist[nthread];
    int worksplit[nthread];

	double begin;
	double end;

	begin = sec();

    for(int i = 0; i < nthread; ++i){
        worksplit[i] = 0;
        worklist[i] = create_node(NULL);
    }


	list_t* tmp_list = vertex_list;//->next;
    int counter = 0;
	while(tmp_list->next != tmp_list){
        tmp_list = tmp_list->next;
		v = tmp_list->data;
		v->listed = true;
        int index = (counter++) % nthread;
        worksplit[index]++;
		add_last(worklist[index], create_node(v));
	}
/*    for(int i = 0; i < nthread; ++i){
        printf("worksplit[%d] = %d\n", i, worksplit[i]);
    }
*/
    for(int i = 0; i < nthread; ++i){
        thread_struct* ts = malloc(sizeof(thread_struct));
        ts->index = i;
        //printf("i = %d\n", i);
        ts->worklist = worklist[i];
        pthread_create(&thread[i], NULL, thread_func, ts);
    }

    for(int i = 0; i < nthread; ++i){
        pthread_join(thread[i], NULL);
    }
	end = sec();
#ifdef USE_OPENCL
	printf("OpenCL runtime: %f s\n", (end - begin));
#else
	printf("CPU runtime: %f s\n", (end - begin));
#endif
}

int main(int ac, char** av){

	int	i;
	nsym = 10000;
	nvertex = 1000;
	int maxsucc = 6;
	int nactive = 10;
	int	nthread = 1;
	bool print_output = false;
	print_input = false; 
	list_t* vertex;
	random_t* r = new_random();
    list_t* tmp_list;

	set_seed(r, 1);
	vertex = create_node(NULL); //First element = NULL

#ifdef USE_OPENCL
	printf("OpenCL: ENABLED\n");
#else
	printf("OpenCL: DISABLED\n");
#endif

	//
	// Parse input parameters
	if(ac != 7){
	    printf("\nWrong # of args (nsym nvertex maxsucc nactive print_output print_input).\nAssuming sane defaults.\n\n");
	} else {
		char *tmp_string;
		sscanf(av[1], "%d", &nsym);
		sscanf(av[2], "%d", &nvertex);
		sscanf(av[3], "%d", &maxsucc);
		sscanf(av[4], "%d", &nactive);

		tmp_string = av[5];
		if(tolower(tmp_string[0]) == 't'){
			print_output = true;
		}else{
			print_output = false;
		}

		tmp_string = av[6];
		if(tolower(tmp_string[0]) == 't'){
			print_input = true;
		}else{
			print_input = false;
		}
	}
    alloc_size = 50*(nsym / (sizeof(unsigned int) * 8)) + 1;
	bitset_subsets = sizeof(unsigned int) * (nsym / (sizeof(unsigned int) * 8) + 1);
	printf("Subsets per bitset: %u\n", bitset_subsets);

	//
	// Setup OpenCL
	cl_int err;
	setup_queue(&device_id, &context, &queue);
	setup_kernel("bitset.cl", "bitset_or", &device_id, &context, &or_kernel);
	setup_kernel("bitset.cl", "bitset_nand", &device_id, &context, &nand_kernel);
	setup_kernel("bitset.cl", "bitset_megaop", &device_id, &context, &megaop_kernel);
    // Set the arguments to nand compute kernel
	err  = clSetKernelArg(nand_kernel, 0, sizeof(cl_mem), buf_nand_bs1);
	err |= clSetKernelArg(nand_kernel, 1, sizeof(cl_mem), buf_nand_bs2);
	err |= clSetKernelArg(nand_kernel, 2,  sizeof(unsigned int), &bitset_subsets);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to set nand-kernel arguments: %s\n", ocl_error_string(err));
            exit(err);
    }
    // Set the arguments to megaop compute kernel
	err  = clSetKernelArg(megaop_kernel, 0, sizeof(cl_mem), buf_megaop_in);
	err |= clSetKernelArg(megaop_kernel, 1, sizeof(cl_mem), buf_megaop_out);
	err |= clSetKernelArg(megaop_kernel, 2, sizeof(cl_mem), buf_megaop_use);
	err |= clSetKernelArg(megaop_kernel, 3, sizeof(cl_mem), buf_megaop_def);
	err |= clSetKernelArg(megaop_kernel, 4,  sizeof(unsigned int), &bitset_subsets);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to set megaop-kernel arguments: %s\n", ocl_error_string(err));
            exit(err);
    }
    // Set the arguments to or compute kernel
	err  = clSetKernelArg(or_kernel, 0, sizeof(cl_mem), buf_or_bs1);
	err |= clSetKernelArg(or_kernel, 1, sizeof(cl_mem), buf_or_bs2);
	err |= clSetKernelArg(or_kernel, 2,  sizeof(unsigned int), &bitset_subsets);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to set or-kernel arguments: %s\n", ocl_error_string(err));
            exit(err);
    }
    // Get the maximum work group size for executing the kernel on the device
    err  = clGetKernelWorkGroupInfo(or_kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &cl_or_local_workgroup_size, NULL);
    err |= clGetKernelWorkGroupInfo(nand_kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &cl_nand_local_workgroup_size, NULL);
    err |= clGetKernelWorkGroupInfo(megaop_kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &cl_megaop_local_workgroup_size, NULL);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to retrieve kernel work group info: %s\n", ocl_error_string(err));
            exit(1);
    }
    buf_or_bs1 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int) * bitset_subsets, NULL, &err);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to allocate device buf_or_bs1 memory: %s\n", ocl_error_string(err));
            exit(1);
    }
    buf_or_bs2 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int) * bitset_subsets, NULL, &err);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to allocate device buf_or_bs2 memory: %s\n", ocl_error_string(err));
            exit(1);
    }
    buf_nand_bs1 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int) * bitset_subsets, NULL, &err);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to allocate device buf_nand_bs1 memory: %s\n", ocl_error_string(err));
            exit(1);
    }
    buf_nand_bs2 = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int) * bitset_subsets, NULL, &err);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to allocate device buf_nand_bs2 memory: %s\n", ocl_error_string(err));
            exit(1);
    }
    buf_megaop_in = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(unsigned int) * bitset_subsets, NULL, &err);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to allocate device buf_megaop_in memory: %s\n", ocl_error_string(err));
            exit(1);
    }
    buf_megaop_out = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(unsigned int) * bitset_subsets, NULL, &err);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to allocate device buf_megaop_out memory: %s\n", ocl_error_string(err));
            exit(1);
    }
    buf_megaop_use = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(unsigned int) * bitset_subsets, NULL, &err);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to allocate device buf_megaop_use memory: %s\n", ocl_error_string(err));
            exit(1);
    }
    buf_megaop_def = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(unsigned int) * bitset_subsets, NULL, &err);
    if (err != CL_SUCCESS) {
            printf("Error: Failed to allocate device buf_megaop_def memory: %s\n", ocl_error_string(err));
            exit(1);
    }


	//
	// Create vertices
    tmp_list = vertex;
	for (i = 0; i < nvertex; ++i){
        insert_after(tmp_list, create_node(new_vertex(i)));
		tmp_list = tmp_list->next;
	}
	generateCFG(vertex, maxsucc, r);
	generateUseDef(vertex, nsym, nactive, r);

	//
	// Compute liveness
	liveness(vertex, nthread);
	
	//
	// Output result
	if(print_output){
        tmp_list = vertex->next;

		for (i = 0; i < nvertex; ++i){
			print_vertex(tmp_list->data);
			tmp_list = tmp_list->next;
		}
	}

	return 0;
}
