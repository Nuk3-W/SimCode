#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include "sim_cache.h"
#include "sim.cpp"

using namespace std;

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim_cache 32 8192 4 7 262144 8 gcc_trace.txt
    argc = 8
    argv[0] = "sim_cache"
    argv[1] = "32"
    argv[2] = "8192"
    ... and so on
*/
int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    cache_params params;    // look at sim_cache.h header file for the the definition of struct cache_params
    char rw;                // variable holds read/write type read from input file. The array size is 2 because it holds 'r' or 'w' and '\0'. Make sure to adapt in future projects
    long unsigned int addr; // Variable holds the address read from input file

    if(argc != 8)           // Checks if correct number of inputs have been given. Throw error and exit if wrong
    {
        printf("Error: Expected inputs:7 Given inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    params.block_size       = strtoul(argv[1], NULL, 10);
    params.l1_size          = strtoul(argv[2], NULL, 10);
    params.l1_assoc         = strtoul(argv[3], NULL, 10);
    params.vc_num_blocks    = strtoul(argv[4], NULL, 10);
    params.l2_size          = strtoul(argv[5], NULL, 10);
    params.l2_assoc         = strtoul(argv[6], NULL, 10);
    trace_file              = argv[7];

    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    // Print params
    printf("  ===== Simulator configuration =====\n"
            "  L1_BLOCKSIZE:                     %lu\n"
            "  L1_SIZE:                          %lu\n"
            "  L1_ASSOC:                         %lu\n"
            "  VC_NUM_BLOCKS:                    %lu\n"
            "  L2_SIZE:                          %lu\n"
            "  L2_ASSOC:                         %lu\n"
            "  trace_file:                       %s\n"
            "  ===================================\n\n", params.block_size, params.l1_size, params.l1_assoc, params.vc_num_blocks, params.l2_size, params.l2_assoc, trace_file);
    Cache L2 = Cache(NULL, "L2", params.l2_size, params.l2_assoc, params.block_size, NULL);
    Cache L1 = Cache(NULL, "L1", params.l1_size, params.l1_assoc, params.block_size, &L2);
    Victim VL1 = Victim(&L1, params.vc_num_blocks, "VL1", (params.vc_num_blocks*params.block_size), params.vc_num_blocks, params.block_size, NULL);
    Cache* ptr  = &VL1;
    L1.Assign(ptr);
    char str[2];
    
    while(fscanf(FP, "%s %lx", str, &addr) != EOF)
    { 
        rw = str[0];
        //for (int i = 0; i < 4; i++)
        //    cout << VL1.VirtualMemory[0][i] << " ";
        //cout << "Count" << endl; 
        if (rw == 'r')
        {   
            //cout << VL1.VirtualMemory[0][15] << endl;
            //cout << addr << endl;
            L1.Read(DecToBinary(addr)); 
            //printf("%s %lx\n", "read", addr);           // Print and test if file is read correctly
            //cout << L2.Reads << endl;
        }
        else if (rw == 'w')
        {   
            //cout << addr << endl;
            //cout << VL1.VirtualMemory[0][15] << endl;
            L1.Write(DecToBinary(addr)); 
            //printf("%s %lx\n", "write", addr);          // Print and test if file is read correctly
            //cout << L2.Reads << endl;
        }
        /*************************************
                  Add cache code here
        **************************************/
    }
    //cout << "Total Miss L1 " << L1.Misses << endl << "Total Hits L1 " << L1.Hits << endl;
    cout << "Total Read L1 " << L1.Reads << endl << "Total Reads Misses L1 " << L1.ReadMiss << endl;
    cout << "Total Writes L1 " <<  L1.Writes << endl << "Total Write Misses L1 " << L1.WriteMiss << endl;
    cout << "Total Swap Requests " << VL1.Reads << endl <<  "Total Swap " << VL1.SwapRequests << endl;
    cout << "L1 Miss Rate " << ((float)(L1.Misses - VL1.SwapRequests)/100000) << endl; 
    cout << "L1 WriteBacks " << L1.WriteBacks + VL1.WriteBacks << endl;
    cout << "L2 Reads " << L2.Reads << endl << "Total Read Misses L2 " << L2.ReadMiss << endl;
    cout << "L2 Writes " << L2.Writes << endl << "Total Write Misses L2 " << L2.WriteMiss << endl;
    cout << "L2 Miss Rate " << ((float)L2.Misses/(L2.Reads)) << endl;
    cout << "L2 WriteBacks " << L2.WriteBacks << endl;
 
    return 1;
}
