#!/bin/bash
#
# NOTE - DO NOT RUN THIS SCRIPT DIRECTLY!!! TIMINGS WILL BE INACCURATE USE THE 
# OTHER SCRIPT (initRun.sh) INSTEAD! (something about needing to use separate
# processes that './runFile &' didnt solve)
#
# Quick Syntax = ./runScript [runCount] [runMode] [timeStamp]

runFile="bin/DSATest"
cpuAffin="7"

bufferSizes=(64 128 256 512 1024 2048 4096)
bufferSizeSuffix=("B" "KB")
bufferType=("single" "single" "single" "bulk" "bulk" "bulk")
modeType=("Cold" "Hits" "Contention" "Cold" "Hits" "Contention")
testTypes=("memmv" "flush") # "cmp")
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

# Check if 'bin/DSATest' exists and is a file
if [[ ! -f "$runFile" ]]; then
    echo -e "\nFATAL: '$runFile' Missing - Compile source code using \"myScripts/buildScript.sh\". Exiting...\n"
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
    outputResFile="results/${bufferType[runMode]}_mov/${sizeSubstr}_${modeType[runMode]}_${fileTimeStamp}.out"
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
        echo -e "\nRunning ${modeType[runMode]}/${bufferType[runMode]}-Mode Tests for ${sizeSubstr}-sized Buffer..."
    fi

    # Run ./myProgram a single time if it hasn't reached 1000 runs yet
    if [ ${runCountsArr[selectedBufferSize]} -lt $runcount ]; then
        
        # Rand 1-10
        #randCPU = echo $((1 + $RANDOM % 10))

        # Run the test
        $runFile $selectedBufferSize $outputResFile $runMode &
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

if [[ $quietFlag -eq 0 ]]; then
    echo -e "\n\t## Reminder: You can pass end of script to quiet script print-statements\n"
fi

# MOVED AVERAGE TO OTHER SCRIPT
exit