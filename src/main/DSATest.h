#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <x86intrin.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <smmintrin.h> // Include for SSE4.1 (and earlier) intrinsics
#include <immintrin.h> // Include for AVX intrinsics
#include <accel-config/libaccel_config.h> // For accel-config of Intel DSA
#include <linux/idxd.h> // For important Macros

#include "../modules/otherFeatureTests.h"
#include "../modules/utils.h"

// Size / settings
#define BUF_SIZE 4096
#define PORTAL_SIZE 4096

// WQ I used _JMac
char* wqPath = "/dev/dsa/wq2.0";

// Descriptor Data
struct dsa_completion_record compRec __attribute__((aligned(32)));
struct dsa_hw_desc descr = {0};
char* srcDSA;
char* dstDSA;
void* wq_portal;

// Function List
void     ANTI_OPT   single_DSADescriptorInit();
void     ANTI_OPT   single_DSADescriptorSubmit();
void     ANTI_OPT   single_DSACompletionCheck();
uint8_t  ANTI_OPT   enqcmd(void *dst, const void *src);