#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <x86intrin.h>
#include <smmintrin.h> // Include for SSE4.1 (and earlier) intrinsics
#include <immintrin.h> // Include for AVX intrinsics

#include "../modules/otherFeatureTests.h"
#include "../modules/utils.h"

// Function List
void ANTI_OPT DSADescriptorInit();
void ANTI_OPT DSADescriptorSubmit();
void ANTI_OPT DSACompletionCheck();