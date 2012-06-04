#include <stdio.h>
#include <ctype.h>

#include "CL/cl.h"

#include "opencl.h"
#include "util.h"
#include "vertex.h"

#define DATA_SIZE (1024)

int main(int argc, char **argv)
{
        cl_int err;							// error code returned from api calls
        size_t global;						// global domain size for our calculation
        size_t local;						// local domain size for our calculation
        cl_device_id device_id;				// device id running computation
        cl_context context;					// compute context
        cl_command_queue queue;				// compute command queue
        cl_kernel kernel;					// compute kernel

        cl_mem tmp_input;					// device memory used for the input array
        cl_mem tmp_output;					// device memory used for the output array

        double data[DATA_SIZE];				// original data set given to device
        double results[DATA_SIZE];			// results returned from device
        unsigned int correct;				// number of correct results returned

		// Variables representing properties of the desired CFG
		int nsym=100, nvertex=8, maxsucc=4, nactive=10, print_output=1, print_input=1;
        char* tmp_string = "";
        unsigned int bitset_size;

		vertex_t *vertices=NULL;
		bitset_t *in=NULL, *out=NULL, *use=NULL, *def=NULL;
		unsigned int *pred_list=NULL, *succ_list=NULL;
		
		if(argc != 7){
		    printf("Wrong # of args (nsym nvertex maxsucc nactive nthreads print_output print_input).\nAssuming sane defaults.\n\n");
		} else {
		    sscanf(argv[1], "%d", &nsym);
		    sscanf(argv[2], "%d", &nvertex);
		    sscanf(argv[3], "%d", &maxsucc);
		    sscanf(argv[4], "%d", &nactive);

		    tmp_string = argv[5];
		    if(tolower(tmp_string[0]) == 't') {
		            print_output = 1;
		    } else {
		            print_output = 0;
		    }

		    tmp_string = argv[6];
		    if(tolower(tmp_string[0]) == 't') {
		            print_input = 1;
		    } else {
		            print_input = 0;
		    }
		}

        create_vertices(nsym, nvertex, maxsucc, nactive, print_input, &vertices, &bitset_size,
                        &pred_list, &succ_list, &in, &out, &use, &def);
		printf("nsym: %d\nnvertex: %d\nmaxsucc: %d\nnactive: %d\nprint_input: %d\nvertices: %p\nbitset_size: %u\npred_list: %p\n"
               "succ_list: %p\nin: %p\nout: %p\nuse: %p\ndef: %p\n", 
               nsym, nvertex, maxsucc, nactive, print_input, vertices, bitset_size, pred_list, succ_list, in, out, use, def);

        // Fill our data set with random unsigned int values
        int i = 0;
        unsigned int count = DATA_SIZE;
        for(i = 0; i < count; i++)
                data[i] = (double) (rand() / (double)RAND_MAX);

        char build_options[50];
        sprintf(build_options, "-D BUFF_SIZE=%u", bitset_size);
        setup_opencl("liveness.cl", "liveness", build_options, &device_id, &kernel, &context, &queue);


        // Get the maximum work group size for executing the kernel on the device
        err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to retrieve kernel work group info: %s\n", ocl_error_string(err));
                exit(1);
        }

        // Create the input and output arrays in device memory for our calculation
        tmp_input = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(double) * count, NULL, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device READ memory: %s\n", ocl_error_string(err));
                exit(1);
        }
        tmp_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(double) * count, NULL, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device WRITE memory: %s\n", ocl_error_string(err));
                exit(1);
        }

        // Write our data set into the input array in device memory
        err = clEnqueueWriteBuffer(queue, tmp_input, CL_TRUE, 0, sizeof(double) * count, data, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to write to source array: %s\n", ocl_error_string(err));
                exit(1);
        }

        // Set the arguments to our compute kernel
        err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &tmp_input);
        err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &tmp_output);
        err |= clSetKernelArg(kernel, 2, sizeof(unsigned int), &count);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to set kernel arguments: %s\n", ocl_error_string(err));
                exit(1);
        }

        // Execute the kernel over the entire range of our 1d input data set
        // using the maximum number of work group items for this device
        global = count;
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to execute kernel: %s\n", ocl_error_string(err));
                return EXIT_FAILURE;
        }

        // Wait for the command commands to get serviced before reading back results
        clFinish(queue);

        // Read back the results from the device to verify the output
        err = clEnqueueReadBuffer(queue, tmp_output, CL_TRUE, 0, sizeof(double) * count, results, 0, NULL, NULL );
        if (err != CL_SUCCESS) {
                printf("Error: Failed to read output array: %s\n", ocl_error_string(err));
                exit(1);
        }

        // Validate our results
        correct = 0;
        for(i = 0; i < count; i++) {
                if(results[i] == data[i] * data[i])
                        correct++;
                else
                        printf("[%d]: %f^2 == %f, != %f\n", i, data[i], data[i] * data[i], results[i]);
        }

        // Print a brief summary detailing the results
        printf("Computed '%d/%d' correct values!\n", correct, count);
		if (print_output) print_vertices(nvertex, maxsucc, vertices, pred_list, succ_list);
}

