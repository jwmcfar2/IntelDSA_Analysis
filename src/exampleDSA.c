#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/idxd.h>
#include <accel-config/libaccel_config.h>
#include <x86intrin.h>
#include <time.h>
#include <emmintrin.h>  // Include for SSE2 intrinsics

#define MAX_LEN 4096
#define WQ_PORTAL_SIZE 4096
#define ENQ_RETRY_MAX 1000
#define POLL_RETRY_MAX 10000

// Uses RDTSCP Instead, which does the 'CPUID' serialization inherently, reducing possible performance overhead
// Adding 'always_inline' to try and force inlining for timer call (GCC only?) _JMac
inline __attribute__((always_inline)) uint64_t rdtscp()
{
    uint32_t low, high;
    __asm__ volatile ("RDTSCP\n\t"       // RDTSCP: Read Time Stamp Counter and Processor ID
                      "mov %%edx, %0\n\t"
                      "mov %%eax, %1\n\t"
                      : "=r" (high), "=r" (low)  // Outputs
                      :: "%rax", "%rbx", "%rcx", "%rdx"); // Clobbers
    return ((uint64_t)high << 32) | low;
}

static inline __attribute__((always_inline)) unsigned int
enqcmd(void *dst, const void *src)//, uint64_t startTime)
{
    uint8_t retry;
    uint64_t startTime = rdtscp();
    asm volatile(".byte 0xf2, 0x0f, 0x38, 0xf8, 0x02\t\n"
                 "setz %0\t\n"
                 : "=r"(retry) : "a" (dst), "d" (src));

    printf("\t > Completed DSA enqueue instruction *attempt*: Cycles elapsed = %lu cycles.\n", rdtscp()-startTime);
    printf("\t\t(Note: Accurate Cycle Reading likely requires concurrent thread\n\t\treading completion record at same time).\n\n", rdtscp()-startTime);

    return (unsigned int)retry;
}

static uint8_t
op_status(uint8_t status){ return status & DSA_COMP_STATUS_MASK; }

static void *
map_wq(void)
{
    void *wq_portal;
    struct accfg_ctx *ctx;
    struct accfg_wq *wq;
    struct accfg_device *device;
    char path[PATH_MAX];
    int fd;
    int wq_found;

    accfg_new(&ctx);

    /*accfg_device_foreach(ctx, device) {
        // Use accfg_device_(*) functions to select enabled device
        // based on name, numa node

        accfg_wq_foreach(device, wq) {
            if (accfg_wq_get_user_dev_path(wq, path, sizeof(path)))
                continue;

            // Use accfg_wq_(*) functions select WQ of type
            // ACCFG_WQT_USER and desired mode

            wq_found = accfg_wq_get_type(wq) == ACCFG_WQT_USER &&
            accfg_wq_get_mode(wq) == ACCFG_WQ_SHARED;
            if (wq_found)
                break;
        }

        if (wq_found)
            break;
    }*/

    // IDK what ^ does exactly, but I know the WQ I want to use...
    strcpy(path, "/dev/dsa/wq2.0");

    accfg_unref(ctx);
    
    //if (!wq_found)
    //    return MAP_FAILED;

    //printf("Test-A2\n");
    fd = open(path, O_RDWR);
    if (fd < 0)
        return MAP_FAILED;

    //printf("Test-A3\n");
    wq_portal = mmap(NULL, WQ_PORTAL_SIZE, PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    close(fd);
    
    //printf("Test-A4\n");
    return wq_portal;
}

int main(int argc, char *argv[])
{
    void *wq_portal;
    struct dsa_hw_desc desc = { };
    char src[MAX_LEN];
    char dst[MAX_LEN];
    char srcDummy[MAX_LEN];
    char dstDummy[MAX_LEN];
    char srcDummy2[MAX_LEN];
    char dstDummy2[MAX_LEN];
    char srcDummy3[MAX_LEN] __attribute__((aligned(16)));
    char dstDummy3[MAX_LEN] __attribute__((aligned(16)));
    char srcDummy4[MAX_LEN] __attribute__((aligned(32)));
    char dstDummy4[MAX_LEN] __attribute__((aligned(32)));
    struct dsa_completion_record comp __attribute__((aligned(32)));
    int rc;
    int poll_retry, enq_retry;
    uint64_t startTime, randVal;

    srand(time(NULL));

    //printf("Test-A\n");
    wq_portal = map_wq();
    if (wq_portal == MAP_FAILED)
        return EXIT_FAILURE;

    //printf("Test-B\n");

    // Set all test Values:
    randVal = rand()%(255);
    memset(src, randVal, MAX_LEN);
    randVal = rand()%(255);
    memset(srcDummy, randVal, MAX_LEN);
    randVal = rand()%(255);
    memset(srcDummy2, randVal, MAX_LEN);
    randVal = rand()%(255);
    memset(srcDummy3, randVal, MAX_LEN);
    randVal = rand()%(255);
    memset(srcDummy4, randVal, MAX_LEN);

    printf("\nTest Latency of ASM mem mov\n");
    // Inline assembly to move the value from src to dest
    // Test cycles for x86 ISA move:
    int i;
    startTime=rdtscp();
    for (i = 0; i < MAX_LEN; i+=8) {
        asm ("movq (%1), %%rax\n\t"
                    "movq %%rax, (%0);"
                    :
                    : "r" (dstDummy + i), "r" (srcDummy + i)
                    : "%rax"
                    );
    }
    printf("\t > Completed looped 64-bit CPU 'Mem Mov (movq)': Cycles elapsed = %lu cycles.\n\n", rdtscp()-startTime);

    printf("Test Latency of C 'memcpy' function\n");
    startTime=rdtscp();
    memcpy(dstDummy2, srcDummy2, MAX_LEN);
    printf("\t > Completed C 'memcpy' function: Cycles elapsed = %lu cycles.\n\n", rdtscp()-startTime);

    printf("Test Latency of SSE Mem Mov Instruction (movdqa)\n");
    startTime=rdtscp();
    for (i = 0; i < MAX_LEN; i += 16) {  // SSE2 register (128-bit or 16 byte) copy, 256 iterations
        __m128i data = _mm_load_si128((__m128i *)(srcDummy3 + i)); // Load 128-bits aligned data
        _mm_store_si128((__m128i *)(dstDummy3 + i), data); // Store 128-bits aligned data
    }
    printf("\t > Completed SSE Mem Mov Instructions: Cycles elapsed = %lu cycles.\n\n", rdtscp()-startTime);
    
    desc.opcode = DSA_OPCODE_MEMMOVE;

    /* Request a completion – since we poll on status, this flag
    * must be 1 for status to be updated on successful
    * completion */
    desc.flags = IDXD_OP_FLAG_RCR;

    /* CRAV should be 1 since RCR = 1 */
    desc.flags |= IDXD_OP_FLAG_CRAV;

    /* Hint to direct data writes to CPU cache */
    desc.flags |= IDXD_OP_FLAG_CC;

    desc.xfer_size = MAX_LEN;
    desc.src_addr = (uintptr_t)src;
    desc.dst_addr = (uintptr_t)dst;
    desc.completion_addr = (uintptr_t)&comp;

retry:
    comp.status = 0;

    /* Ensure previous writes are ordered with respect to ENQCMD */
    _mm_sfence();

    enq_retry = 0;
    printf("Test Latency of DSA mem mov\n");
    startTime = rdtscp();
    while (enqcmd(wq_portal, &desc) && enq_retry++ < ENQ_RETRY_MAX) ;
    printf("\t > Completed DSA enqueue instruction: Cycles elapsed = %lu cycles.\n", rdtscp()-startTime);
    if (enq_retry == ENQ_RETRY_MAX) {
        printf("ENQCMD retry limit exceeded\n");
        rc = EXIT_FAILURE;
        goto done;
    }

    poll_retry = 0;
    while (comp.status == 0 && poll_retry++ < POLL_RETRY_MAX)
    printf("\tVerified completed DSA instruction: Total cycles elapsed = %lu cycles.\n", rdtscp()-startTime);
    _mm_pause();

    if (poll_retry == POLL_RETRY_MAX) {
        printf("Completion status poll retry limit exceeded\n");
        rc = EXIT_FAILURE;
        goto done;
    }

    if (comp.status != DSA_COMP_SUCCESS) {
        if (op_status(comp.status) == DSA_COMP_PAGE_FAULT_NOBOF) {
        int wr = comp.status & DSA_COMP_STATUS_WRITE;
        volatile char *t;
        t = (char *)comp.fault_addr;
        wr ? *t = *t : *t;
        desc.src_addr += comp.bytes_completed;
        desc.dst_addr += comp.bytes_completed;
        desc.xfer_size -= comp.bytes_completed;
        goto retry;
        } else {
        printf("desc failed status %u\n", comp.status);
        rc = EXIT_FAILURE;
        }
    } else {
        printf("\nDescriptor executed successfully\n");
        rc = memcmp(src, dst, MAX_LEN);
        rc ? printf("ERROR: memmove failed.\n") : printf("SUCCESS: DSA Instruction memmove successful.\n", '+');
        rc = rc ? EXIT_FAILURE : EXIT_SUCCESS;
    }

done:
    munmap(wq_portal, WQ_PORTAL_SIZE);
    return rc;
}