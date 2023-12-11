#include <accel-config/libaccel_config.h> // For accel-config of Intel DSA

#include "../modules/otherFeatureTests.h" // All mem mov tests excluding DSA
#include "../modules/utils.h"             // Majority of includes, other util fns

// Size / settings
#define PORTAL_SIZE 4096
#define _headerStr_ "DSA_EnQ DSA_flush clflush clflushopt\n"

// Pull in externs from utils.h
uint64_t flushReload_latency;
uint64_t RTDSC_latency;
uint64_t L1d_Hit_Latency;
uint64_t LLC_Miss_Latency;
uint64_t nprocs=0;

// Perf Counter Indexes, indexed 0-3 (4 entries -> NUM_TESTS=4)
typedef enum {
    DSAenqIndx, // Starts at 0
    DSAFlushIndx,
    clflushIndx,
    clflushoptIndx,
    NUM_TESTS
} resIndxEnum;

// Run Modes
typedef enum {
    singleCold, // Starts at 0
    singleHits,
    singleContention,
    bulkCold,
    bulkHits,
    bulkContention,
    NUM_MODES
} modeEnum;

// WQ I used _JMac
char* wqPath = "/dev/dsa/wq2.0";

// Descriptor Data
struct dsa_completion_record compRec __attribute__((aligned(32)));
struct dsa_hw_desc descr = {0};
uint8_t* srcDSA;
uint8_t* dstDSA;
void* wq_portal;

// Program Vars
modeEnum  mode;
uint64_t  bufferSize;
uint64_t  resArr[NUM_TESTS];
uint64_t  startTimeEnQ, endTimeEnQ;
uint64_t  startTimeDSA, endTimeDSA;
pthread_t checkerThread, victimThread;
uint64_t  endTimePThread;
uint8_t   descriptorRetry;
bool      threadStarted=0;

// Function List
void        single_DSADescriptorInit();
void        finalizeDSA();
uint8_t     enqcmd(void *dst, const void *src);
void        adjustTimes();
void        parseResults(char* fileName);
void*       checkerThreadFn(void* args);
void        spectreDSADescriptorInit(volatile uint8_t *addr, size_t arrLen);
void        DSAMemMvSpectre();
void        TestSpectre();
void        initArr2AndFlush();
void        initDescrArrAndFlush();
uint8_t     spectreV1Gadget(size_t x);
uint8_t     spectreDSAGadget(size_t x);
void        testDescr();

// Structures needed for SpectreV1 Test
uint8_t arr1[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
uint8_t arr2[256*512];
char *spectreSecret = "Pass1234";
int guessMap[256];
char *secretGuess;
size_t arr1Len = 16;
size_t arr2Len = 256*512;
size_t secretLen;
uint8_t dummyReturnVar = 0;
char *secretDSAGuess;
struct dsa_hw_desc spectreDescr[256*512] = {0};
size_t spectreDescrLen = 256*512;