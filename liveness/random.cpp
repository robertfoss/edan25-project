class Random {
	unsigned int w;
	unsigned int z;

	public:
		Random(unsigned int seed):w(seed+1),z(seed*seed+seed+w) {
		}


		unsigned int nextInt() {
			z = 36969 * (z & 65535) + (z >> 16);
			w = 18000 * (w & 65535) + (w >> 16);

			return (z << 16) + w;
		}
};


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

