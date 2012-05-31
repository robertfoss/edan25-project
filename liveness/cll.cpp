#include <stdio.h>
#include <string>
#include <iostream>

#include "cll.h"
#include "util.h"

#define MAX_RESOURCES 32

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
*/
	
    unsigned int best_platform = 0;
    unsigned int best_device = 0;
    getBestDevice(best_platform, best_device);
    std::cout << "Initiating platform-" << best_platform << " device-" << best_device << "." << std::endl;
    printDevices();


    cl_int error = 0;   // Used to handle error codes
    cl_platform_id platform[MAX_RESOURCES];
    cl_device_id device[MAX_RESOURCES];

    // Platform
    error = clGetPlatformIDs(MAX_RESOURCES, platform, NULL);
    if (error != CL_SUCCESS) {
       std::cout << "Error getting platform id: " << oclErrorString(error) << std::endl;
       exit(error);
    }
    // Device
    error = clGetDeviceIDs(platform[best_platform], CL_DEVICE_TYPE_ALL, sizeof(devices), device, NULL); //NULL, ignore number returned devices.
    //error = clGetDeviceIDs(platform[i], CL_DEVICE_TYPE_ALL, sizeof(devices), devices, &num_devices);
    if (err != CL_SUCCESS) {
       std::cout << "Error getting device ids: " << oclErrorString(error) << std::endl;
       exit(error);
    }
    // Context
    context = clCreateContext(0, 1, &(device[best_device]), NULL, NULL, &error);
    if (error != CL_SUCCESS) {
       std::cout << "Error creating context: " << oclErrorString(error) << std::endl;
       exit(error);
    }
    // Command-queue
    //queue = clCreateCommandQueue(context, &device[best_device], 0, &error); //c99-style
    queue = cl::CommandQueue(context, device[best_device], 0, &error);
    if (error != CL_SUCCESS) {
       std::cout << "Error creating command queue: " << oclErrorString(error) << std::endl;
       exit(error);
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

void getBestDevice(unsigned int &ret_platform, unsigned int &ret_device)
{
    unsigned long long best_score = 0;

    cl_platform_id platform[32];
    cl_uint num_platform = 32;
    cl_device_id devices[32];
    cl_uint num_devices;
    cl_uint numberOfCores;
    cl_long amountOfMemory;
    cl_uint clockFreq;
    cl_ulong maxAllocatableMem;
    char extensions[4096];
    size_t extensions_len = 0;

    //get the number of platforms
    clGetPlatformIDs(32, platform, &num_platform);
    for(unsigned int i = 0; i < num_platform; i++) {
        clGetDeviceIDs(platform[i], CL_DEVICE_TYPE_ALL, sizeof(devices), devices, &num_devices);
        for(unsigned int j = 0; j < num_devices; ++j) {
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(numberOfCores), &numberOfCores, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(amountOfMemory), &amountOfMemory, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_CLOCK_FREQUENCY, sizeof(clockFreq), &clockFreq, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(maxAllocatableMem), &maxAllocatableMem, NULL);
            clGetDeviceInfo(devices[j], CL_DEVICE_EXTENSIONS, sizeof(extensions), &extensions, &extensions_len);
            char* an_extension;
            if (extensions_len > 0){
            	
           		printf("\t\tExtensions: \t");
           		an_extension = strtok(extensions, " ");
           		while (an_extension != NULL){
                    if( strcmp( "cl_khr_global_int32_base_atomics", an_extension) == 0){
                        unsigned long long score = clockFreq*numberOfCores+amountOfMemory;
                        if(score>best_score){
                            ret_platform = i;
                            ret_device = j;
                        }
                    }
           			printf("%s\n\t\t\t\t", an_extension);
           			an_extension = strtok(NULL, " ");
           		}
			}
			printf("\n");
        }

    }
    
}


void printDevices()
{
    cl_platform_id platform[32];
    cl_uint num_platform = 32;
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
    for(unsigned int i = 0; i < num_platform; i++) {
        clGetPlatformInfo (platform[i], CL_PLATFORM_VENDOR, sizeof(vendor), vendor, NULL);

        clGetDeviceIDs(platform[i], CL_DEVICE_TYPE_ALL, sizeof(devices), devices, &num_devices);
        for(unsigned int j = 0; j < num_devices; ++j) {
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
}


