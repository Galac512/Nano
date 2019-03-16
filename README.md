# NANO - A nanosecond-precise clock for benchmarking

NANO is a tool I wrote to benchmark the speed of different functions. It uses some functionalities of the Linux kernel to get a nanosecond clock working. Of course, measuring really small 
times may create a lot of errors. That's why NANO has a statistic analysis tool: to allow you to draw real conclusions!

## Features
* A nanosecond clock (based on your CPU clock)
* Statistic analysis with options to correct aberrations
* Multi core processing (both, to compute statistics and run the tests)

## Compatibility
It should works with the Linux Kernel (since Linux 2.6)

## Compiling
To compile NANO, just run `g++ nano.cpp`. Additionally, you can add the `-fopenmp` flag to G++, if you want multi core processing.


## Options
In the `nano.cpp` file, you can edit some `#define`s to change the program.

* `#define TESTS 1000000` Set the number of tests to create the series.
* `#define CUT_HEAD false` Cuts the highest values of the series (to avoid aberrations).
	* `#define HEAD 0.05` Set the percentage of the highest values to be removed.
* `#define CUT_TAIL false` Cuts the lowest values of the series (to avoid aberrations).
	* `#define TAIL 0.05` Set the percentage of the lowest values to be removed.
* `#define MULTICORE false` Enable/Disable multicore processing (you need to add `-fopenmp` to G++ if you want to enable it)

## Usage
You should use the `cpupower` tool to gather informations and set some options on your CPU (on Debian: `apt install linux-cpupower`).

NANO works without any modifications, but other processes running on the same core may affect some of the results. In order to guarantee the maximum accuracy, you should change your boot options (in Grub/Lilo): set `isolcpu=<NUM>`, to tell the kernel not to use core `<NUM>`. After that, you may use `taskset` to force the program to run on the core you previously isolated.

## Your own custom tests
To create your own benchmarks, you can put the code you want to test inside the first `for` loop, around line 57. I suggest you to run the program without adding anything, just to measure the time it takes for your computer to measure time (yeah with a nanosecond precision, reading through assembly code is significantly slow).
I wrote two examples that you can use or modify.

## Outputs
How to count the number of digits in an integer (in base 10)?

- We  can convert the integer to a string `std::to_string()` and use the `size()` operator:
```
Sample Size: 1000000
==============================================
…
Confidence Interval (99%):    [834.553ns, 840.305ns]
==============================================
```

- We can use one of the `log10()` function's properties:
```
Sample Size: 1000000
==============================================
…
Confidence Interval (99%):    [365.897ns, 367.318ns]
==============================================
```
_Tested with all integers from 0 to 1,000,000._

Conclusion: The `log10()` approach is definitely faster.

## Statistic analysis
NANO has a statistic analysis tool, it compute the following informations:

* Number of tests: `Sample Size`
* Quartiles: `Q1` and `Q3`
* Median: `Median`
* Distance between the quartiles: `Q3-Q1`
* Minimum value: `Min`
* Distance from the minimum value to Q1: `Q1-Min`
* Maximum value: `Max`
* Distance from the maximum value to Q3: `Max-Q3`
* Length of the series: `Max-Min`
* Space between two quartiles: `Q3-Q1`
* Mean of the series: `Mean`
* Mode of the series: `mode`
* Standard deviation of the series: `Standard Deviation`
* Standard deviation divided by the mean of the series: `Standard Deviation to Mean`
* Variance of the series: `Variance`
* Confidence intervals (99%, 98%, 95% and 90%): `Confidence Interval`

When you activate the statistic corrections, the statistic series is calculated only on the `Reduced Sample`


## Statistic correction
The distribution was really strange with the benchmarks. Here is an example with benchmarks of `calloc()`:
```
Sample Size:                  100000
==============================================
Q1:                           189017ns
Median:                       189608ns
Q3:                           189832ns
Q3-Q1:                        815ns

Min:                          85001ns
Q1-Min:                       104016ns
Max:                          3573563ns
Max-Q3:                       3383731ns

Max-Min:                      3488562ns
Max-Min to Q3-Q1:             428044%

Mean:                         190909ns
Mode:                         188374ns

Standard Deviation:           18274.5ns
Standard Deviation to Mean:   9.57236%
Variance:                     3.33958e+08
==============================================
```
Just look at the difference between `Q3-Q1` and `Max-Min`, it's incredible! `Q1`, the `Median`,  `Q3` and the `Mean` are roughly equal.

We apply a filter that 'cuts' the 5% lowest and 5% highest values. Here is the result:
```
Sample Size:                  100000
Reduced Sample:               90000
==============================================
Q1:                           200969ns
Median:                       201136ns
Q3:                           201639ns
Q3-Q1:                        670ns

Min:                          200738ns
Q1-Min:                       231ns
Max:                          202145ns
Max-Q3:                       506ns

Max-Min:                      1407ns
Max-Min to Q3-Q1:             210%

Mean:                         201272ns
Mode:                         200738ns

Standard Deviation:           387.243ns
Standard Deviation to Mean:   0.192398%
Variance:                     149957
==============================================
```
As you can see, it's much better now. The standard Deviation was divided by 45, and The `Min` and `Max` values can fit on a box diagram, without requiring the use of a logarithmic scale.
