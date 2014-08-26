Parallel Pattern Matching
==========================

Project from final year module, utilising OpenMP and MPI to parallelise the straightforward string matching algorithm.

The object of the module was to simply get the fastest time for generating correct solutions to a series of string matching problems, providing one OpenMP solution and one MPI solution. I also wrote a non-parallel solution to generate correct result files and use as a baseline time to beat for each parallel solution. The solutions had to come in two flavours: mode 0 finding the first instance of the pattern only, mode 1 finding all instances of the pattern in the text.

My OpenMP solution is fairly straightforward, using parallel for loops in each mode, leaving all the hard work to OpenMP. My MPI solution is more involved, breaking the problem into master and slave processes, where the master process gives individual jobs to the slaves, who return the results to the master.

Exact details of requirements from the project specification:

"Your program must be based on the straightforward pattern matching algorithm. The pattern must be searched from left to right and each character tested separately (in particular you must not use the memcmp function). You may assume that you have 8 cores to use for your OMP program and 16 for MPI.

You will be provided with an example folder called small-inputs in which you will find
 p patterns, pattern0.txt, pattern1.txt
 t texts, text0.txt, text1.txt
 a control file, control.txt

Each line in the control file will contain three numbers and will correspond to a single search.
 The first number will be 0, meaning find just the first occurrence, or 1, meaning find every occurrence.
 The second number will indicate which text to use.
 The third number will indicate which pattern to use.

You may assume that no text or pattern file will exceed 20MB. However, a pattern file may exceed the size of a text file. You may also assume that neither p nor t will exceed 20.

Your program should output its results to a single file, result OMP.txt or result MPI.txt, where each line will contain three integers indicating, respectively, the text id, the pattern id and the position the pattern was detected at. Text positions should be numbered from 0 and -1 should be used to indicate that the pattern is not detected in the text."

