# Cache Hierarchy with Prefetcher Simulator

This simulator implements a flexible cache and memory hierarchy. 

The simulator can then be used to compare the performance, area, and energy of different memory hierarchy configurations, using a subset of the SPEC 2006 benchmark suite, SPEC 2017 benchmark suite, and/or
microbenchmarks.

Guided by a project from ECE563 at NC State University taught by Prof. [Eric Rotenberg](https://ece.ncsu.edu/people/ericro/).


# Instructions on Using this Repo 
1. Type "make" to build.  (Type "make clean" first if you already compiled and want to recompile from scratch.)

2. Run trace reader:
   To run without throttling output:
   ```
   ./cachesim 32 8192 4 262144 8 3 10 ./example_trace.txt
   ```

   To run with throttling (via "less"):
   ```
   ./cachesim 32 8192 4 262144 8 3 10 ./example_trace.txt | less
   ```

   To run and confirm that all requests in the trace were read correctly:
   ```
   ./cachesim 32 8192 4 262144 8 3 10 ./example_trace.txt > echo_trace.txt
   diff ./example_trace.txt echo_trace.txt
   ```
   
   The result of "diff" should indicate that the only difference is that echo_trace.txt has the configuration information.
   
   0a1,10

   ===== Simulator configuration =====
   
   BLOCKSIZE:  32
   
   L1_SIZE:    8192
   
   L1_ASSOC:   4
   
   L2_SIZE:    262144
   
   L2_ASSOC:   8
   
   PREF_N:     3
   
   PREF_M:     10
   
   trace_file: ./example_trace.txt
   
   ===================================
   
