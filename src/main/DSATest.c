#include "DSATest.h"

#define BUF_SIZE 4096

int main(int runCount) {
    uint64_t startTime, endTime;
    srand(time(NULL));

    /*******************/
    // Module/Baseline Fn Tests
    single_memcpyC(BUF_SIZE);
    single_movqInsASM(BUF_SIZE);
    single_SSE1movaps(BUF_SIZE);
    single_SSE2movdqa(BUF_SIZE);
    single_SSE4movntdq(BUF_SIZE);
    single_AVX256(BUF_SIZE);
    single_AVX512_32(BUF_SIZE);
    single_AVX512_64(BUF_SIZE);

    // Intel DSA Fns
    //DSADescriptorInit();
    /*******************/

    return 0;
}

// Make new DSA Descriptor(s)
void ANTI_OPT DSADescriptorInit() {
}