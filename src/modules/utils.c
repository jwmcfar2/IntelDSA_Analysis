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
        startTime=rdtscp();
        randAgitator += curr->next->bytes[randIndex];
        endTime=rdtscp();
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

    spawnLLNodes(L1D_KB + L2_KB + L3_KB);
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
    startTime=rdtscp();
    randAgitator += head->bytes[randIndex];
    endTime=rdtscp();
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

void volatile floodHelperFn(){
    floodAllDataCaches();
    compilerMemFence();
    floodL1InstrCache();
    compilerMemFence();

    // It takes so many LL Nodes to flood L2/L3, the TLBs will surely be flooded indirectly...
    //floodL1TLB();
    //compilerMemFence();
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