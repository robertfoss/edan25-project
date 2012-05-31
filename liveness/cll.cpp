#include <stdio.h>
#include <string>
#include <iostream>

#include "cll.h"
#include "util.h"

CL::CL()
{
    printf("Initialize OpenCL object and context\n");
    //setup devices and context

    //this function is defined in util.cpp
    //it comes from the NVIDIA SDK example code
    ///err = oclGetPlatformID(&platform);
    //oclErrorString is also defined in util.cpp and comes from the NVIDIA SDK
    ///printf("oclGetPlatformID: %s\n", oclErrorString(err));
    std::vector<cl::Platform> platforms;
    err = cl::Platform::get(&platforms);
/*    printf("cl::Platform::get(): %s\n", oclErrorString(err));
    printf("number of platforms: %d\n", platforms.size());
    if (platforms.size() == 0) {
        printf("Platform size 0\n");
    }
*/	printDevices();

    // Get the number of GPU devices available to the platform
    // we should probably expose the device type to the user
    // the other common option is CL_DEVICE_TYPE_CPU
    ///err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
    ///printf("clGetDeviceIDs (get number of devices): %s\n", oclErrorString(err));


    // Create the device list
    ///devices = new cl_device_id [numDevices];
    ///err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
    ///printf("clGetDeviceIDs (create device list): %s\n", oclErrorString(err));


    //for right now we just use the first available device
    //later you may have criteria (such as support for different extensions)
    //that you want to use to select the device
    deviceUsed = 0;
    //create the context
    ///context = clCreateContext(0, 1, &devices[deviceUsed], NULL, NULL, &err);
    //context properties will be important later, for now we go with defualts
    cl_context_properties properties[] =
    { CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[0])(), 0};

    context = cl::Context(CL_DEVICE_TYPE_GPU, properties);
    devices = context.getInfo<CL_CONTEXT_DEVICES>();
    //printf("number of devices %d\n", devices.size());


    //create the command queue we will use to execute OpenCL commands
    ///command_queue = clCreateCommandQueue(context, devices[deviceUsed], 0, &err);
    try {
        queue = cl::CommandQueue(context, devices[deviceUsed], 0, &err);
    }
    catch (cl::Error er) {
        printf("ERROR: %s(%d)\n", er.what(), er.err());
    }

}

CL::~CL()
{
    /*
    printf("Releasing OpenCL memory\n");
    if(kernel)clReleaseKernel(kernel);
    if(program)clReleaseProgram(program);
    if(command_queue)clReleaseCommandQueue(command_queue);
    //need to release any other OpenCL memory objects here
    if(cl_a)clReleaseMemObject(cl_a);
    if(cl_b)clReleaseMemObject(cl_b);
    if(cl_c)clReleaseMemObject(cl_c);

    if(context)clReleaseContext(context);

    if(devices)delete(devices);
    printf("OpenCL memory released\n");

    */
}


void CL::loadProgram(std::string kernel_source)
{
    //Program Setup
    int pl;
    //size_t program_length;
    pl = kernel_source.size();
    printf("Loading OpenCL kernel, kernel size: %d bytes\n", pl);
    //printf("OpenCL kernel: \n %s\n", kernel_source.c_str());
    
    try
    {
        cl::Program::Sources source(1,
				std::make_pair(kernel_source.c_str(), pl));
        program = cl::Program(context, source);
    }
    catch (cl::Error er) {
        printf("ERROR: %s(%s)\n", er.what(), oclErrorString(er.err()));
    }

    printf("Building program.. ");
    try
    {
        err = program.build(devices);
		std::cout << "successful!" << std::endl;
    }
    catch (cl::Error er) {
        printf("error: %s\n", oclErrorString(er.err()));
		std::cout << "\tBuild Status: \t" << program.getBuildInfo<CL_PROGRAM_BUILD_STATUS>(devices[0]) << std::endl;
		std::cout << "\tBuild Options:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_OPTIONS>(devices[0]) << std::endl;
		std::cout << "\tBuild Log:\t" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
    }
}


void CL::printDevices()
{
////////////
/// Robertcode
///////////

    cl_platform_id platform[32];
    cl_uint num_platform;
    char vendor[1024];
    cl_device_id devices[32];
    cl_uint num_devices;
    char deviceName[1024];
    cl_uint numberOfCores;
    cl_long amountOfMemory;
    cl_uint clockFreq;
    cl_ulong maxAllocatableMem;
    cl_ulong localMem;
    cl_bool available;
    char extensions[4096];
    size_t extensions_len = 0;


    //get the number of platforms
    clGetPlatformIDs(32, platform, &num_platform);
    for(int i = 0; i < num_platform; i++) {
        clGetPlatformInfo (platform[i], CL_PLATFORM_VENDOR, sizeof(vendor), vendor, NULL);

        clGetDeviceIDs(platform[i], CL_DEVICE_TYPE_ALL, sizeof(devices), devices, &num_devices);
        for(int j = 0; j < num_devices; ++j) {
            clGetDeviceInfo(devices[j], CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_VENDOR, sizeof(vendor), vendor, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(numberOfCores), &numberOfCores, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(amountOfMemory), &amountOfMemory, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clockFreq), &clockFreq, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(maxAllocatableMem), &maxAllocatableMem, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_LOCAL_MEM_SIZE, sizeof(localMem), &localMem, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_AVAILABLE, sizeof(available), &available, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_EXTENSIONS, sizeof(extensions), &extensions, &extensions_len);
            
            
            printf("Platform-%d Device-%d\t%s - %s\tCores: %d\tMemory: %ldMB\tAvailable: %s\n", i, j, vendor, deviceName, numberOfCores, (maxAllocatableMem/(1024*1024)), (available ? "Yes" : "No"));
            char* an_extension;
            if (extensions_len > 0){
            	
           		printf("\t\tExtensions: \t");
           		an_extension = strtok(extensions, " ");
           		while (an_extension != NULL){
           			printf("%s\n\t\t\t\t", an_extension);
           			an_extension = strtok(NULL, " ");
           		}
			}
			printf("\n");
        }

    }

///////////
///  /Robertcode
///////////
}


