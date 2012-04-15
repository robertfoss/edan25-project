#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "cll.h"
#include "util.h"
#include "random.cpp"

struct timeval start, end;
long mtime, seconds, useconds; 

struct uint_2{
	unsigned int a;
	unsigned int b;
	int semaphor;
};
typedef struct uint_2 uint_2;

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

    //push our CPU arrays to the GPU
    ///err = clEnqueueWriteBuffer(command_queue, cl_a, CL_TRUE, 0, sizeof(float) * num, a, 0, NULL, &event);
    err = queue.enqueueWriteBuffer(cl_a, CL_TRUE, 0, sizeof(uint_2) * num, x, NULL, &event);
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
    err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(num), cl::NullRange, NULL, &event); 
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


