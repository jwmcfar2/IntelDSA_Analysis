#!/bin/bash
#
# NOTE - DO NOT RUN THIS SCRIPT DIRECTLY!!! TIMINGS WILL BE INACCURATE, USE THE 
# OTHER SCRIPT (initRun.sh) INSTEAD! (something about needing to use separate
# processes that './testBinary &' didnt solve)
#
# Quick Syntax = ./runScript [runCount] [runMode] [timeStamp]

#runFile[testTypeIndx]="bin/DSATest"
cpuAffin="7"

testTypes=("memmv" "flush" "cmp")
testFiles=("bin/DSAMemMvTest" "bin/DSAFlushTest" "bin/DSACmpTest")

runBufferSizes=(64 128 256 512 1024 2048 4096)
bufferSizeSuffix=("B" "KB")
bufferType=("single" "single" "bulk" "bulk" "bulk" "bulk")
modeType=("Cold" "Cold" "Bulk" "Bulk" "Contention" "Contention")
outFileArr=("" "" "" "" "" "" "")
declare -A runCountsArr # Array to allow us to randomly choose a buffer size.
currFileIndex=0

fileTimeStamp=$3
if [[ -z $fileTimeStamp ]]; then
    echo -e "No timestamp provided. Exiting..."
    echo -e "NOTE: DO NOT RUN THIS SCRIPT DIRECTLY! USE 'initRun.sh' INSTEAD!\n"
    exit
fi

quietFlag=0
if [[ "$4" == "-q" ]]; then
    quietFlag=1
fi

# Check if 'bin/DSA...' exists and is a file
if [[ ! -f "${testFiles[0]}" || ! -f "${testFiles[1]}" || ! -f "${testFiles[2]}" ]]; then
    echo -e "\nFATAL: Missing Binary - Compile source code using \"myScripts/buildScript.sh\". Exiting...\n"
    exit
fi

# Set number of runs
runCount=$1
runMode=$2
if [[ -z $runCount || $runCount -le 0 ]]; then
    echo -e "\nMissing/Invalid runCount - (Quicker syntax: \"./runScript [runCount] [runMode])\""
    read -p "Enter number of runs per test: " runCount
fi

# Choose run mode
if [[ -z $runMode || $runMode -lt 0 || $runMode -gt 5 ]]; then
    echo -e "\nMissing/Invalid runMode - (Quicker syntax: \"./runScript [runCount] [runMode]\")"
    read -p "Enter run mode number (0-5, defined in DSATest.h): " runMode
fi

# Initialize each bufferSize run count to 0
for bufferSize in "${bufferSizes[@]}"; do
    runCountsArr[$bufferSize]=0
done

for testTypeIndx in "${!testTypes[@]}"; do
    #echo -e "DEBUG: TestTypeIndx = $testTypeIndx"
    bufferSizes=("${runBufferSizes[@]}")

    # Randomly choose one of the bufferSizes to perform the test, to further reduce optimization
    while [ ${#bufferSizes[@]} -gt 0 ]; do
        index=$((RANDOM % ${#bufferSizes[@]}))
        selectedBufferSize=${bufferSizes[index]}
        sizeSubstr=$selectedBufferSize
        suffix="B" # Default suffix for bytes

        if [[ $selectedBufferSize -ge 1024 ]]; then
            # If bufferSize is 1024 or more, convert to kilobytes and change suffix
            sizeSubstr=$(($selectedBufferSize / 1024))
            suffix="KB"
        fi
        sizeSubstr+="$suffix"
        
        # Add file name to Arr if it doesn't exist
        outputResFile="results/${testTypes[testTypeIndx]}/${bufferType[runMode]}/"
        outputResFile+="${sizeSubstr}_${modeType[runMode]}_${fileTimeStamp}.out"
        for ((i=0; i<=currFileIndex; i++)); do
            # String exists in Arr already
            if [[ "${outFileArr[currFileIndex]}" == "$outputResFile" ]]; then
                break
            fi
            if [[ "${outFileArr[currFileIndex]}" == "" ]]; then
                outFileArr[currFileIndex]=$outputResFile
                (( currFileIndex++ ))
                break
            fi
        done

        if [[ quietFlag -eq 0 ]]; then
            echo -e "\nRunning Cold/${bufferType[runMode]}-Mode Tests for ${sizeSubstr}-sized Buffer..."
        fi

        # Run ./myProgram a single time if it hasn't reached 1000 runs yet
        if [ ${runCountsArr[selectedBufferSize]} -lt $runcount ]; then
            # Run the test
            #echo -e "DEBUG: Running Test \"${testFiles[testTypeIndx]} $selectedBufferSize $outputResFile $runMode\"..."
            ${testFiles[testTypeIndx]} $selectedBufferSize $outputResFile $runMode &
            wait $!
            
            # Increment the run count for chosen bufferSize
            runCountsArr[$selectedBufferSize]=$((runCountsArr[$selectedBufferSize] + 1))
            
            # Check if it has been run 1000 times and remove it if it has
            if [ ${runCounts[$selectedBufferSize]} -eq $runcount ]; then
                # Find the index of the buffer size to be removed
                for idx in "${!bufferSizes[@]}"; do
                    if [ "${bufferSizes[$idx]}" -eq "$selectedBufferSize" ]; then
                        # Remove the buffer size from the array by unsetting it
                        unset bufferSizes[idx]
                        break
                    fi
                done
                # Rebuild bufferSizes array to re-index elements
                bufferSizes=("${bufferSizes[@]}")
            fi
        fi

        mv $outputResFile ".tempFile.out"
        cat ".tempFile.out" | column -t > "$outputResFile"
        rm ".tempFile.out"
        if [[ $quietFlag -eq 0 ]]; then
            echo -e "\t...Done! Saved/Appended to: \"$outputResFile\""
        fi
    done
done

exit