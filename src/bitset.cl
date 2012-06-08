__kernel void bitset_or( __global unsigned int* bs1, __global unsigned int* bs2, const unsigned int subset_count)
{
	unsigned int i = get_global_id(0);
	if(i < subset_count) {
        bs1[i] |= bs2[i];
	}
}


__kernel void bitset_nand( __global unsigned int* bs1, __global unsigned int* bs2, const unsigned int subset_count)
{
	unsigned int i = get_global_id(0);
	if(i < subset_count) {
        bs1[i] = ~(bs1[i] & bs2[i]) & bs1[i];

	}
}


__kernel void bitset_megaop( __global unsigned int* in, __global unsigned int* out,  __global unsigned int* use,
                             __global unsigned int* def, const unsigned int subset_count)
{
	unsigned int i = get_global_id(0);
	if(i < subset_count) {
        in[i]  = out[i];
		in[i]  = in[i] & (~(in[i] & def[i]));
		in[i] |= use[i];
	}
}
