#include <stdio.h>

#include "cll.h"


int main(int argc, char** argv)
{
    printf("Hello, OpenCL\n");
    //initialize our CL object, this sets up the context
    CL example;
    
    //load and build our CL program from the file
    #include "liveness.cl" //const char* kernel_source is defined in here
    example.loadProgram(kernel_source);

    //initialize the kernel and send data from the CPU to the GPU
    example.popCorn();
    //execute the kernel
    example.runKernel();
    exit(0);
}
