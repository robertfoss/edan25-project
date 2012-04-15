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

__kernel void liveness(__global uint_2* x, __global unsigned int* c)
{
    unsigned int i = get_global_id(0);

    c[i] = gcd(x[i].a, x[i].b);
}




);
