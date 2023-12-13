#include <accel-config/libaccel_config.h> // For accel-config of Intel DSA

#include "../modules/otherFeatureTests.h" // All mem mov tests excluding DSA
#include "../modules/utils.h"             // Majority of includes, other util fns

// Size / settings
#define PORTAL_SIZE 4096
#define _headerStr_ "DSA_EnQ DSA_memmov ASM_movq C_memcpy SSE1_movaps "\
                    "SSE2_mov SSE4_mov AVX_256 AVX_512_32 AVX_512_64 AMX_LdSt\n"

// Pull in externs from utils.h
uint64_t flushReload_latency;
uint64_t RTDSC_latency;
uint64_t L1d_Hit_Latency;
uint64_t LLC_Miss_Latency;
uint64_t nprocs=0;

// WQ I used (defined in utils.h) _JMac
char* wqPath = WQ_PATH;

// Perf Counter Indexes, indexed 0-9 (10 entries -> NUM_TESTS=10)
typedef enum {
    DSAenqIndx, // Starts at 0
    DSAmovIndx,
    ASMmovqIndx,
    CmemIndx,
    SSE1Indx,
    SSE2Indx,
    SSE4Indx,
    AVX256Indx,
    AVX5_32Indx,
    AVX5_64Indx,
    AMXIndx,
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