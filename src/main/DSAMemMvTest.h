#include <accel-config/libaccel_config.h> // For accel-config of Intel DSA

#include "../modules/otherFeatureTests.h" // All mem mov tests excluding DSA
#include "../modules/utils.h"             // Majority of includes, other util fns

// Size / settings
#define PORTAL_SIZE 4096
#define _headerStr_ "DSA_EnQ DSA_memmov C_memcpy ASM_movq SSE1_movaps "\
                    "SSE2_mov SSE4_mov AVX_256 AVX_512_32 AVX_512_64 AMX_LdSt\n"

// Perf Counter Indexes, indexed 0-9 (10 entries -> NUM_TESTS=10)
typedef enum {
    DSAenqIndx, // Starts at 0
    DSAmovRIndx,
    CmemIndx,
    ASMmovqIndx,
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
uint8_t   descriptorRetry;

// Function List
void     ANTI_OPT   single_DSADescriptorInit();
void     ANTI_OPT   finalizeDSA();
uint8_t  ANTI_OPT   enqcmd(void *dst, const void *src);
void                parseResults(char* fileName);