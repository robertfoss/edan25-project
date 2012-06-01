#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "cll.h"
#include "util.h"
#include "random.cpp"

struct timeval start, end;
long mtime, seconds, useconds; 
unsigned int bitset_size;
int nsym;
int maxsucc;
int maxpred;
int* pred_list;
int* succ_list;
unsigned int* in;
unsigned int* out;
unsigned int* use;
unsigned int* def;
int nvertex;

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


void print_bs(unsigned int* bs);


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
   // printf("printing tmp \t");
    /*printf("__");
    print_bs(new_bs);
    printf("__");*/
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


void print_vertex(vertex_t* v){
	int i;
    int index = v->index;

	printf("use[%d] = { ", index);
	for (i = 0; i < nsym; ++i){
        unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
        unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));

        if(use[index * bitset_size + bit_offset] & (1 << bit_local_index)){
		//if ((v->use[bit_offset] & (1 << bit_local_index))){
			printf("%d ", i);
		}
	}
	printf("}\n");
	printf("def[%d] = { ", index);

	for (i = 0; i < nsym; ++i){
        unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
        unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));
        if(def[index * bitset_size + bit_offset] & (1 << bit_local_index)){
		//if ((v->def[bit_offset] & (1 << bit_local_index))){
			printf("%d ", i);
		}
	}
	printf("}\n\n");
	printf("in[%d] = { ", index);

	for (i = 0; i < nsym; ++i){
        unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
        unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));
        if(in[index * bitset_size + bit_offset] & (1 << bit_local_index)){
		//if ((v->in[bit_offset] & (1 << bit_local_index))){
			printf("%d ", i);
		}
	}
	printf("}\n");
	printf("out[%d] = { ", index);

	for (i = 0; i < nsym; ++i){
        unsigned int bit_offset = (i / (sizeof(unsigned int) * 8));
        unsigned int bit_local_index = (unsigned int) (i % (sizeof(unsigned int) * 8));
        if(out[index * bitset_size + bit_offset] & (1 << bit_local_index)){
		//if ((v->out[bit_offset] & (1 << bit_local_index))){
			printf("%d ", i);
		}
	}
	printf("}\n\n");
}


void print_cfg(vertex_t* vertices){
    printf("print_cfg:\n");
    for(int i = 0; i < nvertex; ++i){
        printf("%d: %d succ[%d] = {", i, vertices[i].succ_count, vertices[i].index);
        for(unsigned int j = 0; j < vertices[i].succ_count; ++j){
            printf(" %d", succ_list[i * maxsucc + j]);
        }
        printf("}\n");
    }
}


void connect(vertex_t* pred, vertex_t* succ){

    int pred_i = pred->index;
    int succ_i = succ->index;
    //printf("\npred_i = %d, pred->succ_count = %d\tsucc_i = %d, succ->pred_count = %d\n", pred_i, pred->succ_count, succ_i, succ->pred_count);

   // printf(" setting succ_list[%d * %d + %d = %d] = %d\n", pred_i, maxsucc, pred->succ_count, pred_i * maxsucc + pred->succ_count, succ_i);

    succ_list[pred_i * maxsucc + pred->succ_count] = succ_i;
    pred_list[succ_i * maxpred + succ->pred_count] = pred_i;

    pred->succ_count++;
    succ->pred_count++;
}


void generateCFG(vertex_t* vertex_list, int nvertex, Random* r, int print_input){
    printf("generateCfg\n");


    pred_list = (int*) malloc((sizeof(int) * nvertex) * nvertex);
    succ_list = (int*) malloc((sizeof(int) * maxsucc) * nvertex);
    in = (unsigned int*) calloc(nvertex * bitset_size, sizeof(unsigned int));
    out = (unsigned int*) calloc(nvertex * bitset_size, sizeof(unsigned int));
    use = (unsigned int*) calloc(nvertex * bitset_size, sizeof(unsigned int));
    def = (unsigned int*) calloc(nvertex * bitset_size, sizeof(unsigned int));


    if(pred_list == NULL){
        printf("pred_list == NULL\n");
    }
    if(succ_list == NULL){
        printf("succ_list == NULL\n");
    }
    if(in == NULL){
        printf("in == NULL\n");
    }
    if(out == NULL){
        printf("out == NULL\n");
    }
    if(use == NULL){
        printf("use == NULL\n");
    }
    if(def == NULL){
        printf("def == NULL\n");
    }

    int s;
    int k;

	connect(&vertex_list[0], &vertex_list[1]);
	connect(&vertex_list[0], &vertex_list[2]);
    int rand;
    for(int i = 2; i < nvertex; ++i){
        if(print_input){
			printf("[%d] succ = {", i);
		}
        rand = r->nextInt();
        s = rand % maxsucc + 1;
        //printf("rand = %d\n", rand);
        for(int j = 0; j < s; ++j){
            rand = r->nextInt();
            k = abs(rand) % nvertex;
            //printf("rand = %d\n", rand);
            if(print_input){
				printf(" %d", k);
			}
            connect(&vertex_list[i], &vertex_list[k]);
        }
        if(print_input){
			printf("}\n");
		}
    }

    //print_cfg(vertex_list);

}


void print_bs_set(unsigned int* bs){
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
}


void generateUseDef(vertex_t* vertex_list, int nvertex, int nactive, Random* r, int print_input){
    printf("generateUseDef\n");
    int sym;

    for(int i = 0; i < nvertex; ++i){
        if(print_input){
			printf("[%d] usedef = {", vertex_list[i].index);
		}

        for(int j = 0; j < nactive; ++j){
            int rand = r->nextInt();
            sym = abs(rand) % nsym;
            //printf("sym = %d\t rand = %d\t abs(rand) = %d\n", sym, rand, abs(rand));

            if(j % 4 != 0){
                if(!bitset_get_bit(def, i, sym)){

					if(print_input){
						printf(" u %d", sym);
					}
                    bitset_set_bit(use, i, sym);
				}
			}else{
				if(!bitset_get_bit(use, i, sym)){
					if(print_input){
						printf(" d %d", sym);
					}
                    bitset_set_bit(def, i, sym);
				}

            }
        }
        if(print_input){
			printf("}\n");
		}
    }

    /*printf("printing use:\t");
    print_bs_set(use);
    printf("printing def:\t");
    print_bs_set(def);*/
}


vertex_t* create_vertices(int nactive, int nvertex2, int print_input){
    printf("create_verices\n");
    vertex_t* vertices;

    //nsym = nsym2;
    //maxsucc = maxsucc2;
    //maxpred = nvertex;
    nvertex = nvertex2;

    vertices = (vertex_t*) malloc(sizeof(vertex_t)*nvertex);
    bitset_size = 50*(nsym / (sizeof(unsigned int) * 8)) + 1;
    printf("bitset_size = %d\n", bitset_size);
    Random r(1);

    for(int i = 0; i < nvertex; ++i){
        vertices[i].index = i;
        vertices[i].listed = 0;
        vertices[i].pred_count = 0;
        vertices[i].succ_count = 0;
    }

	generateCFG(vertices, nvertex, &r, print_input);
	generateUseDef(vertices, nvertex, nactive, &r, print_input);

	return vertices;

}


void CL::popCorn()
{
    printf("Popping the kernel..\n");

    //initialize our kernel from the program
    //kernel = clCreateKernel(program, "liveness", &err);
    //printf("clCreateKernel: %s\n", oclErrorString(err));
    try{
        kernel = cl::Kernel(program, "liveness", &err);
    }
    catch (cl::Error er) {
        printf("ERROR: %s(%d)\n", er.what(), er.err());
    }

       

    //initialize our CPU memory arrays, send them to the device and set the kernel arguements
    num = 600000;
    uint_2 *x = new uint_2[num];
    unsigned int *c = new unsigned int[num];
    
    Random r(17);
    for(int i=0; i < num; i++)
    {
        x[i].a = r.nextInt();
        x[i].b = r.nextInt();
        x[i].semaphor = 0; // By default semaphores are released.
        c[i] = 0;
    }
    
/// Robertcode
    struct timeval start_cpu, end_cpu;
	long mtime_cpu, seconds_cpu, useconds_cpu;
	gettimeofday(&start_cpu, NULL);
    for(int i=0; i < num; i++){
    	c[i] = gcd(x[i].a,x[i].b);
    }
    gettimeofday(&end_cpu, NULL);
    seconds_cpu  = end_cpu.tv_sec  - start_cpu.tv_sec;
    useconds_cpu = end_cpu.tv_usec - start_cpu.tv_usec;
	mtime_cpu = ((seconds_cpu) * 1000 + useconds_cpu/1000.0) + 0.5;
	printf("CPU runtime: %ldms\n", mtime_cpu);
/// /Robertcdoe

    

    printf("Creating OpenCL arrays\n");
    size_t array_size = sizeof(unsigned int) * num;
    //our input arrays
    ///cl_a = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * num, NULL, &err);
    ///cl_b = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * num, NULL, &err);
    cl_a = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(uint_2) * num, NULL, &err);
//    cl_b = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(unsigned int) * num, NULL, &err);
    //our output array
    ///cl_c = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * num, NULL, &err);
    cl_c = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(unsigned int) * num, NULL, &err);

    printf("Pushing data to the GPU..\n");

/// Robertcode 
    gettimeofday(&start, NULL);
/// /Robertcode 

    double begin;
	double end;
    int nvertex;
	int nactive;
    int print_input, print_output;

    nsym = 100;
    nvertex = 8;
    maxsucc = 4;
    maxpred = nvertex;
    nactive = 10;
    print_output = 0;
    print_input = 1;

    begin = sec();//in liveness
    end = sec();

    vertex_t* vertices = create_vertices(nactive, nvertex, print_input);
    printf("vertices created\n");

    //push our CPU arrays to the GPU
    ///err = clEnqueueWriteBuffer(command_queue, cl_a, CL_TRUE, 0, sizeof(float) * num, a, 0, NULL, &event);
    err = queue.enqueueWriteBuffer(cl_a, CL_TRUE, 0, sizeof(uint_2) * num, x, NULL, &event);

    printf("wut\n");

    ///clReleaseEvent(event); //we need to release events in order to be completely clean (has to do with openclprof)
    ///err = clEnqueueWriteBuffer(command_queue, cl_b, CL_TRUE, 0, sizeof(float) * num, b, 0, NULL, &event);
//    err = queue.enqueueWriteBuffer(cl_b, CL_TRUE, 0, array_size, b, NULL, &event);
    ///clReleaseEvent(event);
    ///err = clEnqueueWriteBuffer(command_queue, cl_c, CL_TRUE, 0, sizeof(float) * num, c, 0, NULL, &event);
    err = queue.enqueueWriteBuffer(cl_c, CL_TRUE, 0, sizeof(unsigned int) * num, c, NULL, &event);
    ///clReleaseEvent(event);
    

    //set the arguements of our kernel
    ///err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *) &cl_a);
    ///err  = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *) &cl_b);
    ///err  = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *) &cl_c);
    err = kernel.setArg(0, cl_a);
//    err = kernel.setArg(1, cl_b);
    err = kernel.setArg(1, cl_c);
    //Wait for the command queue to finish these commands before proceeding
    ///clFinish(command_queue);
    

    queue.finish();


    //for now we make the workgroup size the same as the number of elements in our arrays
    //workGroupSize[0] = num;
    delete x;
//    delete b;
    delete c;
}



void CL::runKernel()
{
    printf("Running kernel..\n");

    //execute the kernel
    ///err = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, workGroupSize, NULL, 0, NULL, &event);
    printf("Hello 0 \n");
    err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(num), cl::NullRange, NULL, &event); 
    printf("Hello 1\n");
    ///clReleaseEvent(event);
//r    printf("clEnqueueNDRangeKernel: %s\n", oclErrorString(err));
    ///clFinish(command_queue);
    queue.finish();
    //lets check our calculations by reading from the device memory and printing out the results
    unsigned int c_done[num];
    ///err = clEnqueueReadBuffer(command_queue, cl_c, CL_TRUE, 0, sizeof(float) * num, c_done, 0, NULL, &event);
    err = queue.enqueueReadBuffer(cl_c, CL_TRUE, 0, sizeof(unsigned int) * num, &c_done, NULL, &event);
//r    printf("clEnqueueReadBuffer: %s\n", oclErrorString(err));
    //clReleaseEvent(event);

/// Robertcode     
    gettimeofday(&end, NULL);
    seconds  = end.tv_sec  - start.tv_sec;
    useconds = end.tv_usec - start.tv_usec;
	mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
	printf("OpenCL runtime: %ldms\n", mtime);
/// /Robertcode 

/*	Random r(17);
    for(int i=0; i < num; i++)
    {
        printf("gcd(%u,%u) = c_done[%u] = %d\n", r.nextInt(), r.nextInt(), i, c_done[i]);
    }
*/}


