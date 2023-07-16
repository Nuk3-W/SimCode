#include "function.h"
#include "sim_cache.h"

using namespace std;

class ControlUnit
{
public:
    bool PrefetchSet = false; //false is the tagged prefetcher true is the markov prefetcher
    Cache* CacheConnect;  
    ControlUnit (Cache* assign) : CacheConnect(assign) {};
}; 