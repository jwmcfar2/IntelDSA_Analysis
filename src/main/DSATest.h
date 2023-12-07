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
//uint8_t   checkThreadStarted; // Keeping bool as uint8 because pthreads is giving me a headache...
//uint8_t   submitThreadStarted; // Keeping bool as uint8 because pthreads is giving me a headache...
//uint8_t   finishedSubmission; // Keeping bool as uint8 because pthreads is giving me a headache...
//pthread_mutex_t lock;
//struct timespec ts = {0, 1}; // 5ns
//struct timespec ts;
//_MX       mutex = _MX_INIT;
//_MX_CND   cond  = _MX_CND_INIT;

// DEBUG LOGS
/*uint64_t DEBUG_MAIN_WAIT = 0;
uint64_t DEBUG_THREAD_WAIT = 0;
uint64_t DEBUG_INIT_TIME;
uint64_t DEBUG_BEGIN_TIME;
uint64_t DEBUG_TIMESTAMP_CHECKTHREAD;
uint64_t DEBUG_TIMESTAMP_SUBMITTHREAD;
uint64_t DEBUG_STARTLOCK;
uint64_t DEBUG_ENDLOCK;
uint64_t DEBUG_SEENLOCK;
uint64_t DEBUG_SUBMITFINISHCLOCK;*/

// Function List
void     ANTI_OPT   single_DSADescriptorInit();
//void     ANTI_OPT   single_DSADescriptorSubmit();
void     ANTI_OPT   finalizeDSA();
uint8_t  ANTI_OPT   enqcmd(void *dst, const void *src);
//void*               single_DSASubmitThread(void* arg);
//void*               single_DSACheckerThread(void* arg);
void                parseResults(char* fileName);