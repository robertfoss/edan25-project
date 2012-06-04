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

		cl_mem buf_vertices, buf_pred_list, buf_succ_list, buf_in, buf_out, buf_use, buf_def, buf_bitset_alloc;


		// Variables representing properties of the desired CFG
		int nsym=100, nvertex=8, maxsucc=4, nactive=10, print_output=1, print_input=1, maxpred=8;
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
			maxpred = nvertex;
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
		// Print results of create_vertices().s
		//printf("nsym: %d\nnvertex: %d\nmaxsucc: %d\nnactive: %d\nprint_input: %d\nvertices: %p\nbitset_size: %u\npred_list: %p\n"
        //       "succ_list: %p\nin: %p\nout: %p\nuse: %p\ndef: %p\n", 
        //       nsym, nvertex, maxsucc, nactive, print_input, vertices, bitset_size, pred_list, succ_list, in, out, use, def);


        char build_options[50] = "";
        //sprintf(build_options, "-D BUFF_SIZE=%u", bitset_size); //TODO: REMOVE WITH BUFF_SIZE
        setup_opencl("liveness.cl", "liveness", build_options, &device_id, &kernel, &context, &queue);


		// TODO: SWITCH TO CL_MEM_READ_ONLY AND CL_MEM_WRITE_ONLY WHERE APPROPRIATE
        // Create the input and output arrays in device memory for our calculation
        buf_vertices = clCreateBuffer(context,  CL_MEM_READ_WRITE,  sizeof(vertex_t) * nvertex, vertices, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device buf_vertices memory: %s\n", ocl_error_string(err));
                exit(1);
        }
		// TODO: Due to O(nvertex^2) memory complexity for pred_list (succ_list not being great either)
		// consider using CL_MEM_USE_HOST_PTR to prevent huge amounts of GPU memory being needed
        buf_pred_list = clCreateBuffer(context, (CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE), sizeof(int) * nvertex * nvertex, pred_list, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device buf_pred_list memory: %s\n", ocl_error_string(err));
                exit(1);
        }
        buf_succ_list = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int) * maxsucc * nvertex, succ_list, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device buf_succ_list memory: %s\n", ocl_error_string(err));
                exit(1);
        }
        buf_in = clCreateBuffer(context, CL_MEM_READ_WRITE, nvertex * bitset_size, in, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device buf_in memory: %s\n", ocl_error_string(err));
                exit(1);
        }
        buf_out = clCreateBuffer(context, CL_MEM_READ_WRITE, nvertex * bitset_size, out, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device buf_out memory: %s\n", ocl_error_string(err));
                exit(1);
        }
        buf_use = clCreateBuffer(context, CL_MEM_READ_WRITE, nvertex * bitset_size, use, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device buf_use memory: %s\n", ocl_error_string(err));
                exit(1);
        }
        buf_def = clCreateBuffer(context, CL_MEM_READ_WRITE, nvertex * bitset_size, def, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device buf_def memory: %s\n", ocl_error_string(err));
                exit(1);
        }
        buf_def = clCreateBuffer(context, CL_MEM_READ_WRITE, nvertex * bitset_size, NULL, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device buf_def memory: %s\n", ocl_error_string(err));
                exit(1);
        }
        buf_bitset_alloc = clCreateBuffer(context, (CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_WRITE), nvertex * bitset_size, NULL, &err);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to allocate device buf_def memory: %s\n", ocl_error_string(err));
                exit(1);
        }

        // Write our data set into the input array in device memory
        err  = clEnqueueWriteBuffer(queue, buf_vertices, CL_TRUE, 0, sizeof(vertex_t) * nvertex, vertices, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(queue, buf_pred_list, CL_TRUE, 0, sizeof(int) * nvertex * nvertex, pred_list, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(queue, buf_succ_list, CL_TRUE, 0, sizeof(int) * maxsucc * nvertex, succ_list, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(queue, buf_in, CL_TRUE, 0, nvertex * bitset_size, in, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(queue, buf_out, CL_TRUE, 0, nvertex * bitset_size, out, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(queue, buf_use, CL_TRUE, 0, nvertex * bitset_size, use, 0, NULL, NULL);
        err |= clEnqueueWriteBuffer(queue, buf_def, CL_TRUE, 0, nvertex * bitset_size, def, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to write to GPU memory: %s\n", ocl_error_string(err));
                exit(1);
        }

        // Set the arguments to our compute kernel
		err  = clSetKernelArg(kernel, 0,  sizeof(unsigned int), &nvertex);
		err |= clSetKernelArg(kernel, 1,  sizeof(unsigned int), &maxpred);
		err |= clSetKernelArg(kernel, 2,  sizeof(unsigned int), &maxsucc);
		err |= clSetKernelArg(kernel, 3,  sizeof(unsigned int), &bitset_size);
		err |= clSetKernelArg(kernel, 4,  sizeof(cl_mem), &buf_vertices);
		err |= clSetKernelArg(kernel, 5,  sizeof(cl_mem), &buf_pred_list);
		err |= clSetKernelArg(kernel, 6,  sizeof(cl_mem), &buf_succ_list);
		err |= clSetKernelArg(kernel, 7,  sizeof(cl_mem), &buf_in);
		err |= clSetKernelArg(kernel, 8,  sizeof(cl_mem), &buf_out);
		err |= clSetKernelArg(kernel, 9,  sizeof(cl_mem), &buf_use);
		err |= clSetKernelArg(kernel, 10, sizeof(cl_mem), &buf_def);
		err |= clSetKernelArg(kernel, 11, sizeof(cl_mem), &buf_bitset_alloc);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to set kernel arguments: %s\n", ocl_error_string(err));
                exit(1);
        }

        // Get the maximum work group size for executing the kernel on the device
        err = clGetKernelWorkGroupInfo(kernel, device_id, CL_KERNEL_WORK_GROUP_SIZE, sizeof(local), &local, NULL);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to retrieve kernel work group info: %s\n", ocl_error_string(err));
                exit(1);
        }

        // Execute the kernel over the entire range of our 1d input data set
        // using the maximum number of work group items for this device
        global = nvertex;
		global = 0; // This can't be the right way?
        err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global, &local, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to execute kernel: %s\n", ocl_error_string(err));
                return EXIT_FAILURE;
        }

        // Wait for the command commands to get serviced before reading back results
        clFinish(queue);

        // Read back the results from the device to verify the output
        err  = clEnqueueReadBuffer(queue, buf_use, CL_TRUE, 0, nvertex * bitset_size, use, 0, NULL, NULL);
        err |= clEnqueueReadBuffer(queue, buf_def, CL_TRUE, 0, nvertex * bitset_size, def, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
                printf("Error: Failed to read output array: %s\n", ocl_error_string(err));
                exit(1);
        }


        // Print a the results
		if (print_output) print_vertices(nvertex, maxsucc, vertices, pred_list, succ_list);
}

