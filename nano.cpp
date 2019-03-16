#include <iostream>
#include <math.h>
#include <cstring>
#include <algorithm>

#ifndef byte
	#define byte unsigned char
#endif

/*
 * Benchmarking tester for GNU/Linux
 * Remember: Add the following options in your bootloader (GRUB/Lilo): `isolcpu=<NUM>` to isolate a CPU
 */

#define TESTS 1000000

#define CUT_HEAD true
#define HEAD 0.05
#define CUT_TAIL true
#define TAIL 0.05


#define MULTICORE false //Warning: you cannot isolate all cores. So this result is not really a benchmark (if you're software decoding videos at the same time, it will totally change)

inline uint64_t rdtsc()
{
    uint32_t lo, hi;
    __asm__ __volatile__
	(
      "xorl %%eax, %%eax\n"
      "cpuid\n"
      "rdtsc\n"
      : "=a" (lo), "=d" (hi)
      :
      : "%ebx", "%ecx"
	);
    return (uint64_t)hi << 32 | lo;
}

int main(int argc, char* argv[])
{
	size_t Tests = TESTS;

	uint64_t* Vars = (uint64_t*)malloc( Tests * sizeof(uint64_t) );

	double Mean = 0.0;
	double Variance = 0.0;
	size_t Mode = 0;

	unsigned int i = 0;
#pragma omp parallel for ordered schedule(dynamic) if(MULTICORE) shared(Vars) private(i)
	for (i = 0; i < Tests; i++)
	{
		uint64_t time = rdtsc();

		//The code to benchmark (run it with nothing first, to test the time it takes to 'measure time')

		//* Allocating 2^20 (0x0) bytes 
		//byte* Arr = (byte*)malloc(1048576 * sizeof(byte));
		//memset(Arr, 0x0, 1048576 * sizeof(byte)); //~187µs
		//byte* Arr = (byte*)calloc(1048576, sizeof(byte)); //~189µs, the NetBSD calloc is a malloc+memset but adds a check for NULLPTR before running memset, hence the 2µs difference
		//free(Arr); //DO NOT FORGET TO UNCOMMENT THIS LINE

		//* Number of digits in int in base10
		//size_t len = (int)log(i)+1; //~366ns
		//size_t len = std::to_string(i).size(); //~837ns

		time = rdtsc() - time;
		Vars[i] = time;
	}

	//Sort the series
	std::sort( &Vars[0], &Vars[Tests], [](const uint64_t& left, const uint64_t& right){ return left > right; } );

	size_t From = 0;
#if CUT_HEAD == true
	From = HEAD*Tests;
#endif

	size_t To = Tests;
#if CUT_TAIL == true
	To -= TAIL*Tests;
#endif

	if (From != 0 || To != Tests)
	{
		Tests = To-From;
		uint64_t* Vars_t = (uint64_t*)malloc( Tests * sizeof(uint64_t) );
		memcpy(&Vars_t[0], &Vars[From], Tests*sizeof(uint64_t));

		free(Vars);

		Vars = (uint64_t*)malloc( Tests * sizeof(uint64_t) );
		memcpy(&Vars[0], &Vars_t[0], Tests*sizeof(uint64_t));

		free(Vars_t);
	}


	//Mean
#pragma omp parallel for ordered schedule(dynamic) shared(Vars, Variance) private(i)
	for (i = 0; i < Tests; i++)
		Mean += (double)Vars[i]/Tests;


	//Variance
#pragma omp parallel for ordered schedule(dynamic) shared(Vars, Variance) private(i)
	for (i = 0; i < Tests; i++)
	{
		//We cannot use abs because it's unsigned
		if ((double)Vars[i] > Mean)
			Variance += ((double)Vars[i] - Mean)*((double)Vars[i] - Mean)/Tests;
		else
			Variance += (Mean - (double)Vars[i])*(Mean - (double)Vars[i])/Tests;
	}

	//Find the mode
	size_t num = 0;
	size_t num_t = 0;
	for (i = 1; i < Tests; i++)
	{
		if ( Vars[i] == Vars[i-1] )
			num++;

		if (num > num_t)
		{
			num_t = num;
			Mode = i;
		}
	}

	std::cout << "Sample Size:                  " << TESTS << "\n";
	if (TESTS != Tests) std::cout << "Reduced Sample:               " << Tests << "\n";
	std::cout << "==============================================\n";
	std::cout << "Q1:                           " << Vars[(uint64_t)(3.0*Tests/4.0)] << "ns\n";
	std::cout << "Median:                       " << Vars[(uint64_t)(Tests/2.0)] << "ns\n";
	std::cout << "Q3:                           " << Vars[(uint64_t)(Tests/4.0)] << "ns\n";
	std::cout << "Q3-Q1:                        " << Vars[(uint64_t)(Tests/4.0)] - Vars[(uint64_t)(3.0*Tests/4.0)] << "ns\n";
	std::cout << "\n";
	std::cout << "Min:                          " << Vars[Tests-1] << "ns\n";
	std::cout << "Q1-Min:                       " << Vars[(uint64_t)(3.0*Tests/4.0)]-Vars[Tests-1] << "ns\n";
	std::cout << "Max:                          " << Vars[0] << "ns\n";
	std::cout << "Max-Q3:                       " << Vars[0]-Vars[(uint64_t)(Tests/4.0)] << "ns\n";
	std::cout << "\n";
	std::cout << "Max-Min:                      " << Vars[0]-Vars[Tests-1] << "ns\n";
	std::cout << "Max-Min to Q3-Q1:             " << (double)(Vars[0]-Vars[Tests-1])/(Vars[(uint64_t)(Tests/4.0)] - Vars[(uint64_t)(3.0*Tests/4.0)])*100.0 << "%\n";
	std::cout << "\n";
	std::cout << "Mean:                         " << Mean << "ns\n";
	std::cout << "Mode:                         " << Vars[Mode] << "ns\n";
	std::cout << "\n";
	std::cout << "Standard Deviation:           " << sqrt(Variance) << "ns\n";
	std::cout << "Standard Deviation to Mean:   " << sqrt(Variance)/Mean*100.0 << "%\n";
	std::cout << "Variance:                     " << Variance << "\n";
	std::cout << "\n";
	std::cout << "Confidence Interval (99%):    [" << Mean-2.576*sqrt(Variance)/sqrt((double)Tests) << "ns, " << Mean+2.576*sqrt(Variance)/sqrt((double)Tests) << "ns]\n";
	std::cout << "Confidence Interval (98%):    [" << Mean-2.326*sqrt(Variance)/sqrt((double)Tests) << "ns, " << Mean+2.326*sqrt(Variance)/sqrt((double)Tests) << "ns]\n";
	std::cout << "Confidence Interval (95%):    [" << Mean-1.96*sqrt(Variance)/sqrt((double)Tests) << "ns, " << Mean+1.96*sqrt(Variance)/sqrt((double)Tests) << "ns]\n";
	std::cout << "Confidence Interval (90%):    [" << Mean-1.645*sqrt(Variance)/sqrt((double)Tests) << "ns, " << Mean+1.645*sqrt(Variance)/sqrt((double)Tests) << "ns]\n";
	std::cout << "==============================================" << std::endl; //Flush

	free(Vars);
	
	return 0;
}
