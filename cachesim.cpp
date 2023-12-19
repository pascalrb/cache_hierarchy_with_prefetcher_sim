#include <iostream>
#include <fstream>
#include "cachesim.h"

using namespace std;

/*  "argc" holds the number of command-line arguments.
    "argv[]" holds the arguments themselves.

    Example:
    ./sim 32 8192 4 262144 8 3 10 traces/gcc_trace.txt
    argc = 9
    argv[0] = "./sim"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/


int main (int argc, char *argv[]) {
    FILE *fp;			// File pointer.
    char *trace_file;		// This variable holds the trace file name.
    cache_params_t params;	// Look at the cachesim.h header file for the definition of struct cache_params_t.
    char rw;			// This variable holds the request's type (read or write) obtained from the trace.
    uint32_t addr;		// This variable holds the request's address obtained from the trace.
     			// The header file <inttypes.h> above defines signed and unsigned integers of various sizes in a machine-agnostic way.  "uint32_t" is an unsigned integer of 32 bits.

    // Exit with an error if the number of command-line arguments is incorrect.
    if (argc != 9) {
        cout << "Error: Expected 8 command-line arguments but was provided " << argc - 1 << "." << endl;
        cout << "usage: ./cachesim 32 8192 4 262144 8 3 10 gcc_trace.txt" << endl;
        exit(EXIT_FAILURE);
    }

    // "atoi()" (included by <stdlib.h>) converts a string (char *) to an integer (int).
    params.blocksize = (uint32_t) atoi(argv[1]);
    params.l1_size   = (uint32_t) atoi(argv[2]);
    params.l1_assoc  = (uint32_t) atoi(argv[3]);
    params.l2_size   = (uint32_t) atoi(argv[4]);
    params.l2_assoc  = (uint32_t) atoi(argv[5]);
    params.pref_n    = (uint32_t) atoi(argv[6]);
    params.pref_m    = (uint32_t) atoi(argv[7]);
    trace_file       = argv[8];

    //if(params.l1_assoc < 1 || params.l2_assoc < 1){
    //    cout << "Error: l1 and l2 associativity must >= 1"
    //    exit(EXIT_FAILURE);
    //}
    ////TODO: check that blocksize, l1 size, l2 size are powers of 2

    //          if pref_n !=0, make sure pref_m is also !=0
    if(params.pref_n != 0){
        assert(params.pref_m != 0);
    }

    g_mem_op_count = 0;

// ------------------------------------------------------------------------------

    // Instantiating L1 and L2 cache
    Cache* l1_cache = new Cache(1, params.l1_size, params.l1_assoc, params.blocksize, params.pref_n, params.pref_m);
    Cache* l2_cache = new Cache(2, params.l2_size, params.l2_assoc, params.blocksize, params.pref_n, params.pref_m);

    if (params.l2_size > 0){
        l1_cache->next_lvl_cache = l2_cache;
    }


// ------------------------------------------------------------------------------

    // Open the trace file for reading.
    fp = fopen(trace_file, "r");
    if (fp == (FILE *) NULL) {
       // Exit with an error if file open failed.
       printf("Error: Unable to open file %s\n", trace_file);
       exit(EXIT_FAILURE);
    }
     

    uint32_t i = 1;
    // Read requests from the trace file and echo them back.
    while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {	// Stay in the loop if fscanf() successfully parsed two tokens as specified.
        if (rw == 'r'){
            //printf("%d=r %x\n", i, addr);
            l1_cache->read(addr);
        }else if (rw == 'w'){
            //printf("%d=w %x\n", i, addr);
            l1_cache->write(addr);
        }else {
            printf("Error: Unknown request type %c.\n", rw);
            exit(EXIT_FAILURE);
        }
        i++;

    }

    // Print simulator configuration.
    printf("===== Simulator configuration =====\n");
    printf("BLOCKSIZE:  %u\n", params.blocksize);
    printf("L1_SIZE:    %u\n", params.l1_size);
    printf("L1_ASSOC:   %u\n", params.l1_assoc);
    printf("L2_SIZE:    %u\n", params.l2_size);
    printf("L2_ASSOC:   %u\n", params.l2_assoc);
    printf("PREF_N:     %u\n", params.pref_n);
    printf("PREF_M:     %u\n", params.pref_m);
    printf("trace_file: %s\n", trace_file);
    printf("\n");
    
    printf("===== L1 contents =====\n");
    l1_cache->cache_sets_sort();
    l1_cache->print_cache();
    printf("\n");

    if(params.l2_size > 0){
        printf("===== L2 contents =====\n");
        l2_cache->cache_sets_sort();
        l2_cache->print_cache();
        printf("\n");

        if(params.pref_n > 0){
            printf("===== Stream Buffer(s) contents =====\n");
            l2_cache->print_prefetcher();
            printf("\n");
        }

    }else{
        if(params.pref_n > 0){
            printf("===== Stream Buffer(s) contents =====\n");
            l1_cache->print_prefetcher();
            printf("\n");
        }
    }

    printf("===== Measurements =====\n");
    printf("a. %-30s %u\n", "L1 reads: "                      , l1_cache->read_count);
    printf("b. %-30s %u\n", "L1 read misses: "                , l1_cache->read_miss_count);
    printf("c. %-30s %u\n", "L1 writes: "                     , l1_cache->write_count);
    printf("d. %-30s %u\n", "L1 write misses: "               , l1_cache->write_miss_count);
    double l1_miss_rate = ( (double)(l1_cache->read_miss_count + l1_cache->write_miss_count) / (double) (l1_cache->read_count + l1_cache->write_count) );
    printf("e. %-30s %.4f\n", "L1 miss rate: "                , l1_miss_rate);
    printf("f. %-30s %u\n", "L1 writebacks: "                 , l1_cache->writebacks_to_next_lvl_count);
    printf("g. %-30s %u\n", "L1 prefetches: "                 , l1_cache->prefetches_to_next_lvl_count);

    printf("h. %-30s %u\n", "L2 reads (demand): "             , l2_cache->read_count);
    printf("i. %-30s %u\n", "L2 read misses (demand): "       , l2_cache->read_miss_count);
    printf("j. %-30s %u\n", "L2 reads (prefetch): "           , l2_cache->read_from_prefetch_count);
    printf("k. %-30s %u\n", "L2 read misses (prefetch): "     , l2_cache->read_miss_from_prefetch_count);
    printf("l. %-30s %u\n", "L2 writes: "                     , l2_cache->write_count);
    printf("m. %-30s %u\n", "L2 write misses: "               , l2_cache->write_miss_count);
    double l2_miss_rate = (params.l2_size > 0) ? ((double)(l2_cache->read_miss_count) / (double) (l2_cache->read_count)) : (double) 0;
    printf("n. %-30s %.4f\n", "L2 miss rate: "                , l2_miss_rate);
    printf("o. %-30s %u\n", "L2 writebacks: "                 , l2_cache->writebacks_to_next_lvl_count);
    printf("p. %-30s %u\n", "L2 prefetches: "                 , l2_cache->prefetches_to_next_lvl_count);

    printf("q. %-30s %u\n", "memory traffic: "              , g_mem_op_count);

    return(0);
}

