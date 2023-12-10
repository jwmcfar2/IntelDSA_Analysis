#include "utils.h"

// Init Globals
uint64_t L1d_Miss_Latency=0;
uint64_t L1TLB_Miss_Latency=0;
uint64_t L1TLB_Entries=0;
uint64_t globalAgitator=20;

void volatile flush(void* p){
    asm volatile ("clflush 0(%0)\n"
        :
        : "c" (p)
        : "rax");
}

void volatile flushLiteral(uint64_t VA){
    printf("DEBUG-C1: {%lu}\n", VA);
    void* ptrVA = (void *)VA;
    printf("DEBUG-C2: {%lu} -- NULL=%s\n", ptrVA, (ptrVA==0)?"True":"False");
    if(ptrVA==0)
        return;
    asm volatile ("clflush 0(%0)\n"
        :
        : "c" (VA)
        : "rax");
    printf("DEBUG-D: {%lu}\n", VA);
}

void volatile flush2(void* p, void* q){
    asm volatile ("clflush 0(%0)\n"
        :
        : "c" (p)
        : "rax");
    asm volatile ("clflush 0(%0)\n"
        :
        : "c" (q)
        : "rax");
}

void volatile prime2(uint8_t *src, uint8_t *dst, uint64_t size){
    for(int i=0; i<size; i++)
    {
        globalAgitator += src[i];
        globalAgitator += dst[i];
        globalAgitator %= 128;
    }
    globalAgitator %= 20;
}

// I use this to "sleep" the main() thread at a ~400 cycle granularity
uint64_t spawnNOPs(uint16_t count) {
    for(int i=0; i<count; i++)
        __asm__ volatile ("nop");
}

uint64_t rdtsc() {
    unsigned cycles_high, cycles_low;

    asm volatile ("CPUID\n\t"
    "RDTSC\n\t"
    "mov %%edx, %0\n\t"
    "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
    "%rax", "%rbx", "%rcx", "%rdx");

    return (((uint64_t)cycles_high << 32) | cycles_low);
}

void compilerMFence(){asm volatile ("" : : : "memory");}
void cpuMFence(){_mm_mfence();}

void detailedAssert(bool assertRes, const char* msg){
    if(!assertRes)
    {
        printf("Error: Assertion Failed. \n    ErrMsg: \"%s\"\n", msg);
        abort();
    }
}

void valueCheck(uint8_t* src, uint8_t* dst, uint64_t size, char* errDetails)
{
    char* errStr = malloc(strlen(errDetails) + 75 + 1);
    strcpy(errStr, errDetails);
    strcat(errStr, "-- Validation:FAIL - Source/Destination value mismatch.");

    if(memcmp(src, dst, size) == 0)
        {}//printf("\t>>%s\t-- Validation:SUCCESS - Destination value matches source value.\n", errDetails);
    else
        detailedAssert(false, errStr);

    free(errStr);
}

void volatile flushHelperFn(){
    flushAllDataCaches();
}

// Unused?
void volatile floodHelperFn(){
    floodAllDataCaches();
    //compilerMFence();
    //floodL1InstrCache();
    //compilerMFence();

    // It takes so many LL Nodes to flood L2/L3, the TLBs will surely be flooded indirectly...
    //floodL1TLB();
    //compilerMFence();
}

void profileCacheLatency(){
    uint64_t randIndex, randVal, start, end;
    uint64_t avgLLCMissLatency=0, avgHitLatency=0, avgFRLatency=0;
    uint64_t randArr0[4*1024], randArr1[4*1024], randArr2[4*1024], randArr3[4*1024], profileArr[4*1024];
    int runs=100000;

    // Try to get a relatively fair Cold Miss Latency
    for(int i=0; i<10; i++)
    {
        uint64_t randSwitch = rand()%4;

        switch(randSwitch){
            case 0:
                randIndex = rand()%(4*1024);
                randVal = rand()%(1024);

                start = rdtsc();
                randArr0[randIndex] = randVal;
                end = rdtsc();

                avgLLCMissLatency+=(end-start);
                flush(&randArr0[randIndex]);
                break;
            case 1:
                randIndex = rand()%(4*1024);
                randVal = rand()%(1024);

                start = rdtsc();
                randArr1[randIndex] = randVal;
                end = rdtsc();

                avgLLCMissLatency+=(end-start);
                flush(&randArr1[randIndex]);
                break;
            case 2:
                randIndex = rand()%(4*1024);
                randVal = rand()%(1024);

                start = rdtsc();
                randArr2[randIndex] = randVal;
                end = rdtsc();

                avgLLCMissLatency+=(end-start);
                flush(&randArr2[randIndex]);
                break;
            case 3:
                randIndex = rand()%(4*1024);
                randVal = rand()%(1024);

                start = rdtsc();
                randArr3[randIndex] = randVal;
                end = rdtsc();

                avgLLCMissLatency+=(end-start);
                flush(&randArr3[randIndex]);
                break;            
        }
    }
    LLC_Miss_Latency=(avgLLCMissLatency/10);

    // Profile our system - so we know the average latency of a true cache hit versus the
    // latency of a miss after we flush the cache of the address (NOT A TRUE COLD MISS)
    //printf("Profiling Cache Hit vs Miss Latency (%d runs)...\n", runs);
    for(int i=0; i<runs; i++)
    {
        randIndex = rand()%(4*1024);

        // Cold miss - Pull address into cache
        randVal = rand()%(1024);
        profileArr[randIndex] = randVal;

        // Measure the time of a hit
        randVal = rand()%(1024);
        start = rdtsc();
        profileArr[randIndex] = randVal;
        end = rdtsc();
        avgHitLatency += end-start;

        // Flush - measure latency from reload
        flush(&profileArr[randIndex]);
        randVal = rand()%(1024);
        start = rdtsc();
        profileArr[randIndex] = randVal;
        end = rdtsc();
        avgFRLatency += end-start;
    }
    L1d_Hit_Latency=(avgHitLatency/runs);
    flushReload_latency=(avgFRLatency/runs);

    //printf("DEBUG: Avg Cache Hit Latency (Cycles)    =  %ld\n", L1d_Hit_Latency);
    //printf("DEBUG: Avg Flush+Reload Latency (Cycles) =  %ld\n", flushReload_latency);
    //printf("DEBUG: Avg LLC Miss Latency (Cycles)     =  %ld\n", LLC_Miss_Latency);
}

void profileRDTSC(){
    int runs=1000000;
    uint64_t startTime, endTime;
    uint64_t avgRDTSCLatency=0;

    // Pull global into cache
    globalAgitator += rand()%3;

    for(int i=0; i<runs; i++)
    {
        startTime = rdtsc();
        globalAgitator += rdtsc();
        endTime = rdtsc();
        avgRDTSCLatency += endTime - startTime;

        globalAgitator %= 50;
    }
    RTDSC_latency = avgRDTSCLatency/runs;

    //printf("\nDEBUG: AVG RDTSC CYCLES = %lu cycles\n\n", RTDSC_latency);

    globalAgitator %= 50;
}

void volatile calculateL1TLB_Entries(){
    //printf("Calculating L1TLB...\n");
    //printf("DEBUG: ListNode size *should* be 4096, actual=%d\n", sizeof(ListNode));
    uint64_t startTime, endTime, baseLatency;
    uint32_t randIndex;
    ListNode *head, *curr;
    uint64_t randAgitator = 7;
    uint32_t deltaTolerance = 200;

    // Make sure L1d_Miss_Latency has been calculated:
    if(L1d_Miss_Latency == 0)
        setL1DMissLatency;

    // Make a 'head' node for LL:
    head = createNode();
    curr = head;

    // L1 TLB Shouldn't be more than 64 entries I wouldn't think,
    // but let's double it to be certain when we check later...
    // (starting at '1' because we alread made the head node)
    for(int i=1; i<128; i++)
    {
        // First let's make a new LL entry...
        curr->next = createNode();

        // ...Then TRIPLE check it is in L1
        for(int j=0; j<LL_NODE_PADDING; j++)
        {
            randAgitator += curr->next->bytes[j];
            randAgitator %= rand()%(UINT32_MAXVAL+1);
        }
        curr = curr->next;
    }

    // Now, we flush all these LL nodes, translations cached in TLB should have lower
    // 'Cold Miss' latencies, since their translations would still be in TLB
    flushLinkedList(head);
    curr = head;
    for(int i=0; i<128; i++)
    {
        // TRIPLE check data is in L1
        for(int j=0; j<LL_NODE_PADDING; j++)
        {
            randAgitator += curr->bytes[j];
            randAgitator %= rand()%(UINT32_MAXVAL+1);
        }

        // Get latency of accessing the next node
        randIndex = rand()%(LL_NODE_PADDING);
        startTime=rdtsc();
        randAgitator += curr->next->bytes[randIndex];
        endTime=rdtsc();
        randAgitator %= rand()%(UINT32_MAXVAL+1);

        // Let's use the 2nd entry as baseline latency
        // Since head may have been pulled in from 'curr = head;'
        if(i=0)
        {
            baseLatency = endTime-startTime;
            curr = curr->next;
            continue;
        }

        // Check if this entry has a higher latency than baseline +/- deltaTolerance
        if(abs((endTime-startTime)-baseLatency) > deltaTolerance)
        {
            //printf("DEBUG: LATENCY OVER TOLERANCE DETECTED\n");
            //printf("DEBUG: i=%d, base=%lu, this=%lu, delta=%lu\n", 
            //    i, baseLatency, (endTime-startTime), abs((endTime-startTime)-baseLatency));
            
            L1TLB_Miss_Latency = L1d_Miss_Latency - (endTime-startTime);
            break;
        }

        curr = curr->next;
    }
    globalAgitator %= (globalAgitator+randAgitator+20);
    freeLinkedList(head);
}

// CACHE NUMBERS WERE PULLED FROM CPUID LOG ("cpuid -1 > cpuid.log") //
// WARNING - this INCLUDES L3 cache which is shared among all cores and will affect other tests running!
void volatile floodAllDataCaches(){
    //printf("Assuming Local L1d=64KB, Local L2=2MB, Per-Socket LLC (L3)=52.5MB\n");
    //printf("Flooding all these caches... (Warning - Flushing LLC will affect other active users!)\n");

    //printf("DEBUG: LL_NODE_PADDING=%d, sizeof(void*)=%d\n", LL_NODE_PADDING, sizeof(void*));

    spawnLLNodes(20000);
}

void volatile flushAllDataCaches(){
    // Span entire VA space, 0 -> 2^48, i+=8 for bytes
    printf("DEBUG-A\n");
    for(uint64_t i=1; i<MAX_VA_RANGE; i+=8)
    {
        printf("DEBUG-B: [%lu]\n", i);
        flushLiteral(i);
    }
}

// CACHE NUMBERS WERE PULLED FROM CPUID LOG ("cpuid -1 > cpuid.log") //
void volatile floodL1InstrCache(){
    //printf("Assuming Local L1i=32KB\n");
    //printf("Flooding instruction cache...\n");

    printf("NOTE: floodL1InstrCache() -- TBA\n");
}

void volatile setL1DMissLatency(){
    //printf("Calculating L1D Latency...\n");
    //printf("DEBUG: ListNode size *should* be 4096, actual=%d\n", sizeof(ListNode));
    ListNode *head, *curr;
    uint32_t randIndex;
    uint64_t startTime, endTime;
    uint64_t randAgitator = 6;
    uint64_t maxL1Pages = L1D_KB / 4;

    // Make a 'head' node for LL:
    head = createNode();
    curr = head;

    for(int i=1; i<maxL1Pages+1; i++)
    {
        // First let's make a new LL entry...
        curr->next = createNode();

        // ...Then TRIPLE check it is in L1
        for(int j=0; j<LL_NODE_PADDING; j++)
        {
            randAgitator += curr->next->bytes[j];
            randAgitator %= rand()%(UINT32_MAXVAL+1);
        }
        
        curr = curr->next;
    }

    // Now we should have evicted the head node from L1 Cache. Measure the latency of this access
    randIndex = rand()%(LL_NODE_PADDING);
    startTime=rdtsc();
    randAgitator += head->bytes[randIndex];
    endTime=rdtsc();
    randAgitator %= rand()%(UINT32_MAXVAL+1);

    L1d_Miss_Latency=endTime-startTime;
    //printf("Latency Observed for L1 Miss: %lu cycles.\n", L1d_Miss_Latency);
    globalAgitator %= (globalAgitator+randAgitator+20);
    freeLinkedList(head);
}

void volatile floodL1TLB(){
    //printf("Flooding L1 TLB...\n");

    // Make sure we have number of TLB entries
    if(L1TLB_Entries == 0)
        calculateL1TLB_Entries();

    spawnLLNodes(L1TLB_Entries+1);
}


void volatile spawnLLNodes(uint32_t numEntries)
{
    ListNode *head, *curr;
    uint32_t randIndex;
    uint64_t randAgitator = 76;
    uint64_t maxL1Pages = L1D_KB / 4;

    // Make a 'head' node for LL:
    head = createNode();
    curr = head;

    for(int i=1; i<numEntries; i++)
    {
        //if(i>4230)
        //printf("DEBUG: i=%d\n", i);
        // First let's make a new LL entry...
        curr->next = createNode();

        // ...Then TRIPLE check it is in L1
        for(int j=0; j<LL_NODE_PADDING; j++)
        {
            //if(i>4230 && j<5)
            //    printf("\tDEBUG2a: i=%d::j=%d\n", i, j);

            randAgitator += curr->next->bytes[j];
            //if(i>4230 && j<5)
            //    printf("\t\t>DEBUG2b: i=%d::j=%d\n", i, j);
            randAgitator %= rand()%(UINT32_MAXVAL+1);
        }
        
        //if(i>4230)
        //    printf(">> DEBUG3: i=%d\n", i);
        curr = curr->next;
    }
    globalAgitator %= (globalAgitator+randAgitator+20);
    flushLinkedList(head);
    freeLinkedList(head);
}

void fillNodeArr(ListNode *node){
    uint64_t randNum;

    for(int i=0; i<LL_NODE_PADDING; i++)
    {
        randNum = rand()%(255);
        node->bytes[i] = randNum;
    }
}

void flushLinkedList(ListNode* head){
    ListNode* tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        flush(tmp);
    }
}

void freeLinkedList(ListNode* head){
    ListNode* tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

ListNode* createNode(){
    ListNode* node = (ListNode*)malloc(sizeof(ListNode));

    if (node)
    {
        node->next = NULL;
        fillNodeArr(node);
    }
    else
        detailedAssert(false, "createNode() - Failed to create new LL node.");

    return node;
}

// Spawn thread on any core but main core
void spawnThreadOnDiffCore(pthread_t *restrict threadPTR, void* threadFN){
    cpu_set_t threadCPUSet;
    pthread_attr_t threadAttr;
    uint8_t mainCPUID=sched_getcpu();
    uint8_t threadCPUID=mainCPUID;

    // Get total cores in system (syscall)
    if(nprocs == 0)
        nprocs = sysconf(_SC_NPROCESSORS_ONLN);

    // Initialize the thread attributes / cpuset variable
    pthread_attr_init(&threadAttr);
    CPU_ZERO(&threadCPUSet);

    // Ensure thread spawns on diff core:
    while(threadCPUID<((mainCPUID+1)%nprocs) && threadCPUID>((mainCPUID-1)%nprocs))
        threadCPUID=rand()%nprocs;
    CPU_SET(threadCPUID, &threadCPUSet);
    
    pthread_create(threadPTR, &threadAttr, threadFN, NULL);
}

// Spawn thread on the specific sibling core (shared L1/L2)
void spawnThreadOnSiblingCore(pthread_t *restrict threadPTR, void* threadFN){
    cpu_set_t threadCPUSet;
    pthread_attr_t threadAttr;
    uint8_t mainCPUID=sched_getcpu();
    uint8_t threadCPUID; //, CPUSiblingGap;

    // Get total cores in system (syscall), sibling is +/- half this. 
    // (cat /sys/devices/system/cpu/cpu0/topology/thread_siblings_list)
    if(nprocs == 0)
        nprocs = sysconf(_SC_NPROCESSORS_ONLN);

    // Initialize the thread attributes / cpuset variable
    pthread_attr_init(&threadAttr);
    CPU_ZERO(&threadCPUSet);

    threadCPUID = (mainCPUID>=(nprocs/2))? mainCPUID-(nprocs/2): mainCPUID+(nprocs/2);
    CPU_SET(threadCPUID, &threadCPUSet);
    
    //printf("DEBUG: Spawning Checker Thread on CPU-%d, (This CPU=%d)\n", threadCPUID, mainCPUID);
    pthread_create(threadPTR, &threadAttr, threadFN, NULL);
}