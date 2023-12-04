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