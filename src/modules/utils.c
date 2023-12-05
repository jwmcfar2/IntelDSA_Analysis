#include "utils.h"

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

void detailedAssert(bool assertRes, const char* msg){
    if(!assertRes)
    {
        printf("Error: Assertion Failed. \n    ErrMsg: \"%s\"\n", msg);
        abort();
    }
}

void valueCheck(char* src, char* dst, uint64_t size, char* errDetails)
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

// CACHE NUMBERS WERE PULLED FROM CPUID LOG ("cpuid -1 > cpuid.log") //
// WARNING - this INCLUDES L3 cache which is shared among all cores and will affect other tests running!
void volatile floodAllDataCaches(){
    printf("Assuming Local L1d=64KB, Local L2=2MB, Per-Socket LLC (L3)=52.5MB\n");
    printf("Flushing all these caches... (Warning - Flushing LLC will affect other active users!)\n");

}

// CACHE NUMBERS WERE PULLED FROM CPUID LOG ("cpuid -1 > cpuid.log") //
void volatile floodL1InstrCache(){
    printf("Assuming Local L1i=32KB\n");
    printf("Flushing instruction cache...\n");
}

void volatile setL1DMissLatency(){
    printf("Calculating L1D Latency...\n");
    printf("DEBUG: ListNode size *should* be 4096, actual=%d\n", sizeof(ListNode));
    ListNode *head, *curr;
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
            randAgitator += curr->bytes[i]
            randAgitator %= rand()%(UINT32_MAX+1);
        }
        
        curr = curr->next;
    }

    // Now we should have evicted the head node from L1 Cache. Measure the latency of this access
    uint32_t randIndex = rand()%(LL_NODE_PADDING);
    startTime=rdtscp();
    randAgitator += head->bytes[randIndex]
    endTime=rdtscp();
    randAgitator %= rand()%(UINT32_MAX+1);

    L1d_Miss_Latency=endTime-startTime;
    printf("Latency Observed for L1 Miss: %lu cycles.\n", L1d_Miss_Latency);
}

void volatile calculateTLB(){
    printf("Calculating L1TLB...\n");
    printf("DEBUG: ListNode size *should* be 4096, actual=%d\n", sizeof(ListNode));
    ListNode *head, *curr;
    uint64_t randAgitator = 7;

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
            randAgitator += curr->bytes[i]
            randAgitator %= rand()%(UINT32_MAX+1);
        }
        curr = curr->next;
    }

    // Now, we flush all these LL nodes, translations cached in TLB should have lower
    // 'Cold Miss' latencies, since their translations would still be in TLB
    flushLinkedList(head);
    curr = head;
    for(int i=1; i<128; i++)
    {

    }
}

void volatile floodTLB(){
    printf("Assuming L1TLB=32\n");
    printf("Flushing instruction cache...\n");

}

ListNode* createNode() {
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

void fillNodeArr(ListNode *node){
    uint64_t randNum;

    for(int i=0; i<LL_NODE_PADDING; i++)
    {
        randNum = rand()%(255);
        printf("DEBUG: RandNum = %lu\n", randNum);
        node->bytes[i] = randNum;
    }
}

void flushLinkedList(ListNode* head) {
    ListNode* tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        flush(tmp);
    }
}

void freeLinkedList(ListNode* head) {
    ListNode* tmp;
    while (head != NULL) {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}